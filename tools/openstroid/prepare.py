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
        // Wait for React to mount the login page or the signed-in home.
        await window.webContents.executeJavaScript(`new Promise(resolve => {
          const mounted = () => document.querySelector('.openstroid-login-logo,.horizon-home,.hz-connecting');
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
