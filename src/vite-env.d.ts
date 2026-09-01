/// <reference types="vite/client" />

/**
 * Build-time settings, supplied by the host at build time.
 *
 * Only `VITE_`-prefixed names reach the browser bundle, and everything here is
 * baked into the shipped JavaScript: this is configuration, never a secret.
 */
interface ImportMetaEnv {
    /**
     * Where the game server lives, as a full `wss://host` URL.
     *
     * Set this on the static host that serves the page. Left unset, the client
     * falls back to the same host on the server's own port, which is what a
     * local dev run gives you and what a single-box deploy wants.
     */
    readonly VITE_SERVER_URL?: string;
}

interface ImportMeta {
    readonly env: ImportMetaEnv;
}
