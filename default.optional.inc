<?php

// Install your favorite development modules.
drush_print("Enabling custom preference modules.\n");
$modules = array(
  'devel',
  'coder',
  'coder_review',
  'update',
  'styleguide',
  'livejs',
);
if (!empty($modules)) {
  module_enable($modules);
}
