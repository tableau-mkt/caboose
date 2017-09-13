<?php
/**
 * @file
 * Enable/disable your site's modules.
 * Copy as settings.inc and update for your needs.
 */


$modules_disable = array(
  'redis',
  'new_relic_insights',
);

$modules_enable = array(
  'stage_file_proxy',
  'simpletest',
  'update',
  'link_css',
  'devel',
  'module_builder',
  'search_krumo'
);
