import { ClientMessage } from 'src/features/net/types/client-message.type';
import { ServerMessage } from 'src/features/net/types/server-message.type';

export function encode(msg: ClientMessage | ServerMessage): string {
    return JSON.stringify(msg);
}
