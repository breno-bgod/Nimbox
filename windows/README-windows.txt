NimboPlay Cloud Gaming for Windows - Portable Edition
=================================================

Xbox Cloud Gaming (xCloud) native client: browse the cloud catalog and stream
games with sound, with a controller (Xbox, DualSense, DualShock 4, 8BitDo,
Switch Pro...), keyboard and mouse.

You need a Microsoft account and a Game Pass plan with cloud gaming
(free-to-play games like Fortnite stream without it once on your account).
Unofficial project, not affiliated with, endorsed or sponsored by
Microsoft or Sony.


Running it
----------
Extract the zip archive to any folder and run:

    NimboPlay.exe

Experimental xCloud device access test (double-click, no commands needed):

    Testar-xCloud.cmd

Choose Killer Instinct 2 Classic, Grand Theft Auto VI or Fortnite.
Mode 1 checks the catalog and allowed devices; mode 2 also requests and
immediately closes a test session, only when the account is entitled.
This does NOT bypass device authorization or start gameplay. It reports
what Xbox services actually return to your account on this client.
Do not share screenshots containing your XUID, gamertag or sign-in codes.

Optional command-line parameters:
    --windowed      Start in windowed mode
    --fullscreen    Start in fullscreen mode
    --data-dir <dir> Store user data in custom folder

F11 or Alt+Enter toggles fullscreen; Settings > Display mode remembers it.

Self-test without signing in:
    NimboPlay.exe --selftest C:\Temp\psbox-selftest


Your data
---------
Sign-in credentials, settings, caches and log (xcloud.log) are kept in:
    %LOCALAPPDATA%\psbox\ (or specified by --data-dir)
Nothing is written to the program folder. Deleting %LOCALAPPDATA%\psbox\
completely resets the app.


Controllers
-----------
Standard XInput controllers (Xbox One, Xbox Series, 8BitDo) work out of the box.
PlayStation DualSense and DualShock 4 controllers are supported via SDL3 USB/Bluetooth HID.
