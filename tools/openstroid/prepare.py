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
copyfile(assets / "HorizonLayout.tsx", root / "src/layouts/AuthenticatedLayout.tsx")
replace("src/App.tsx", '<MyGamesPage />', '<HorizonHome />')
replace("src/App.tsx", '<LoginPage />', '<><button className="hz-return" onClick={() => window.close()}>Voltar ao NimboPlay</button><LoginPage /></>')
# Branding shown to the user only: preserve executable names, IPC and session keys.
replace("index.html", "<title>OpenStroid — Cloud Gaming</title>", "<title>NimboPlay — Boosteroid</title>")
replace("index.html", 'href="/logo.png"', 'href="/nimbo-service.svg"')

replace("electron/main.ts", "  attachWindowLogging(window, 'main');", """  if (process.argv.includes('--nimbo-embedded')) {
    window.webContents.on('page-title-updated', event => event.preventDefault());
    window.hookWindowMessage(0x8000 + 125, () => {
      window.show();
      window.focus();
    });
    window.webContents.once('did-finish-load', async () => {
      try {
        // Never let the native host wait indefinitely for React to mount.
        // Keep the backend's actual authentication and streaming unchanged.
        const mounted = await Promise.race([
          window.webContents.executeJavaScript(`new Promise(resolve => {
            const ready = () => document.querySelector('.openstroid-login-logo,.horizon-home,.hz-connecting');
            if (ready()) { resolve(true); return; }
            const observer = new MutationObserver(() => {
              if (ready()) { observer.disconnect(); resolve(true); }
            });
            observer.observe(document.body,{subtree:true,childList:true});
            setTimeout(() => { observer.disconnect(); resolve(false); }, 18000);
          })`),
          new Promise(resolve => setTimeout(() => resolve(false), 20000)),
        ]);
        if (!mounted) throw new Error('Horizon Boosteroid renderer did not mount within 20 seconds');
        if (!window.isDestroyed()) window.setTitle('NimboPlay.Boosteroid.Ready');
      } catch (error) {
        console.error('[nimbo] Boosteroid renderer failed to initialize', error);
        if (!window.isDestroyed()) window.close();
      }
    });
    window.webContents.on('did-fail-load', (_event, code, description, url, isMainFrame) => {
      if (!isMainFrame) return;
      console.error('[nimbo] Boosteroid page load failed', code, description, url);
      if (!window.isDestroyed()) window.close();
    });
    window.webContents.on('render-process-gone', () => app.exit(1));
    window.on('unresponsive', () => app.exit(1));
  }
  attachWindowLogging(window, 'main');""")
replace("electron/main.ts", "  createMainWindow();\n  registerIpcHandlers();\n  registerAuthSessionIpc(ipcMain, authSessionStore);",
        "  registerIpcHandlers();\n  registerAuthSessionIpc(ipcMain, authSessionStore);\n  createMainWindow();")
replace("src/components/AppHeader.tsx", ">OpenStroid<", ">NimboPlay<")
replace("src/components/AppHeader.tsx", "Sign out", "Sair")
replace("src/components/AppHeader.tsx", "            Settings\n", "            Configurações\n")
replace("src/pages/LoginPage.tsx", "OpenStroid Desktop", "NimboPlay")
replace("src/pages/LoginPage.tsx", "OpenStroid logo", "NimboPlay")
replace("src/pages/LoginPage.tsx", "background: '#0b0d12'", "background: 'radial-gradient(ellipse at top right, #344661, #101725 62%, #090d15)'")
replace("src/pages/LoginPage.tsx", "Sign in with Boosteroid QR code", "Entre com sua conta Boosteroid")
replace("src/pages/LoginPage.tsx", "Scan to Sign In", "Escaneie para entrar")
replace("src/pages/LoginPage.tsx", "                      Login to Boosteroid", "                      Entrar pelo navegador")
replace("src/pages/LoginPage.tsx", "Preparing QR code", "Preparando código QR")
replace("src/pages/LoginPage.tsx", "Use your phone camera or the Boosteroid app", "Use a câmera do celular ou o aplicativo Boosteroid")
replace("src/pages/LoginPage.tsx", "Or click Login to Boosteroid to finish sign-in in your browser. Stay on this page until you are redirected.", "Você também pode entrar pelo navegador. Mantenha esta tela aberta até concluir.")
replace("src/pages/InstallPage.tsx", "Search any Boosteroid game", "Pesquisar jogos do Boosteroid")
replace("src/pages/InstallPage.tsx", "label: 'Name'", "label: 'Nome'")
replace("src/pages/InstallPage.tsx", "label: 'All'", "label: 'Todos'")
replace("src/pages/InstallPage.tsx", "label: 'Available'", "label: 'Disponíveis'")
replace("src/pages/InstallPage.tsx", "label: 'Installed'", "label: 'Instalados'")
replace("src/pages/InstallPage.tsx", "label: 'Controller'", "label: 'Controle'")
replace("src/pages/InstallPage.tsx", "label: 'Free'", "label: 'Grátis'")
replace("src/pages/SettingsPage.tsx", "OpenStroid user", "Conta Boosteroid no NimboPlay")
replace("src/pages/LoginPage.tsx", "QR code verified. OpenStroid is establishing your local session.", "Código confirmado. Conectando sua conta Boosteroid.")
replace("src/pages/LoginPage.tsx", "                      New Code", "                      Gerar novo código")
replace("src/pages/LoginPage.tsx", "src=\"/logo.png\"", "src=\"/nimbo-service.svg\"")
replace("src/pages/SettingsPage.tsx", "<h1>Settings</h1>", "<h1>Configurações</h1>")
replace("src/pages/InstallPage.tsx", "title=\"Catalog details\"", "title=\"Detalhes do jogo\"")
replace("src/pages/InstallPage.tsx", "                  Play\n", "                  Jogar\n")
replace("src/pages/InstallPage.tsx", "                Install\n", "                Instalar\n")
replace("src/pages/InstallPage.tsx", "                  Remove\n", "                  Remover\n")
copyfile(assets / "nimbo-service.svg", root / "public/nimbo-service.svg")
copyfile(assets.parents[1] / "assets/branding/nimboplay_internal.png", root / "public/nimbo-brand.png")
replace("src/components/RequireAuth.tsx", '<Center h="100vh">', '<Center className="hz-connecting" h="100vh" role="status"><img src="/nimbo-brand.png" alt="NimboPlay" /><p>Conectando ao Boosteroid…</p><button onClick={() => window.close()}>Voltar</button>')


# Native NimboPlay control-plane mode: keep the proven OpenStroid API and
# player, but do not open its separate catalog/login window.
copyfile(assets / "nimboBridge.ts", root / "electron/nimboBridge.ts")
replace("electron/main.ts",
        "import { registerAuthSessionIpc }",
        "import { startNimboBridge } from './nimboBridge.js';\nimport { registerAuthSessionIpc }")
replace("electron/main.ts", "let bridgePort = 3001;",
        """const nimboBridgeArg = process.argv.find(arg => arg.startsWith('--nimbo-bridge='));
let nimboControlServer: import('node:http').Server | null = null;
let bridgePort = 3001;""")
replace("electron/main.ts", "    show: !process.argv.includes('--nimbo-embedded'),",
        "    show: !process.argv.includes('--nimbo-embedded') && !nimboBridgeArg,")
replace("electron/main.ts", "    frame: !process.argv.includes('--nimbo-embedded'),",
        "    frame: !process.argv.includes('--nimbo-embedded') && !nimboBridgeArg,")
replace("electron/main.ts", "    skipTaskbar: process.argv.includes('--nimbo-embedded'),",
        "    skipTaskbar: process.argv.includes('--nimbo-embedded') || Boolean(nimboBridgeArg),")
replace("electron/main.ts", "  attachWindowLogging(window, 'main');",
        """  if (nimboBridgeArg) {
    // Hidden until gameplay: the NimboPlay/SDL Horizon owns all menu surfaces.
    window.webContents.on('page-title-updated', event => event.preventDefault());
    window.hookWindowMessage(0x8000 + 125, () => {
      window.show();
      window.focus();
    });
    window.webContents.on('did-finish-load', () => {
      if (window.webContents.getURL().includes('/stream')) {
        window.setTitle('NimboPlay.Boosteroid.Ready');
      }
    });
  }
  attachWindowLogging(window, 'main');""")
replace("electron/main.ts",
        "  void window.loadURL(`http://127.0.0.1:${bridgePort}`);\n}",
        "  void window.loadURL(`http://127.0.0.1:${bridgePort}`);\n  return window;\n}")
replace("electron/main.ts",
        "  createMainWindow();\n\n  app.on('activate',",
        """  if (nimboBridgeArg) {
    nimboControlServer = await startNimboBridge({
      key: nimboBridgeArg.substring('--nimbo-bridge='.length),
      backendPort: bridgePort,
      sessionStore: authSessionStore,
      launchExistingPlayer: async launch => {
        const window = createMainWindow();
        await new Promise<void>((resolve, reject) => {
          const timeout = setTimeout(() => reject(new Error('Player window did not load')), 15000);
          window.webContents.once('did-finish-load', () => {
            clearTimeout(timeout);
            resolve();
          });
          window.webContents.once('did-fail-load', (_event, code, description) => {
            clearTimeout(timeout);
            reject(new Error('Player window load failed: ' + code + ' ' + description));
          });
        });
        if (window.isDestroyed()) throw new Error('Player window closed');
        // Invoke the original authenticated Electron IPC player entry point.
        await window.webContents.executeJavaScript(
          'window.openStroid.openStream(' + JSON.stringify(launch) + ')',
        );
      },
    });
  } else {
    createMainWindow();
  }

  app.on('activate',""")
replace("electron/main.ts",
        "    if (BrowserWindow.getAllWindows().length === 0) {",
        "    if (!nimboBridgeArg && BrowserWindow.getAllWindows().length === 0) {")
replace("electron/main.ts",
        "  app.on('before-quit', () => {\n    server.close();",
        "  app.on('before-quit', () => {\n    nimboControlServer?.close();\n    server.close();")
