# Caboose
### Pantheon Terminus, Drush (Drupal) refresh command

Refresh the database on your local development environment from Pantheon and make changes necessary for your daily coding glory.

__Installation:__
 1. Clone into your ~/.drush folder.
 2. Set a site name in sites/default/settings.local.php
  * `$conf['caboose_pantheon_name'] = 'my-site';`
 3. Clear Drush cache with ```drush cc drush``` command.
 4. Copy default.settings.inc as settings.inc to list modules to enable/disable.
 5. Copy default.sanitize.inc as sanitize.inc and adjust (recommended).
 6. Copy default.optional.inc as optional.inc and add your own preference changes (optional).

Run command from site root.

__Basic examples:__
 * ```drush caboose```
 * ```drush caboose live```
 * ```drush caboose dev```

Default environment is test.

__Notes:__
 * Be sure you've installed [Terminus](https://github.com/pantheon-systems/cli/releases) (0.9.3 or greater is recommended)
 * Be sure to login to Terminus via the ```terminus auth login``` command.
 * Progress indicator on mysql import requires [`pv`](http://www.ivarch.com/programs/pv.shtml) (`brew install pv`)
 * Having `pcntl` functions allows handling interrupt signals (Ctrl+C). MAMP doesn't usually have these functions enabled.
