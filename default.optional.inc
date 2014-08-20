<?php

// Install your favorite development modules.
drush_print("Enabling custom preference modules.\n");
$modules = array(
  'devel',
  'coder',
  'coder_review',
  'styleguide',
  'forminspect',
  'module_builder',
  'project_src',
  'search_krumo',
  'hacked',
);
if (!empty($modules)) {
  module_enable($modules, TRUE);
}

// Update permissions.
drush_print("Allowing anonymous users to use Devel.\n");
$query_auth = "INSERT INTO role_permission (rid, permission, module) VALUES (2, 'access devel information', 'devel')";
shell_exec(_drush_sql_connect() . ' -e "' . $query_auth . '"');
$query_anon = "INSERT INTO role_permission (rid, permission, module) VALUES (1, 'access devel information', 'devel')";
shell_exec(_drush_sql_connect() . ' -e "' . $query_anon . '"');

// Performance enhancements on the semaphore table.
$query_semaphore_alter =
  "ALTER TABLE semaphore ENGINE = MEMORY; ALTER TABLE semaphore DROP PRIMARY KEY; ALTER TABLE semaphore ADD PRIMARY KEY (name, value) USING BTREE; ALTER TABLE semaphore ADD UNIQUE name (name) USING BTREE; ALTER TABLE semaphore DROP INDEX value; ALTER TABLE semaphore ADD INDEX value (value) USING BTREE; ALTER TABLE semaphore DROP INDEX expire; ALTER TABLE semaphore ADD INDEX expire (expire) USING BTREE;";
shell_exec(_drush_sql_connect() . ' -e ' . escapeshellarg($query_semaphore_alter));
