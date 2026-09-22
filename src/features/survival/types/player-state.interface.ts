import { Container } from 'src/features/items/types/container.interface';
import { ItemId } from 'src/features/items/types/item-id.type';

export interface PlayerState {
    x: number;
    y: number;
    vx: number;
    vy: number;
    facing: number;
    health: number;
    calories: number;
    hydration: number;
    temperature: number;
    radiation: number;
    alive: boolean;
    respawnTimer: number;
    invuln: number;
    hurtFlash: number;
    attackTimer: number;
    swingAnim: number;
    /** Seconds the bow has been drawn for, up to BOW_DRAW_SECONDS. 0 when it is not. */
    bowDraw: number;
    walkPhase: number;
    /** Hotbar index currently held. */
    activeSlot: number;
    inventory: Container;
    hotbar: Container;
    /**
     * What you have on, as a one-slot container.
     *
     * A container rather than a bare id so the inventory screen can treat it
     * like every other slot: drag something in to put it on, drag it out to
     * take it off. Read it through `wornId`.
     */
    worn: Container;
    /** Healing over time from a syringe. */
    healOverTime: number;
    bleeding: number;
    /** Applying something that takes time: what, and how long is left. */
    using: ItemId | null;
    useLeft: number;
    useTotal: number;
    /** Seconds left on a magazine change, and what it started at. */
    reloadLeft: number;
    reloadTotal: number;
}
