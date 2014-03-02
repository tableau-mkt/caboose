<?php

// Query settings.
drush_print("Overwriting fields.\n");
$options = array(
  'db-spec' => $db_spec,
);
$backend_options = array(
  'integrate' => FALSE,
  'method' => 'POST',
  'override-simulated' => TRUE,
);


// Fields.
$fields = array(
  'profile_hidden_email' => 'abc@example.com',
  'profile_last_name' => 'Smith',
  'profile_phone' => '555-555-5555',
);


foreach($fields as $field) {
  $query = "UPDATE field_data_field_$field SET field_$field_value=''"
}
$result = drush_invoke_process($target_site, 'sql-query', array($query), $options, $backend_options);

// Accounts.
drush_print("Overwriting account data.\n");
$query = "UPDATE users SET mail=CONCAT(PASSWORD(mail), '@example.com'), pass='password', init=CONCAT(PASSWORD(init), '@example.com')"
$result = drush_invoke_process($target_site, 'sql-query', array($query), $options, $backend_options);

// Admin password.
_drush_user_password('admin', 'password');
