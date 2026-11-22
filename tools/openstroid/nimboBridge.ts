// NimboPlay-only local control plane for the existing OpenStroid backend.
// No streaming, authentication, token exchange, or Boosteroid API logic is
// reimplemented here. All sensitive session data stays in this process.
import { createServer, type Server } from 'node:http';
import { timingSafeEqual } from 'node:crypto';

type JsonRecord = Record<string, unknown>;
type SessionStore = {
  read(): Promise<string | null>;
  write(value: string | null): Promise<unknown>;
};

interface NimboBridgeOptions {
  key: string;
  backendPort: number;
  sessionStore: SessionStore;
  launchExistingPlayer(payload: JsonRecord): Promise<void>;
}

const PORT = 3037;
const MAX_JSON_BYTES = 4096;

function record(value: unknown): JsonRecord {
  return value !== null && typeof value === 'object' && !Array.isArray(value)
    ? value as JsonRecord : {};
}

function firstString(value: unknown, keys: string[]): string {
  const data = record(value);
  for (const key of keys) {
    if (typeof data[key] === 'string') return String(data[key]);
  }
  return '';
}

function gameInfo(value: unknown): JsonRecord {
  const raw = record(value);
  const details = ['application', 'app', 'game', 'item']
    .map(key => record(raw[key]))
    .find(item => Object.keys(item).length > 0) ?? raw;
  const id = Number(details.id ?? details.appId ?? details.applicationId ??
    details.gameId ?? raw.id ?? raw.appId);
  if (!Number.isSafeInteger(id) || id <= 0) return {};
  return {
    id,
    name: firstString(details, ['name', 'title', 'displayName'])
      || firstString(raw, ['name', 'title', 'displayName']),
    publisher: firstString(details, ['publisher', 'developer']),
    description: firstString(details, ['description', 'summary']),
    coverUrl: firstString(details,
      ['cover', 'coverUrl', 'imageUrl', 'image', 'poster', 'thumbnailUrl', 'thumbnail']),
    heroUrl: firstString(details, ['heroUrl', 'backgroundUrl', 'bannerUrl']),
  };
}

function publicGames(value: unknown): JsonRecord[] {
  const data = record(value);
  const games = Array.isArray(value) ? value : data.games;
  if (!Array.isArray(games)) return [];
  const ids = new Set<number>();
  const safe: JsonRecord[] = [];
  for (const item of games) {
    const game = gameInfo(item);
    const id = Number(game.id);
    if (!id || ids.has(id)) continue;
    ids.add(id);
    safe.push(game);
    if (safe.length >= 5000) break;
  }
  return safe;
}

function equalKey(a: string, b: string): boolean {
  const x = Buffer.from(a, 'utf8');
  const y = Buffer.from(b, 'utf8');
  return x.length === y.length && timingSafeEqual(x, y);
}

export async function startNimboBridge(options: NimboBridgeOptions): Promise<Server> {
  if (!/^[a-f0-9]{64}$/i.test(options.key)) throw new Error('Invalid NimboPlay bridge key');
  let inMemorySession: string | null = null;
  let pendingQrId: string | null = null;
  let launchPending = false;
  let launchError = '';

  const backend = async (
    method: 'GET' | 'POST', path: string, body?: JsonRecord, timeoutMs = 12000,
  ): Promise<{ status: number; json: JsonRecord }> => {
    const handoff = inMemorySession || await options.sessionStore.read();
    const response = await fetch('http://127.0.0.1:' + options.backendPort + path, {
      method,
      headers: {
        'content-type': 'application/json',
        ...(handoff ? { 'x-openstroid-session': handoff } : {}),
      },
      ...(body ? { body: JSON.stringify(body) } : {}),
      signal: AbortSignal.timeout(timeoutMs),
    });
    const data = record(await response.json());
    return { status: response.status, json: data };
  };

  const send = (res: import('node:http').ServerResponse, status: number, payload: JsonRecord) => {
    const body = JSON.stringify(payload);
    res.writeHead(status, {
      'Content-Type': 'application/json; charset=utf-8',
      'Cache-Control': 'no-store',
      'Content-Length': Buffer.byteLength(body),
      'X-Content-Type-Options': 'nosniff',
    });
    res.end(body);
  };

  const server = createServer(async (req, res) => {
    const remote = req.socket.remoteAddress;
    // The bridge is loopback-only and bound to a per-process secret; never
    // expose handoff data, cookies, credentials or stream-launch payloads.
    if (remote !== '127.0.0.1' && remote !== '::ffff:127.0.0.1') {
      send(res, 403, { error: 'Not allowed' });
      return;
    }
    const key = String(req.headers['x-nimboplay-bridge'] ?? '');
    if (!equalKey(key, options.key)) {
      send(res, 403, { error: 'Not allowed' });
      return;
    }
    const url = new URL(req.url || '/', 'http://127.0.0.1');
    const method = req.method ?? 'GET';

    try {
      if (method === 'GET' && url.pathname === '/health') {
        send(res, 200, { ok: true, provider: 'boosteroid' });
        return;
      }
      if (method === 'GET' && url.pathname === '/status') {
        const state = await backend('GET', '/auth/session');
        const authenticated = state.status === 200 && state.json.authenticated === true;
        const user = record(state.json.user);
        send(res, 200, {
          authenticated,
          name: authenticated ? firstString(user, ['name', 'displayName', 'email']) : '',
          launching: launchPending,
          error: launchError,
        });
        return;
      }
      if (method === 'POST' && url.pathname === '/login/start') {
        const qr = await backend('POST', '/auth/login/qr/start');
        if (qr.status < 200 || qr.status >= 300) {
          send(res, 502, { error: 'Falha ao iniciar o login Boosteroid' });
          return;
        }
        pendingQrId = typeof qr.json.id === 'string' ? qr.json.id : null;
        send(res, 200, {
          validationUrl: firstString(qr.json, ['validationUrl']),
          status: firstString(qr.json, ['status']),
        });
        return;
      }
      if (method === 'GET' && url.pathname === '/login/status') {
        if (!pendingQrId) {
          send(res, 200, { authenticated: false, status: 'idle' });
          return;
        }
        const qr = await backend('GET',
          '/auth/login/qr/status/' + encodeURIComponent(pendingQrId));
        const status = firstString(qr.json, ['status']);
        const handoff = firstString(qr.json, ['sessionHandoff']);
        if (status === 'succeeded' && handoff) {
          inMemorySession = handoff;
          await options.sessionStore.write(handoff);
          pendingQrId = null;
        }
        send(res, 200, {
          authenticated: status === 'succeeded' && !!handoff,
          status,
        });
        return;
      }
      if (method === 'GET' && url.pathname === '/catalog') {
        const [installed, catalog] = await Promise.all([
          backend('GET', '/library/installed', undefined, 25000),
          backend('GET', '/library/catalog', undefined, 25000),
        ]);
        if (installed.status !== 200 || catalog.status !== 200) {
          send(res, 502, { error: 'Não foi possível carregar o catálogo Boosteroid' });
          return;
        }
        send(res, 200, {
          installed: publicGames(installed.json),
          catalog: publicGames(catalog.json),
        });
        return;
      }
      if (method === 'POST' && url.pathname === '/play') {
        if (launchPending) {
          send(res, 409, { error: 'Um jogo já está iniciando' });
          return;
        }
        let data = '';
        for await (const chunk of req) {
          data += String(chunk);
          if (data.length > MAX_JSON_BYTES) break;
        }
        const parsed = data.length <= MAX_JSON_BYTES ? record(JSON.parse(data)) : {};
        const appId = Number(parsed.appId);
        if (!Number.isSafeInteger(appId) || appId <= 0) {
          send(res, 400, { error: 'Jogo inválido' });
          return;
        }
        launchPending = true;
        launchError = '';
        // Respond immediately: the native Horizon remains responsive while
        // the original backend starts its potentially long streaming session.
        send(res, 202, { accepted: true });
        void (async () => {
          try {
            const launch = await backend('POST', '/api/stream/launch', { appId }, 190000);
            if (launch.status < 200 || launch.status >= 300) {
              throw new Error('O Boosteroid não conseguiu iniciar o jogo');
            }
            await options.launchExistingPlayer(launch.json);
          } catch (error) {
            console.error('[nimbo] stream launch failed', error);
            launchError = 'Não foi possível iniciar o jogo. Tente novamente.';
          } finally {
            launchPending = false;
          }
        })();
        return;
      }
      send(res, 404, { error: 'Comando não encontrado' });
    } catch (error) {
      console.error('[nimbo] control request failed', url.pathname, error);
      send(res, 503, { error: 'Serviço Boosteroid indisponível' });
    }
  });
  server.requestTimeout = 15000;
  await new Promise<void>((resolve, reject) => {
    server.once('error', reject);
    server.listen(PORT, '127.0.0.1', () => {
      server.removeListener('error', reject);
      resolve();
    });
  });
  console.log('[nimbo] local Boosteroid control plane ready');
  return server;
}
