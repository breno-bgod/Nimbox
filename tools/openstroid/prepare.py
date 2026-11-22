"""Apply the NimboPlay Windows host integration to the pinned build checkout."""
from pathlib import Path
from shutil import copyfile

root = Path.cwd()
assets = Path(__file__).resolve().parent

def replace(path, old, new):
    target = root / path
    source = target.read_text(encoding="utf-8")
    if source.count(old) != 1:
        raise SystemExit(f"Unexpected OpenStroid source in {path}")
    target.write_text(source.replace(old, new), encoding="utf-8")

for name in ("HorizonHome.tsx", "horizon.css"):
    copyfile(assets / name, root / "src/pages" / name)
replace("electron/main.ts", "    width: 1440,", """    show: !process.argv.includes('--nimbo-embedded'),
    frame: !process.argv.includes('--nimbo-embedded'),
    skipTaskbar: process.argv.includes('--nimbo-embedded'),
    width: 1440,""")
replace("src/App.tsx", "import { AuthProvider }", "import { HorizonHome } from './pages/HorizonHome';\nimport { AuthProvider }")
replace("src/App.tsx", "const MyGamesPage = lazy(() => import('./pages/LibraryPage').then((module) => ({ default: module.MyGamesPage })));", "")
replace("src/App.tsx", '<Route path="/my-games" element={<MyGamesPage />} />', '')
replace("src/App.tsx", '<Route path="/login" element={<LoginPage />} />', '''<Route path="/login" element={<><button style={{position:'fixed',top:16,left:16,zIndex:100}} onClick={() => window.close()}>Voltar ao NimboPlay</button><LoginPage /></>} />
        <Route path="/my-games" element={<RequireAuth><HorizonHome /></RequireAuth>} />''')

replace("electron/main.ts", "  attachWindowLogging(window, 'main');", """  if (process.argv.includes('--nimbo-embedded')) {
    window.webContents.on('page-title-updated', event => event.preventDefault());
    window.hookWindowMessage(0x8000 + 125, () => {
      window.show();
      window.focus();
    });
    window.webContents.once('did-finish-load', async () => {
      try {
        // Wait for React to mount the login page or the signed-in home.
        await window.webContents.executeJavaScript(`new Promise(resolve => {
          const mounted = () => document.querySelector('.openstroid-login-logo,.horizon-home');
          if (mounted()) { resolve(true); return; }
          const observer = new MutationObserver(() => {
            if (mounted()) { observer.disconnect(); resolve(true); }
          });
          observer.observe(document.body,{subtree:true,childList:true});
        })`);
        if (!window.isDestroyed()) window.setTitle('NimboPlay.Boosteroid.Ready');
      } catch { if (!window.isDestroyed()) window.close(); }
    });
    window.webContents.on('render-process-gone', () => app.exit(1));
    window.on('unresponsive', () => app.exit(1));
  }
  attachWindowLogging(window, 'main');""")
replace("electron/main.ts", "  createMainWindow();\n  registerIpcHandlers();\n  registerAuthSessionIpc(ipcMain, authSessionStore);",
        "  registerIpcHandlers();\n  registerAuthSessionIpc(ipcMain, authSessionStore);\n  createMainWindow();")
replace("electron/main.ts", "  const { startBridgeServer }", "  if (process.argv.includes('--nimbo-embedded')) process.env.PORT = '0';\n  const { startBridgeServer }")
