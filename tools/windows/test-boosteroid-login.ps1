param([string]$Executable)
$ErrorActionPreference = 'Stop'
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class LoginProbe {
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string cls,string title);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr h,uint relation);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h,uint msg,IntPtr w,IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern void keybd_event(byte key,byte scan,uint flags,UIntPtr extra);
}
'@
function Press-Key([byte]$Key) {
    [LoginProbe]::keybd_event($Key,0,0,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 150
    [LoginProbe]::keybd_event($Key,0,2,[UIntPtr]::Zero)
    Start-Sleep -Milliseconds 250
}
$exe = (Resolve-Path $Executable).Path
$env:XCLOUD_DATA_DIR = Join-Path $env:RUNNER_TEMP nimbo-login-test
$app = Start-Process $exe -WorkingDirectory (Split-Path $exe) -ArgumentList '--windowed' -PassThru
try {
    $deadline = (Get-Date).AddSeconds(35)
    do {
        Start-Sleep -Milliseconds 500
        $app.Refresh()
        $hostWindow = $app.MainWindowHandle
    } until ($hostWindow -ne 0 -or (Get-Date) -gt $deadline -or $app.HasExited)
    if ($hostWindow -eq 0) { throw 'NimboPlay window did not open' }
    [void][LoginProbe]::SetForegroundWindow($hostWindow)
    Start-Sleep -Seconds 3
    Press-Key 0x27
    Press-Key 0x0D
    $deadline = (Get-Date).AddSeconds(45)
    do {
        Start-Sleep -Milliseconds 250
        $login = [LoginProbe]::FindWindow($null,'NimboPlay.Boosteroid.Ready')
    } until (($login -ne 0 -and [LoginProbe]::IsWindowVisible($login)) -or (Get-Date) -gt $deadline)
    if ($login -eq 0 -or ![LoginProbe]::IsWindowVisible($login)) { throw 'Boosteroid login never became visible' }
    if ([LoginProbe]::GetWindow($login,4) -ne $hostWindow) { throw 'Login is outside the NimboPlay host' }
    [void][LoginProbe]::PostMessage($login,0x10,0,0)
    Start-Sleep -Seconds 2
    $app.Refresh()
    if ($app.HasExited -or !$app.Responding -or [LoginProbe]::IsWindowVisible($login)) { throw 'Return to selector failed' }
} finally {
    if (!$app.HasExited) { $app.Kill($true) }
}
