import { useEffect, useState } from 'react';
import { getInstalledGames, launchStream } from '../api';
import { imageUrl, uniqueGames, coerceGame, describeLaunchError } from '../lib/gameUtils';
import type { InstalledGame } from '../types';
import './horizon.css';

export function HorizonHome() {
  const [games, setGames] = useState<InstalledGame[]>([]);
  const [index, select] = useState(0);
  const [status, setStatus] = useState('Carregando seus jogos…');
  const [busy, setBusy] = useState(false);
  const [reload, refresh] = useState(0);
  const game = games[index];
  useEffect(() => {
    let active = true;
    getInstalledGames().then(items => {
      if (!active) return;
      const library = uniqueGames(items.map(coerceGame));
      setGames(library);
      select(Math.max(0,library.findIndex(item => String(item.id) === sessionStorage.getItem('nimbo:selection'))));
      setStatus(library.length ? '' : 'Sua biblioteca está vazia.');
    }).catch(() => { if (active) setStatus('Não foi possível carregar sua biblioteca.'); });
    return () => { active = false; };
  }, [reload]);
  useEffect(() => {
    if (game) sessionStorage.setItem('nimbo:backdrop', imageUrl(game));
  }, [game]);
  async function play() {
    if (!game || busy) return;
    setBusy(true);
    try {
      const launch = await launchStream(game.id);
      sessionStorage.setItem('nimbo:selection',String(game.id));
      sessionStorage.setItem('openstroid:lastLaunch',JSON.stringify(launch));
      if (window.openStroid?.openStream) await window.openStroid.openStream(launch);
      else window.location.assign('/stream');
    } catch (error) { setStatus(describeLaunchError(error,game.name)); setBusy(false); }
  }
  useEffect(() => {
    const move = (delta: number) => select(i => Math.max(0,Math.min(games.length-1,i+delta)));
    function key(e: KeyboardEvent) {
      if (document.querySelector('[role="dialog"], [role="menu"]') || (e.target instanceof Element && e.target.closest('.hz-top'))) return;
      if (e.key === 'ArrowRight' || e.key === 'ArrowLeft') { e.preventDefault(); move(e.key === 'ArrowRight' ? 1 : -1); }
      else if (e.key === 'Escape') window.close();
      else if (e.key === 'Enter' && document.activeElement === document.body) void play();
    }
    let previous = [true,true,true,true];
    const timer = window.setInterval(() => {
      const pad = Array.from(navigator.getGamepads()).find(Boolean);
      if (!pad || !document.hasFocus() || document.querySelector('[role="dialog"], [role="menu"]')) return;
      const inputs = [!!pad.buttons[14]?.pressed || pad.axes[0] < -.6, !!pad.buttons[15]?.pressed || pad.axes[0] > .6,
                      !!pad.buttons[0]?.pressed, !!pad.buttons[1]?.pressed];
      if (inputs[0] && !previous[0]) move(-1);
      if (inputs[1] && !previous[1]) move(1);
      if (inputs[2] && !previous[2]) void play();
      if (inputs[3] && !previous[3]) window.close();
      previous = inputs;
    },80);
    window.addEventListener('keydown',key);
    return () => { clearInterval(timer); window.removeEventListener('keydown',key); };
  }, [game,busy,games.length]);
  useEffect(() => { document.querySelector('.horizon-rail [aria-current="true"]')?.scrollIntoView({block:'nearest',inline:'center'}); },[index,games]);
  return <main className="horizon-home">
    {game && <img key={game.id} className="horizon-art" src={imageUrl(game)} alt="" />}
    <section><small>Meus jogos</small><h1>{game?.name || 'Boosteroid'}</h1>
    {game && <button disabled={busy} onClick={play}>{busy ? 'Iniciando…' : 'Jogar'}</button>}
    {status && <p role="status">{status} <button onClick={() => refresh(i=>i+1)}>Atualizar</button></p>}</section>
    <nav className="horizon-rail" aria-label="Meus jogos">{games.map((item,i) =>
      <button key={item.id} aria-current={i===index} aria-label={item.name} onClick={() => select(i)}>
        <img loading="lazy" src={imageUrl(item)} alt="" /><span>{item.name}</span></button>)}</nav>
  </main>;
}
