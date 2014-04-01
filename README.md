# Caboose
### Pantheon Terminus, Drush (Drupal) refresh command

Refresh the database on your local development environment from Pantheon and make changes necessary for your daily coding glory.

__Installation:__
 1. Clone into your ~/.drush folder.
 2. Set a site UUID in sites/default/settings.local.php
  * `$conf['caboose_pantheon_uuid'] = '44444444-3333-2222-1111-000000000000';`
 3. Clear Drush cache with ```drush cc drush``` command.
 4. Copy default.settings.inc as settings.inc to list modules to enable/disable.
 5. Copy default.sanitize.inc as sanitize.inc and adjust (recommended).
 6. Copy default.optional.inc as optional.inc and add your own preference changes (optional).

Run command from site root.

__Basic examples:__
 * ```drush caboose```
 * ```drush caboose test```
 * ```drush caboose dev```

Default environment is live.

__Notes:__
 * Be sure to login to Terminus via the ```drush pauth``` command.
 * If you haven't used other Terminus commands, you'll likely need to refresh aliases with the ```drush paliases`` command.