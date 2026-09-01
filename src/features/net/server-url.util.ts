import { DEFAULT_SERVER_PORT } from 'src/features/net/constants/default-server-port.constant';

/**
 * Where to look for the game server.
 *
 * Three answers, most specific first:
 *
 * 1. `?server=` in the address bar, so a room can be shared as a link and so a
 *    server can be tested without rebuilding anything.
 * 2. `VITE_SERVER_URL`, baked in at build time. This is the one that matters
 *    for a real deploy: the page is served as static files from one host and
 *    the server runs on another, so the client cannot work the address out for
 *    itself the way it can on one box.
 * 3. The same host on the server's own port, which is what a local dev run
 *    gives you and what a single-machine deploy wants.
 *
 * Never a secret. This is a public address that ships inside the bundle, and
 * anyone who can play the game can read it.
 */
export function defaultServerUrl(): string {
    const override = new URLSearchParams(location.search).get('server');
    if (override) return override;

    const configured = import.meta.env.VITE_SERVER_URL;
    if (configured) return configured;

    const secure = location.protocol === 'https:';
    const scheme = secure ? 'wss' : 'ws';
    const host = location.hostname || 'localhost';
    return `${scheme}://${host}:${DEFAULT_SERVER_PORT}`;
}
