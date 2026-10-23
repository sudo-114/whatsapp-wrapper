# WhatsApp Wrapper (Win32 + WebView2)

A minimal Windows desktop wrapper for `https://web.whatsapp.com/`, written in C++.

## Features in this starter

- WebView2 window with persistent profile storage.
- Account 1 and Account 2 profile folders.
- System tray icon, minimize-to-tray, restore, and Exit menu.
- Reload and zoom commands.
- GitHub Actions Windows build workflow.

## Requirements

- Windows 10/11 x64
- Visual Studio 2022 Build Tools with Desktop development with C++
- CMake 3.24+
- Microsoft Edge WebView2 Runtime

## Build locally

Run PowerShell in the project root:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\build.ps1
```

Expected output: `build\Release\WhatsAppWrapper.exe`.

## Build with GitHub Actions

Push to the `master` branch or use Actions > Build WhatsApp Wrapper > Run workflow.
Download the `WhatsAppWrapper-Windows-x64` artifact from the workflow run.

## Separate accounts

Run one instance with the default profile and another with:

```powershell
.\WhatsAppWrapper.exe --profile account2
```

The profiles are stored under `%LOCALAPPDATA%\WhatsAppWrapper\account1` and
`%LOCALAPPDATA%\WhatsAppWrapper\account2`.

## Important limitations

This is a starter project and has not been compiled or tested on Windows in this environment.
Validate the WebView2 SDK package layout and CMake library path against the version downloaded.
Native Windows toast notifications and unread-count integration are not implemented here.
Web notifications depend on WhatsApp Web, WebView2 support, and Windows notification settings.
The tray menu lets you switch the current window's profile, but separate concurrent accounts
are better launched as two processes using `--profile account1` and `--profile account2`.
