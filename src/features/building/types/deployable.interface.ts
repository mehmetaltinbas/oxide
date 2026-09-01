import { DeployableKind } from 'src/features/building/types/deployable-kind.type';
import { Container } from 'src/features/items/types/container.interface';

export interface Deployable {
    id: number;
    kind: DeployableKind;
    x: number;
    y: number;
    hp: number;
    maxHp: number;
    owner: number;
    container?: Container;
    /** Fires and furnaces. */
    lit?: boolean;
    fuel?: number;
    /** Smelting or cooking progress in seconds. */
    progress?: number;
    flash: number;
}
