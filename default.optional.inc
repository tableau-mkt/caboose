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
  'forminspect',
  'module_builder',
  'project_src',
);
if (!empty($modules)) {
  module_enable($modules);
}
