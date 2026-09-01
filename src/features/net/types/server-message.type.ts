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
    | { t: 'destroyed'; id: number }
    | { t: 'door'; id: number; open: boolean }
    | { t: 'chat'; from: string; text: string }
    | { t: 'pong'; at: number };
