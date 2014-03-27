<?php
/**
 * @file
 * Enable/disable your site's modules.
 * Copy as settings.php and update for your needs.
 */


$modules_disable = array(
  'redis',
  'tableau_user_profile_sync',
  'new_relic_insights',
);

$modules_enable = array(
  'stage_file_proxy',
  'simpletest',
  'features_plumber',
  'advanced_help',
  'update',
);
