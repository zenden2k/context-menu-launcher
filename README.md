
# Static compilation
```powershell
# Compile Release
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' /m:8 /property:Configuration=Release .\singleinstance.sln

# Compile Release (Hidden Console)
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' /m:8 /property:Configuration=ReleaseHidden .\singleinstance.sln

# Check dll dependencies
&'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.43.34808\bin\Hostx64\x64\dumpbin.exe' /dependents .\Release\singleinstance.exe

# Check dll dependencies (Hidden Console)
&'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.43.34808\bin\Hostx64\x64\dumpbin.exe' /dependents .\Release\singleinstance_hidden.exe
```
**OR**
```
# Run this to automatically find Visual Studio and your current MSVC version, then build and test both versions.
build_singleinstance.ps1
```

# context-menu-launcher
Select multiple files from Windows Explorer menu and launch just one instance of process

[Download executable](https://github.com/owenstake/context-menu-launcher/releases/download/latest/singleinstance.exe)

## How to pass multiple files to shell context menu command (Windows Explorer)

You can achieve it with this program. No shell extensions involved.

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

## Hidden Console version

singleinstance_hidden.exe functions identically to the standard version but utilizes CreateProcessW with the CREATE_NO_WINDOW flag instead of ShellExecute.

This version is specifically engineered to suppress "console flashes"—those brief, distracting windows that appear when launching CLI-based applications. It is particularly effective for Python scripts that spawn subprocesses, where standard methods (like pyw.exe) often fail to fully hide the terminal. By forcing a detached process state, it ensures that neither the interpreter nor any child processes can trigger a visible console.
```
Windows Registry Editor Version 5.00

[HKEY_CLASSES_ROOT\SystemFileAssociations\.txt\Shell\p4merge]
"MultiSelectModel"="Player"

[HKEY_CLASSES_ROOT\SystemFileAssociations\.txt\Shell\p4merge\Command]
@="\"d:\\singleinstance_hidden.exe\" \"%1\" py.exe \"C:\\myawesomescript.py\" $files --si-timeout 400"
```
For debugging you may want to use singleinstance.exe until you have ironed out any bugs.

## Autobuilds and Anti-Virus Issues

Autobuilds and pre compiled .exe versions may trigger warnings from your AV software, especially for the hidden version. All builds are checked by Yara, BinSkim and ClamAV during the build process to ensure the code is clean. The main cause for the trigger is that the hidden versions CreateProcessW with the CREATE_NO_WINDOW flag is a powerful command that could be used for malicious purposes. Avast for example will sometimes on first run scan and send it for checking, after about 5-10 minutes it should allow you to use the .exe's unhindered.

If you build your own .exe's from source locally you should find your AV will not complain.
