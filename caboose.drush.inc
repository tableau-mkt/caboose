<?php
/**
 * @file Local refresh Drush command from Pantheon environments.
 */

/**
 * Settings...
 */
define('DEFAULT_ENV', 'live');

/**
 * Implements hook_drush_help().
 */
function caboose_drush_help($section) {
  switch ($section) {
    case 'drush:caboose':
      return dt('Refresh the database on your local development environment from Pantheon and make changes necessary for your daily coding glory.');
  }
}

/**
 * Implementation of hook_drush_command().
 */
function caboose_drush_command() {
  $items['caboose-fresh'] = array(
    'description' => 'Refresh your local development environment from Pantheon.',
    'examples' => array(
      'drush caboose' => 'Download and install the latest backup of live production.',
      'drush caboose test' => 'Download and install latest backup of test environment.',
      'drush caboose dev' => 'Download and install latest backup of dev environment.',
    ),
    'arguments' => array(
      'environment' => 'Environment to grab database from.',
    ),
    // Required for variable_get. Maybe a way to avoid.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION,
    'aliases' => array('caboose'),
  );
  return $items;
}

/**
 * Command argument complete callback.
 */
function caboose_fresh_complete() {
  return array('values' => array('live', 'test', 'dev'));
}

/**
 * Command argument complete callback.
 */
function caboose_new_complete() {
  return array('values' => array('live', 'test', 'dev'));
}

/**
 * Validate arguments.
 */
function drush_caboose_validate($env = DEFAULT_ENV) {
  switch ($env) {
    case 'live':
    case 'test':
    case 'dev':
      // Just do it.
      break;
    default:
    return drush_set_error('NOT_A_THING', dt("\nSay what? That's not a thing.\n"));
      break;
  }
}


/**
 * Command callback. Refresh your local environment.
 *
 * @param $env
 *   The dev/test/live environment to target.
 */
function drush_caboose_fresh($env = DEFAULT_ENV) {
  _caboose_graphics('intro');

  // Get your data onto this environment.
  if (_caboose_install($env)) {

    // Kick up to a higher bootstrap level for altering the install.
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);

    // Shape up the install how you like it.
    if (_caboose_freshenup($env)) {

      _caboose_graphics('outro');
    }

  }
  return TRUE;
}


/**
 * Abstracted functions...
 */

/**
 * Run through freshening up steps.
 */
function _caboose_install($env) {
  // Sanity checking.
  if (!function_exists('terminus_drush_command')) {
    return drush_set_error('DRUSH_CABOOSE_NO_TERMINUM', 'Terminus not available.');
  }
  if (variable_get('caboose_pantheon_uuid', FALSE)) {
    $site_uuid = variable_get('caboose_pantheon_uuid', NULL);
  }
  else {
    return drush_set_error('DRUSH_CABOOSE_NO_SITE', 'No site UUID specified.');
  }

  // Get the user's session set.
  if ($session_data = terminus_bootstrap()) {
    extract($session_data);
  }
  else {
    return FALSE;
  }

  $target_site = '@self';

  // Download and load code modified from drush_terminus_pantheon_site_load_backup().
  // Retrieve the latest bucket.
  $bucket = terminus_latest_bucket($site_uuid, $env, 'database');

  drush_print('Downloading database from (' . $env . ') site backup.' . "\n");
  $destination = drush_tempdir();
  $result = terminus_api_backup_download_url($site_uuid, $env, $bucket, 'database');
  $data = json_decode($result['json']);
  $filename = strstr(basename($data->url), '?', '_');
  $cache_duration = 86400*365;

  $path = _drush_download_file($data->url, $destination . DIRECTORY_SEPARATOR . $filename, $cache_duration);

  if (!$path && !drush_get_context('DRUSH_SIMULATE')) {
    return drush_set_error('DRUSH_PSITE_DOWNLOAD_FAILED', 'Unable to download ' . $filename . ' to ' . $destination . ' from ' . $data->url);
  }

  drush_print("Download was successful. It's here...");
  drush_print("$path\n");

  drush_print("Extracting backup.\n");
  // This seems insane, but it won't work without.
  // NOTE: For some reason this is not necessary within Terminus.
  ini_set('memory_limit', '2048M');
  shell_exec('gunzip ' . $path);
  $path = strstr($path, '.gz', TRUE);

  drush_print("Overwriting database.");
  // Drop the whole DB to get started fresh.
  $db_spec = _drush_sql_get_db_spec();
  shell_exec(_drush_sql_connect() . ' -e "DROP DATABASE ' . $db_spec['database'] . '; CREATE DATABASE ' . $db_spec['database'] . '"');
  // Avoid PHP based import for memory reasons.
  shell_exec(_drush_sql_connect() . ' < ' . $path);
  drush_delete_dir($path);

  drush_print("Database load complete.\n");
}

/**
 * Run through steps to alter the data how you want.
 */
function _caboose_freshenup($env) {
  // Enable and disable modules.
  drush_print("Grabbing settings...\n");
  if (file_exists(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'settings.inc')) {
    require_once(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'settings.inc');

    if (isset($modules_disable) && count($modules_disable) > 0) {
      drush_print("Disabling modules.\n");
      module_disable($modules_disable);
    }

    drush_print("Running updates...");
    drush_invoke_process('@self', 'updb');

    if (isset($modules_enable) && count($modules_enable) > 0) {
      drush_print("\nEnabling modules.\n");
      module_enable($modules_enable);
    }
  }
  else {
    drush_print('No local module settings found. Did you copy that file correctly?');
  }

  // Sanitize the local data for security.
  drush_print("Sanitizing local data...\n");
  if (file_exists(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'sanitize.inc')) {
    require_once(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'sanitize.inc');

    // Overwrite fields.
    drush_print("Overwriting fields.\n");
    foreach($sanitize_fields as $field => $value) {
      $query = "UPDATE field_data_field_$field SET field_{$field}_value = '$value'";
      shell_exec(_drush_sql_connect() . ' -e "' . $query . '"');
      // Legacy approach.
      //$result = drush_invoke_process($target_site, 'sql-query', array($query), $options, $backend_options);
    }

    // Account details.
    drush_print("Overwriting account data.\n");
    // Leave some accounts alone.
    if (isset($unaltered_accounts)) {
      $accounts = array_map(function($value) {
        return '\'' . $value . '\'';
      }, $unaltered_accounts);
      $where = 'WHERE mail <> ' . implode(' AND mail <> ', $accounts);
    }
    $query = "UPDATE users SET mail = CONCAT(PASSWORD(mail), '$email_domain'), pass = 'password', init = CONCAT(PASSWORD(init), '$email_domain') $where";
    shell_exec(_drush_sql_connect() . ' -e "' . $query . '"');

    // Admin password.
    $user = user_load(1);
    user_save($user, array('pass' => $admin_pass));

  }
  else {
    drush_print('No sanitize steps found. You should really do that.');
  }

  // Optional local changes include.
  drush_print("Running custom commands...\n");
  if (file_exists(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'optional.inc')) {
    require_once(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'optional.inc');
  }
  else {
    drush_print('FYI: No custom steps found.');
  }

  drush_print("Done. Clearing cache.\n");
  // @todo Might be nice to clear the Drush cache.
  cache_clear_all();
}


/**
 * Draw cute stuff.
 */
function _caboose_graphics($stage) {
  switch ($stage) {
    case 'intro':

      $graphic = <<<EOD
  _     _   _       __             _                         
 | |___| |_( )___  / _|_ _ ___ ___| |_  ___ _ _    _  _ _ __ 
 | / -_)  _|/(_-< |  _| '_/ -_|_-<| ' \/ -_) ' \\  | || | '_ \\
 |_\___|\\__| /__/ |_| |_| \\___/__/|_||_\\___|_||_|  \\_,_| .__/
                                                       |_|   
      o o  o   o  ~~  ~~ ~                                   _____
    o     _____         ________________ ________________ ___|_=_|_()__
  .][_mm__|[]| ,===___ ||   PANTHEON   | |   TERMINUS   | | caboose  |
 >(_______|__|_|______]_|______________|_|______________|_|__________|_|
 _/oo-OOOO-oo' !oo!!oo!=`!o!o!----!o!o!'=`!o!o!----!o!o!'=`!o!o--o!o!'
=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=

EOD;

      break;

    case 'outro':

      $graphic = <<<EOD
                                          ;;;          
                                        ;f;ff;         
                                 ;tf;  ;fiffff;        
                               ;ffffff fffffff;        
     Friendship is magic!      fffffff;fffffff;        
                              ;fffGEffffj;fffjff       
                              jffLKKGff;fffffffjtff;   
                             ;fffKKKGffff;ffffjfffff;  
                             ;fffKKKDKKKKKGffffffffff  
                             ;ffGKKKEKKKKKKKfffffffff; 
               ;             ;fiGKKKKKKKKKKKKf;  ;fff; 
             ;ffff;          ;ffGKKKKKKKKKKKKG    fff; 
            jffffffi          fffffGKKK  KKKKK    fff  
       ;;  ;fffiff;fj      ;ffffffffKt####KKKK  ;fff;  
     ;ffff ;fifffffff;     jffffffffKD##WtiKKK ;ffi;   
    ;ffffffjffffffffff;   ;ffffffffjKK###  KKK ;f;;;   
    ;ffffffffffffffff;;   ;ffffff;jiKK###  KKD  ;;     
    ffffffffifffff;ffff   ;fff;ftfff;.W    KKG         
    fffjfff;tfffff;;fff   ;ffffffffff;K    KKG         
    fffffffffffffff ;f;    fffjfffffffKKKKKKKKG        
    ;fjffffffffffff  f;    ;ffffffffffKKKKKDKG         
    ;f;fff;ffffffff  ;;      ; fifffffGKKKKEG          
     fifffffifffff;  ;tGGGG   ;fffffftK                
   ;fffffffff;fffj   GKKKKKKG  fff;jEKK                
  ;ffff;fff;ffff;   GKWKKKKKKGGifff;KKKG               
  ffffffffffff;     KKKKKKKKKKKEKKjfEKKG               
  tfffffftfffft    GKDDKKKKKKKKKGffiKKKG               
  ;f;;fff;fffft    GKEDKKKGKKKKKKKKKKKK                
  ;iffffffffff;    GKKKKKKKKKKKKKKKKKKKGG              
; ifffffffffff     GKKKKKKGGGKKKKKKKKKGKK              
; ;ffffffffff;     GKKKKKKKKKDKKGGKKKKKGK              
;f;;ft;ffff;;       GGGKKKKKKKKKKKKKKKKKG              
 fffff  ;;         GKKKKKKKKKKKKKKKKKKKG               
  ;ff;             GGGGGKKKKGKKKKKKKKKG                
                        GGGGGGKKKKEGG                  

                  <<< Refresh complete >>>
    
EOD;

      break;

    default:
      $graphic = '';
  }
  
  drush_print($graphic . "\n");
}
