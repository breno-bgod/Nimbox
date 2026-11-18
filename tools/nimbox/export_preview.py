#!/usr/bin/env python3
"""Exports the approved NimboPlay HTML with native C++ pose logic compiled to WASM.

Usage: python tools/nimbox/export_preview.py approved-preview.html output.html
Requires clang++ with wasm32 and wasm-ld. This doesn't turn the full UI into C++;
it transfers the *exact* C++ pose/selection logic into the existing CSS design.
"""
from pathlib import Path
import base64
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
POSE_CPP = ROOT / 'src/ui/nimbox_pose_core.cpp'


def export(source: Path, target: Path) -> None:
    original = source.read_text(encoding='utf-8')
    # The projector dimensions are part of the native/browser visual contract.
    required_css = (
        'width:1600px;height:900px',
        'perspective:1250px;perspective-origin:50% 37%',
        'width:286px;height:389px',
    )
    for fragment in required_css:
        if fragment not in original:
            raise RuntimeError(f'HTML geometry drift: expected {fragment!r}; update C++ and preview together')
    with tempfile.TemporaryDirectory(prefix='nimbox-wasm-') as work:
        wasm = Path(work) / 'nimbox_pose.wasm'
        subprocess.run([
            'clang++', '--target=wasm32', '-O3', '-ffreestanding',
            '-fno-exceptions', '-fno-rtti', '-nostdlib',
            '-Wl,--no-entry', '-Wl,--export=nimbox_pose_value',
            '-Wl,--export=nimbox_relative_offset', '-Wl,--strip-all',
            '-I', str(ROOT / 'src'), str(POSE_CPP), '-o', str(wasm),
        ], check=True)
        payload = base64.b64encode(wasm.read_bytes()).decode('ascii')

    start = original.index('function offset(i){')
    finish = original.index('function makeCases(){', start)
    original = original[:start] + original[finish:]
    begin = original.index('function renderCases(){')
    pos = original.index('el.style.transform=', begin)
    original = original[:begin] + '''function renderCases(){if(!window.nimboxCppExports)return;const t=state.index;GAMES.forEach((g,i)=>{const el=caseNodes[i],d=window.nimboxCppExports.nimbox_relative_offset(i,t,count),a=Math.abs(d),v=window.nimboxCppExports.nimbox_pose_value(d,5)>.5;
const pose=window.nimboxCppExports.nimbox_pose_value;
const x=pose(d,0),y=pose(d,1),z=pose(d,2),rotation=pose(d,3),scale=pose(d,4);
''' + original[pos:]

    original = original.replace('el.style.zIndex=String(20-a)', 'el.style.zIndex=String(pose(d,6))')
    boot = "makeCases();renderCases();setTheme('aurora');setReduced(false);renderLibrary();renderGenres();"
    if boot not in original:
        raise RuntimeError('Approved preview boot code changed; refuse to modify unknown layout')
    wasm_boot = '''makeCases();setTheme('aurora');setReduced(false);renderLibrary();renderGenres();
const NIMBOX_CPP_WASM_BASE64 = "''' + payload + '''";
WebAssembly.instantiate(Uint8Array.from(atob(NIMBOX_CPP_WASM_BASE64),c=>c.charCodeAt(0))).then(({instance})=>{
  window.nimboxCppExports=instance.exports;
  document.documentElement.dataset.nimboxEngine='cpp-wasm';
  renderCases();
}).catch(err=>{
  document.documentElement.dataset.nimboxEngine='error';
  console.error('NimboPlay C++ preview engine not available:',err);
  const title=document.querySelector('#focus-name');
  if(title)title.textContent='Erro ao carregar motor C++ do carrossel';
});'''
    original = original.replace(boot, wasm_boot)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(original, encoding='utf-8')

if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit('Usage: export_preview.py approved-preview.html exported-preview.html')
    export(Path(sys.argv[1]), Path(sys.argv[2]))
