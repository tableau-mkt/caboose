# Caboose
### Pantheon Terminus, Drush (Drupal) refresh command

Refresh the database on your local development environment from Pantheon and make changes necessary for your daily coding glory.

__Installation:__
 1. Clone into your ~/.drush folder.
 2. Set a site UUID in sites/all/settings.local.php
  * $conf['caboose_pantheon_uuid'] = '44444444-3333-2222-1111-000000000000';
 3. Copy default.settings.inc as settings.inc to list modules to enable/disable.
 4. Copy default.optional.inc as optional.inc and add your own preference changes (optional).
 5. Run command from site root.

__Basic examples:__
 * ```drush caboose```
 * ```drush caboose test```
 * ```drush caboose dev```
Default environment is live.
