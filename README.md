# context-menu-launcher
Select multiple files from Windows Explorer menu and launch just one instance of process

[Download executable](https://github.com/zenden2k/context-menu-launcher/releases/tag/1.0)

## How to pass multiple files to shell context menu command (Windows Explorer)

You can archive it with this program. No shell extensions involved.

The main idea is that one instance of my program will be launched per file you have selected. It is checking if another instance of singleinstance program is running, and using Inter-Process Communication to notify the existing instance that other files have been selected.

Do not forget to set registry option MultiSelectModel=Player, otherwise number of files will be limited.

```
Usage: singleinstance.exe "%1" {command} $files [arguments]

Optional arguments for singleinstance (not passed to command):

--si-timeout {time to wait in msecs}
```

Sample registry file:
```
Windows Registry Editor Version 5.00

[HKEY_CLASSES_ROOT\SystemFileAssociations\.txt\Shell\p4merge]
"MultiSelectModel"="Player"

[HKEY_CLASSES_ROOT\SystemFileAssociations\.txt\Shell\p4merge\Command]
@="\"d:\\singleinstance.exe\" \"%1\" \"C:\\Program Files\\Perforce\\p4merge.exe\" $files --si-timeout 400"
```
