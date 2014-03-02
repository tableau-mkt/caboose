# Caboose
### Pantheon Terminus, Drush (Drupal) refresh command

Refresh the database on your local development environment from Pantheon and make changes necessary for your daily coding glory.

__Installation:__
 1. Clone into your ~/.drush folder.
 2. Set a site UUID in sites/all/settings.local.php
  * $conf['caboose_pantheon_uuid'] = '44444444-3333-2222-1111-000000000000';
 3. Copy default.settings.inc as settings.inc and adjust (optional).
 4. Copy default.optional.inc as optional.inc and adjust (optional).
 5. Run command from site root.

__Basic examples:__
 * ```drush caboose live```
 * ```drush caboose test```
 * ```drush caboose dev```
Default environment is test.

__Database changes:__
To avoid any database changes after refresh use the untouched flag.

__Fresh backups:__
To create an immediate backup of the target environment you can use the "now" option. Careful of live!
 
 __Examples:__
 * ```drush caboose live``` Download and install the latest backup of production.
 * ```drush caboose test --untouched``` Download and install latest backup of test environment but do not alter the site.
 * ```drush caboose dev --now``` Create a backup of the dev environment, download and install.