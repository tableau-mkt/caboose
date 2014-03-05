<?php
/**
 * @file
 * Sanitize the local data for security.
 * Copy as sanitize.php and update for your needs.
 */


// Fields.
$sanitize_fields = array(
  'profile_hidden_email' => 'abc@example.com',
  'profile_last_name'    => 'Smith',
  'profile_phone'        => '555-555-5555',
);

// Accounts.
$email_domain = '@example.com';
$admin_pass   = 'password';
