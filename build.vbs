Set WshShell = CreateObject("WScript.Shell")
WshShell.Run "cmd /c build.bat", 0
Set WshShell = Nothing