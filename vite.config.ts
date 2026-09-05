import { defineConfig } from 'vite';
import { fileURLToPath } from 'node:url';

/**
 * Imports are absolute and rooted at `src/`, per docs/architecture/code-conventions.md.
 * The bundler needs the same alias TypeScript resolves through `paths`.
 */
export default defineConfig({
    resolve: {
        alias: {
            src: fileURLToPath(new URL('./src', import.meta.url)),
            // Numbers the client and the server both need. One file, imported
            // by both, so they cannot drift apart.
            shared: fileURLToPath(new URL('./shared', import.meta.url)),
        },
    },
});
