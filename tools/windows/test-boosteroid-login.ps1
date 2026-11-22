param([string]$Executable)
# Headless Boosteroid integration smoke test. Real QR authorization requires
# a human account and is intentionally NOT simulated by this CI test.
$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class NimboLoginProbe {
    [DllImport("user32.dll", CharSet=CharSet.Unicode)]
    public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out Rect r);
    [DllImport("user32.dll")] public static extern IntPtr PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    public struct Rect { public int Left, Top, Right, Bottom; }
}
'@

$exe = (Resolve-Path $Executable).Path
$env:XCLOUD_DATA_DIR = Join-Path $env:RUNNER_TEMP 'nimbo-native-boosteroid-test'
New-Item -ItemType Directory -Path $env:XCLOUD_DATA_DIR -Force | Out-Null
$app = Start-Process $exe -WorkingDirectory (Split-Path $exe) -ArgumentList '--windowed' -PassThru
try {
    $deadline = (Get-Date).AddSeconds(35)
    do {
        Start-Sleep -Milliseconds 500
        $app.Refresh()
        $hostWindow = $app.MainWindowHandle
    } until ($hostWindow -ne 0 -or $app.HasExited -or (Get-Date) -gt $deadline)
    if ($hostWindow -eq 0) { throw 'NimboPlay native window did not open' }

    [void][NimboLoginProbe]::SetForegroundWindow($hostWindow)
    Start-Sleep -Seconds 3
    $rect = New-Object NimboLoginProbe+Rect
    [void][NimboLoginProbe]::GetClientRect($hostWindow, [ref]$rect)
    $coords = ([int]($rect.Bottom * .49) -shl 16) -bor [int]($rect.Right * .70)
    [void][NimboLoginProbe]::PostMessage($hostWindow, 0x200, 0, $coords)
    [void][NimboLoginProbe]::PostMessage($hostWindow, 0x201, 1, $coords)
    [void][NimboLoginProbe]::PostMessage($hostWindow, 0x202, 0, $coords)

    $logPath = Join-Path $env:XCLOUD_DATA_DIR 'xcloud.log'
    $ready = $false
    $deadline = (Get-Date).AddSeconds(60)
    do {
        Start-Sleep -Milliseconds 500
        $app.Refresh()
        if ($app.HasExited) { throw 'NimboPlay exited while starting Boosteroid' }
        if (-not $app.Responding) { throw 'Native Horizon stopped responding' }
        if (Test-Path $logPath) {
            $logText = Get-Content -Raw -Path $logPath -ErrorAction SilentlyContinue
            $ready = $logText -match 'Boosteroid: background helper ready'
        }
    } until ($ready -or (Get-Date) -gt $deadline)
    if (-not $ready) {
        Get-Process | Where-Object ProcessName -Match 'NimboPlay|OpenStroid' |
            Select-Object ProcessName,Id,MainWindowTitle,Responding | Out-Host
        if (Test-Path $logPath) { Get-Content $logPath -Tail 80 | Out-Host }
        throw 'Boosteroid local helper did not become ready'
    }

    # Login/catalog must remain in the native Horizon. The Electron window is
    # only allowed to become visible when an actual streaming game starts.
    $foreignMenu = [NimboLoginProbe]::FindWindow('Chrome_WidgetWin_1', 'NimboPlay.Boosteroid.Ready')
    if ($foreignMenu -ne [IntPtr]::Zero) {
        throw 'Boosteroid opened a second Electron menu instead of Horizon'
    }
    Write-Host 'OK: Horizon responsive, backend ready, no second login window.'
    Write-Host 'Real account login, catalog and gameplay still require manual QA.'
} finally {
    $app.Refresh()
    if (-not $app.HasExited) { $app.Kill($true) }
}
