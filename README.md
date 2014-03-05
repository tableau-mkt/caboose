# Caboose
### Pantheon Terminus, Drush (Drupal) refresh command

Refresh the database on your local development environment from Pantheon and make changes necessary for your daily coding glory.

__Installation:__
 1. Clone into your ~/.drush folder.
 2. Set a site UUID in sites/all/settings.local.php
  * $conf['caboose_pantheon_uuid'] = '44444444-3333-2222-1111-000000000000';
 3. Clear Drush cache with ```drush cc drush``` command.
 4. Copy default.settings.inc as settings.inc to list modules to enable/disable.
 5. Copy default.optional.inc as optional.inc and add your own preference changes (optional).
 6. Run command from site root.

__Basic examples:__
 * ```drush caboose``` (default is live)
 * ```drush caboose test```
 * ```drush caboose dev```

__Database changes:__
To avoid any database changes after refresh use the untouched flag.

__Fresh backups:__
To create an immediate backup of the target environment you can use the "now" option.
 
 __Examples:__
 * ```drush caboose --untouched```
 * ```drush caboose --now```
