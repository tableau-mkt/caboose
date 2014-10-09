<?php

/**
 * EXAMPLE: Update permissions.

drush_print("Allowing non-admin users to use Devel.\n");
foreach (array(1, 2, 8) as $role) {
  $query_anon = "INSERT INTO role_permission (rid, permission, module) VALUES ($role, 'access devel information', 'devel')";
  shell_exec(_drush_sql_connect() . ' -e "' . $query_anon . '"');
}
 */
