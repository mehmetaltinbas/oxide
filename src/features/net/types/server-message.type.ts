import { NetDeployable } from 'src/features/net/types/net-deployable.interface';
import { NetPlayer } from 'src/features/net/types/net-player.interface';
import { NetStructure } from 'src/features/net/types/net-structure.interface';
import { RoomSummary } from 'src/features/net/types/room-summary.interface';

export type ServerMessage =
    | { t: 'welcome'; version: number; you: string }
    | { t: 'rooms'; rooms: RoomSummary[] }
    | { t: 'joined'; roomId: string; seed: number; you: string; tick: number }
    | { t: 'left' }
    | { t: 'error'; message: string }
    /** Full snapshot on join, then deltas every tick. */
    | {
          t: 'snapshot';
          tick: number;
          players: NetPlayer[];
          structures: NetStructure[];
          deployables: NetDeployable[];
      }
    | { t: 'state'; tick: number; players: NetPlayer[]; joined?: NetPlayer[]; left?: string[] }
    | { t: 'built'; structure: NetStructure }
    | { t: 'deployed'; deployable: NetDeployable }
    /**
     * A placement the server would not accept. The client puts a piece down the
     * moment you click, so it needs telling to take that one back.
     */
    | {
          t: 'refused';
          what: 'build';
          kind: string;
          gx: number;
          gy: number;
          side?: 'n' | 'w';
          reason: string;
      }
    | { t: 'destroyed'; id: number }
    | { t: 'door'; id: number; open: boolean }
    | { t: 'chat'; from: string; text: string }
    /** Someone has asked you to team up. It lapses on its own. */
    | { t: 'invited'; from: string; fromName: string }
    | { t: 'teamed'; message: string }
    | { t: 'pong'; at: number };
