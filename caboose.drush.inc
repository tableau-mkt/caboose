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
      'drush caboose live' => 'Download and install the latest backup of production.',
      'drush caboose test --untouched' => 'Download and install latest backup of test environment but do not alter the site.',
      'drush caboose dev --now' => 'Create a backup of the dev environment, download and install.',
    ),
    'arguments' => array(
      'environment' => 'Environment to grab database from.',
    ),
    // Required for variable_get. Maybe a way to avoid.
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
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
 * Command callback. Refresh your local environment.
 *
 * @param $env
 *   The dev/test/live environment to target.
 *
 * @param $alter
 *   Whether to take any action against the database.
 */
function drush_caboose_fresh($env = DEFAULT_ENV) {
  drush_print("  _     _   _       __             _                         ");
  drush_print(" | |___| |_( )___  / _|_ _ ___ ___| |_  ___ _ _    _  _ _ __ ");
  drush_print(" | / -_)  _|/(_-< |  _| '_/ -_|_-<| ' \/ -_) ' \\  | || | '_ \\");
  drush_print(" |_\___|\\__| /__/ |_| |_| \\___/__/|_||_\\___|_||_|  \\_,_| .__/");
  drush_print("                                                       |_|   ");
  drush_print("      o o  o   o  ~~  ~~ ~                                   _____");
  drush_print("    o     _____         ________________ ________________ ___|_=_|_()__");
  drush_print("  .][_mm__|[]| ,===___ ||              | |              | |          |");
  drush_print(" >(_______|__|_|_GBRR_]_|              |_|              |_|          |_|");
  drush_print(" _/oo-OOOO-oo' !oo!!oo!=`!o!o!----!o!o!'=`!o!o!----!o!o!'=`!o!o--o!o!'");
  drush_print("=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=");
  drush_print("");

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
  $session_data = terminus_bootstrap();
  if ($session_data === FALSE) {
    return FALSE;
  }
  extract($session_data);

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

  drush_print("Overwriting database manually.");
  // Drop the whole DB to get started fresh.
  $db_spec = _drush_sql_get_db_spec();
  shell_exec(_drush_sql_connect() . ' -e "DROP DATABASE .' . $db_spec['database']);
  shell_exec(_drush_sql_connect() . ' -e "CREATE DATABASE .' . $db_spec['database']);
  // Avoid PHP based import for memory reasons.
  shell_exec(_drush_sql_connect() . ' < ' . $path);

  drush_print("Database load complete.\n");
  drush_delete_dir($path);

  // Sanitize the local data for security.
  drush_print("Grab settings...\n");
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

  // Optional local changes include.
  drush_print("Running custom commands...\n");
  if (file_exists(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'optional.inc')) {
    require_once(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'optional.inc');
  }

  drush_print("Done. Clearing cache.\n");
  // @todo Might be nice to clear the Drush cache.
  cache_clear_all();

  $end_graphic = <<<EOD
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
  drush_print($end_graphic . "\n\n");

  return TRUE;
}

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
