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
    'bootstrap' => function_exists('variable_get') ? DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION : DRUSH_BOOTSTRAP_DRUPAL_FULL,
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
  if (file_exists(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'say.inc')) {
    require_once(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'say.inc');
  }

  _caboose_bling('intro', $message);

  // Get your data onto this environment.
  if (_caboose_install($env)) {

    // Shape up the install how you like it.
    if (_caboose_freshenup($env)) {
      _caboose_bling('outro');
    }

  }

  return TRUE;
}


/**
 * Run through freshening up steps.
 */
function _caboose_install($env) {
  $terminus = exec('which terminus');

  if (function_exists('variable_get')) {
    $site_uuid = variable_get('caboose_pantheon_uuid', NULL);
    $site_name = variable_get('caboose_pantheon_site', NULL);
  }
  else {
    $site_uuid = \Drupal::config('custom')->get('caboose_pantheon_uuid');
    $site_name = \Drupal::config('custom')->get('caboose_pantheon_site');
  }

  // Sanity checking.
  if (empty($terminus)) {
    return drush_set_error('DRUSH_CABOOSE_NO_TERMINUS', 'Terminus not available.');
  }
  if (version_compare(phpversion(), '5.5') < 0) {
    return drush_set_error('DRUSH_CABOOSE_PHP_55_MIN', 'Terminus requires PHP 5.5 at a minimum.');
  }

  // Ensure we're logged in.
  $exit_code = NULL;
  $array = array();
  exec($terminus . ' auth whoami', $array, $exit_code);
  if ($exit_code !== 0) {
    return drush_set_error('DRUSH_CABOOSE_NO_AUTH', 'Terminus is not authenticated with Pantheon.');
  }

  // Backwards compatibility layer, load site name from given UUID if available.
  if (empty($site_name) && !empty($site_uuid)) {
    $sites_info = json_decode(exec($terminus . ' sites list --format=json'));
    foreach ($sites_info as $site_info) {
      if ($site_info->id === $site_uuid) {
        $site_name = $site_info->name;
        break;
      }
    }
  }

  // If we still have no Pantheon site name, abort.
  if (empty($site_name)) {
    return drush_set_error('DRUSH_CABOOSE_NO_SITE', 'No site UUID or name specified.');
  }

  // Determine the latest backup URL and download it to the drush tmp folder.
  // terminus site backups get --site={$site_name} --env={$env} --element=db --latest --format=json
  drush_print('Downloading database from (' . $env . ') site backup.' . "\n");
  $destination = drush_tempdir();
  $raw_url = exec($terminus . ' site backups get --site=' . escapeshellarg($site_name) . ' --env=' . escapeshellarg($env) . ' --element=db --latest --format=json');
  $url = json_decode($raw_url);
  $filename = strstr(basename($url), '?', '_');

  $path = _caboose_download_file($url, $destination . DIRECTORY_SEPARATOR . $filename, TRUE);

  if (!$path && !drush_get_context('DRUSH_SIMULATE')) {
    return drush_set_error('DRUSH_PSITE_DOWNLOAD_FAILED', 'Unable to download ' . $filename . ' to ' . $destination . ' from ' . $url);
  }

  drush_print("Download was successful. It's here...");
  drush_print("$path\n");

  drush_print("Overwriting database.");

  if (function_exists('_drush_sql_get_db_spec')) {
    $db_spec = _drush_sql_get_db_spec();
    $connect_string = _drush_sql_connect();
  }
  else {
    $db_spec = drush_sql_get_class()->db_spec();
    $connect_string = drush_sql_get_class()->connect();
  }

  // Drop the whole DB to get started fresh.
  shell_exec($connect_string . ' -e "DROP DATABASE ' . $db_spec['database'] . '; CREATE DATABASE ' . $db_spec['database'] . '"');
  // If pv command exists, use it to give an import progress bar
  if (_caboose_command_exists('pv')) {
    $import_cmd = "pv $path | gzcat | " . $connect_string;
  }
  else {
    $import_cmd = "gzcat $path | " . $connect_string;
  }
  // Import the SQL file, using system to avoid PHP based import (for memory reasons).
  system($import_cmd);
  drush_delete_dir($path);

  drush_print("Database load complete.\n");

  return TRUE;
}

/**
 * Run through steps to alter the data how you want.
 */
function _caboose_freshenup($env) {

  if (function_exists('_drush_sql_connect')) {
    $connect_string = _drush_sql_connect();
  }
  else {
    $connect_string = drush_sql_get_class()->connect();
  }

  // Kick up to a higher bootstrap level for altering the install.
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);

  // Enable and disable modules.
  drush_print("Grabbing settings...\n");
  if (file_exists(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'settings.inc')) {
    require_once(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'settings.inc');

    if (isset($modules_disable) && count($modules_disable) > 0) {
      drush_print("Disabling modules.\n");
      if (function_exists('module_disable')) {
        module_disable($modules_disable, TRUE);
      }
      else {
        try {
          \Drupal::service('module_installer')->uninstall($modules_disable, TRUE);
        }
        catch (\Drupal\Core\Extension\MissingDependencyException $e) {
          drush_print('Error enabling module: ' . $e->getMessage());
        }
      }
    }

    drush_print("Running updates...");
    drush_invoke_process('@self', 'updb');

    if (isset($modules_enable) && count($modules_enable) > 0) {
      drush_print("\nEnabling modules.\n");
      if (function_exists('module_enable')) {
        module_enable($modules_enable, TRUE);
      }
      else {
        try {
          \Drupal::service('module_installer')->install($modules_enable, TRUE);
        }
        catch (\Drupal\Core\Extension\MissingDependencyException $e) {
          drush_print('Error enabling module: ' . $e->getMessage());
        }
      }
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
      shell_exec($connect_string . ' -e "' . $query . '"');
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
    shell_exec($connect_string . ' -e "' . $query . '"');

    // Admin password.
    $user = user_load(1);
    if (method_exists($user, 'save')) {
      $user->setPassword($admin_pass);
      $user->save();
    }
    else {
      user_save($user, array('pass' => $admin_pass));
    }

  }
  else {
    drush_print('No sanitization steps found. You should really do that.');
  }

  // Optional local changes include.
  if (file_exists(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'optional.inc')) {
    drush_print("Running custom commands...\n");
    require_once(dirname(__FILE__) . DIRECTORY_SEPARATOR . 'optional.inc');
  }
  else {
    drush_print('FYI: No custom steps found.\n');
  }

  drush_print("Done. Clearing cache.\n");
  // @todo Might be nice to clear the Drush cache.
  drupal_flush_all_caches();

  return TRUE;
}


/**
 * Abstracted functions...
 */


/**
 * Check if a system command exists.
 * @param  string $cmd
 *   Name of command to check for.
 * @return boolean
 *   Result of if command exists on the system.
 */
function _caboose_command_exists($cmd) {
  $returnVal = null;
  exec("which $cmd", $output, $returnVal);
  return $returnVal === 0;
}


/**
 * Utility function to print and optionally overwrite (useful for animations)
 * @param  string $str
 *   A string to print
 * @param  boolean $overwrite
 *   Overwrite previous lines
 */
function _caboose_print($str, $overwrite = TRUE) {
  if ($overwrite) {
    $num_lines = substr_count($str, "\n");
    print chr(27) . "[" . $num_lines . "A" . // Set cursor up x lines
      chr(27) . "[0G" ; // Set cursor to first column

  }
  print $str;
}


/**
 * Based on drush_shell_exec, but backgrounds the process and returns the PID
 *
 * Executes a shell command. Output is only printed if in verbose mode. Output is stored and can be retrieved using drush_shell_exec_output(). If in simulation mode, no action is taken.
 *
 * @param string $cmd
 *   The command to execute. May include placeholders used for sprintf.
 * @param  ...
 *   Values for the placeholders specified in $cmd. Each of these will be passed through escapeshellarg() to ensure they are safe to use on the command line.
 * @return boolean
 *   TRUE on success, FALSE on failure
 */
function _caboose_shell_exec($cmd) {
  $args = func_get_args();

  //do not change the command itself, just the parameters.
  for ($x = 1; $x < sizeof($args); $x++) {
    $args[$x] = escapeshellarg($args[$x]);
  }
  $command = call_user_func_array('sprintf', $args);

  if (drush_get_context('DRUSH_VERBOSE') || drush_get_context('DRUSH_SIMULATE')) {
    drush_log('Executing: ' . $command);
  }

  if (!drush_get_context('DRUSH_SIMULATE')) {
    $pid = exec($command . ' > /dev/null 2>&1 & echo $!', $output, $result);

    return ($result == 0) ? $pid : FALSE;
  }
}


/**
 * Based on drush_download_file, but instead calls _caboose_shell_exec
 *
 * @param string $url
 *   The url of the file to download.
 * @param string $destination
 *   The name of the file to be saved, which may include the full path.
 * @param boolean $overwrite
 *   Overwrite any file thats already at the destination.
 * @return string
 *   The path to the downloaded file, or FALSE if the file could not be downloaded.
 */
function _caboose_download_file($url, $destination, $overwrite = TRUE) {
  static $use_wget;
  if ($use_wget === NULL) {
    $use_wget = drush_shell_exec('which wget');
  }

  $destination_tmp = drush_tempnam('download_file');
  if ($use_wget) {
    $pid = _caboose_shell_exec("wget -q --timeout=30 -O %s %s", $destination_tmp, $url);
  }
  else {
    $pid = _caboose_shell_exec("curl --fail -s -L --connect-timeout 30 -o %s %s", $destination_tmp, $url);
  }

  // Wait for the process to finish and animate some bling
  _caboose_bling('working', NULL, $pid);

  if (!drush_file_not_empty($destination_tmp) && $file = @file_get_contents($url)) {
    @file_put_contents($destination_tmp, $file);
  }
  if (!drush_file_not_empty($destination_tmp)) {
    // Download failed.
    return FALSE;
  }

  drush_move_dir($destination_tmp, $destination, $overwrite);
  return $destination;
}


/**
 * Do cute stuff.
 * @param string $stage
 *   Place in the process.
 * @param integer
 *   Optional process ID to watch
 * @param integer
 *   Number of microseconds to sleep during frames
 */
function _caboose_bling($stage, $message = '', $pid = NULL, $microseconds = 100000) {
  switch ($stage) {
    case 'intro':
      if (empty($message)) {
        $message = 'All aboard.';
      }
      $graphic = <<<EOD
  _     _   _       __             _
 | |___| |_( )___  / _|_ _ ___ ___| |_  ___ _ _    _  _ _ __
 | / -_)  _|/(_-< |  _| '_/ -_|_-<| ' \/ -_) ' \  | || | '_ \
 |_\___|\__| /__/ |_| |_| \___/__/|_||_\___|_||_|  \_,_| .__/
                                                       |_|
EOD;
      break;
    case 'working':
      $graphic = array(
<<<EOD
      o o                                                    _____
    o     _____         ________________ ________________ ___|_=_|_()__
  .][_mm__|[]| ,===___ ||   PANTHEON   | |              | |          |
 >(_______|__|_|______]_|______________|_|______________|_|__________|_|
 _/oo-OOOO-oo' !oo!!oo!=`!o!o!----!o!o!'=`!o!o!----!o!o!'=`!o!o--o!o!'
===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠=
EOD
,
<<<EOD
      o  ~~ ~                                                _____
    o     _____         ________________ ________________ ___|_=_|_()__
  .][_mm__|[]| ,===___ ||   PANTHEON   | |   TERMINUS   | |          |
 >(_______|__|_|______]_|______________|_|______________|_|__________|_|
 _/oo-OOOO-oo' !oo!!oo!=`!o!o!----!o!o!'=`!o!o!----!o!o!'=`!o!o--o!o!'
≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠
EOD
,
<<<EOD
      o o  ~~  ~~ ~                                          _____
    o     _____         ________________ ________________ ___|_=_|_()__
  .][_mm__|[]| ,===___ ||   PANTHEON   | |   TERMINUS   | | caboose  |
 >(_______|__|_|______]_|______________|_|______________|_|__________|_|
 _/oo-OOOO-oo' !oo!!oo!=`!o!o!----!o!o!'=`!o!o!----!o!o!'=`!o!o--o!o!'
=≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===
EOD
,
<<<EOD
      o o  o   o  ~~  ~~ ~                                   _____
    o     _____         ________________ ________________ ___|_=_|_()__
  .][_mm__|[]| ,===___ ||              | |              | |          |
 >(_______|__|_|______]_|______________|_|______________|_|__________|_|
 _/oo-OOOO-oo' !oo!!oo!=`!o!o!----!o!o!'=`!o!o!----!o!o!'=`!o!o--o!o!'
==≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠===≠==
EOD
        );
      break;

    case 'outro':
      if (empty($message)) {
        $message = 'The train is in the station.';
      }
      $graphic = <<<EOD
             LLftttttftf
            fttttLttffttf fttttttttt
    tttfLLtttffftftffLftLLtl:lttttttt            +----------------------+
 ttttttttttttttftflllllLftl::i;iftttfC           | Friendship is magic! |
ttttff   ftttLi;:::::::::;;::;i:iffLLf           +----------------------+
tttf        i:::::::::::::::;;i::ltfft
 ttt        ::::::::;;::;::::::::lfLtf
   fttf    :::::;l@..CC@L@;::CffffLfL
     ff    ::::;@. lf@@ .@i:Ltttttttttttf                  ffffff
            :::;@ iL@@L  i;;Ltttttttttttff          LtffLffffLLffttf
            ;::;@ lL@@@@@i::;LLfftttttffftf       ffLtttttttttttfLttf
            :::::;..CLL;::;fttttttttfLftttf     fffttffffffttttttfLtf
           :::i::::::::::;ftttftttttfLtttf     tLtf    ftttttttttfLtLCffttf
              ::::::;;   CtttttttLfLLLLL      tC      ftttttttttfLtfttttttttfL
                        ;;LttttttfLttC::::t;;:i      ftttttttttffLLLLffttttttfC
                        :::;LfffLfttfC:filft;llf      ftttttttttttttttfLfttttf
                        :::::CfffLCi::::f;f:lli;;       fttftttttftttttfftttf
                         :::;CffLC;:::::;;f:t:::;        fLtttttftttttfLftf
                         ;::::::::::::;:::;;i::;        ttttttttttttffffL
                            i;:::;;;;;;;l:::::;        tttttfffffLLfftttttf
                            i;:::;     iii;:::::;      fttttttfLftttttttttt
                            i;::::i       ;i;:::::;     ftttttffftftttfttf
                            i;::::i        ;;;:::::      ffttfLLfttfttff
                           ;l:::::;        ;;;l::::;            CLLfftff
                           ;l:::::;        ;:;l;::::;          fttf
                          :l::::::;        ::;l:::::;             ffttttf
                         :;;::::::;        ::;;:::::;i
                        :;;::::::;;       ::;:::::::;
                         :::::::;;          :::::::;;

                  <<< Refresh complete >>>

EOD;
      break;

    default:
      $graphic = '';
  }

  if (is_array($graphic) && isset($pid)) {
    $overwrite = FALSE;
    $frame = 0;
    $count = count($graphic);
    do {
      _caboose_print($graphic[$frame % $count], $overwrite);
      usleep($microseconds);
      $overwrite = TRUE;
      $frame++;
      system("kill -0 $pid 2>/dev/null", $retval);
    } while ($retval == 0);
    print "\n";
  }
  else {
    drush_print($graphic . "\n");
  }

  if (_caboose_command_exists('say')) {
    $say_cmd = "say $message > /dev/null 2>/dev/null &";
    shell_exec($say_cmd);
  }

}
