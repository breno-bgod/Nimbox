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
