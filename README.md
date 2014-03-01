# Caboose
### Pantheon Terminus, Drush command

Local refresh Drush command from Pantheon environments.

__Installation:__
 1. Clone into your ~/.drush folder.
 2. Copy default.optional.inc as optional.inc
 3. Set a site UUID in settings.local.php
 4. Run command from site root.

__Examples:__
 * ```drush caboose dev```
 * ```drush caboose test```
 * ```drush caboose live```

Default environment is test.