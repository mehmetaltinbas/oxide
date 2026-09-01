/**
 * Wire protocol shared by the client and the authoritative server. See
 * docs/systems/networking.md. Both sides import these types, so they must stay
 * free of any browser or node dependency.
 */
export type ClientMessage =
    | { t: 'hello'; version: number; name: string }
    | { t: 'rooms' }
    | { t: 'join'; roomId: string }
    | { t: 'create'; name: string; pvp: boolean }
    | { t: 'leave' }
    /** Sent only when the input actually changes, plus a keepalive. */
    | {
          t: 'input';
          seq: number;
          up: boolean;
          down: boolean;
          left: boolean;
          right: boolean;
          run: boolean;
          facing: number;
      }
    | { t: 'build'; kind: string; gx: number; gy: number; side?: 'n' | 'w' }
    | { t: 'door'; id: number; open: boolean }
    | { t: 'damage'; id: number; amount: number }
    | { t: 'chat'; text: string }
    | { t: 'ping'; at: number };
