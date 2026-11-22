import { useEffect } from 'react';
import { NavLink, Outlet, useLocation, useNavigate } from 'react-router-dom';
import { AppHeader } from '../components/AppHeader';

export function AuthenticatedLayout() {
  const { pathname } = useLocation();
  const navigate = useNavigate();
  const home = pathname === '/my-games';
  useEffect(() => {
    function back(event: KeyboardEvent) {
      if (event.key === 'Escape' && !event.defaultPrevented && !document.querySelector('[role="dialog"],[role="menu"],[aria-expanded="true"]') && !home) {
        navigate('/my-games');
      }
    }
    window.addEventListener('keydown', back);
    return () => window.removeEventListener('keydown', back);
  }, [home, navigate]);
  const cover = sessionStorage.getItem('nimbo:backdrop');
  return <div className="hz-shell">
    {!home && cover && <img src={cover} className="hz-backdrop" alt="" />}
    <header className="hz-top"><strong>NimboPlay</strong>
      <nav aria-label="Boosteroid">
        <NavLink to="/my-games">Boosteroid</NavLink>
        <NavLink to="/library">Catálogo</NavLink>
        <NavLink to="/install">Instalar</NavLink>
        <NavLink to="/settings" state={{backgroundPath: pathname}}>Configurações</NavLink>
      </nav>
      <AppHeader /><button onClick={() => window.close()}>Trocar serviço</button>
    </header>
    <div className={home ? 'hz-front' : 'hz-page'}>
      {!home && <h1>{pathname === '/install' ? 'Instalar jogos' : 'Catálogo'}</h1>}
      <Outlet />
    </div>
  </div>;
}
