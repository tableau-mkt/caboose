<?php

// Install your favorite development modules.
$optional_modules_enable = array(
  'devel',
  'coder',
  'coder_review',
  'styleguide',
  'forminspect',
  'module_builder',
  'project_src',
  'search_krumo',
);

// Update permissions.
drush_print("Allowing anonymous users to use Devel.\n");

$query_auth = "INSERT INTO role_permission (rid, permission, module) VALUES (2, 'access devel information', 'devel')";
shell_exec(_drush_sql_connect() . ' -e "' . $query_auth . '"');

$query_anon = "INSERT INTO role_permission (rid, permission, module) VALUES (1, 'access devel information', 'devel')";
shell_exec(_drush_sql_connect() . ' -e "' . $query_anon . '"');

$query_anon = "INSERT INTO role_permission (rid, permission, module) VALUES (8, 'access devel information', 'devel')";
shell_exec(_drush_sql_connect() . ' -e "' . $query_anon . '"');
