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
        },
    },
});
