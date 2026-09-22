import { ItemId } from 'src/features/items/types/item-id.type';
import { GunSound } from 'src/shared/types/gun-sound.interface';

/**
 * What each firearm sounds like, loosely from what the real thing does:
 *
 * - The bow has no report at all, only the string: a low twang and a hiss.
 * - The revolver is a sharp, bright crack with a short low end.
 * - The semi-auto is heavier: a hard crack, a deep thump, a long roll-off.
 * - The AK is punchier and shorter, so it stays clean firing seven a second.
 * - The shotguns are low booms with little crack; the pump racks after each.
 * - The launcher is not a bang but a rushing whoosh as the motor lights.
 */
export const GUN_SOUNDS: Partial<Record<ItemId, GunSound>> = {
    bow: {
        crack: { hz: 2400, dur: 0.12, peak: 0.12, filter: 'highpass' },
        thump: { from: 190, to: 95, dur: 0.16, peak: 0.28, wave: 'triangle' },
    },
    revolver: {
        crack: { hz: 2600, dur: 0.12, peak: 0.55, filter: 'bandpass' },
        thump: { from: 150, to: 60, dur: 0.14, peak: 0.35, wave: 'square' },
        tail: { hz: 900, dur: 0.35, peak: 0.18 },
    },
    rifle: {
        crack: { hz: 3200, dur: 0.1, peak: 0.6, filter: 'bandpass' },
        thump: { from: 115, to: 45, dur: 0.2, peak: 0.45, wave: 'square' },
        tail: { hz: 700, dur: 0.55, peak: 0.24 },
    },
    ak47: {
        crack: { hz: 2800, dur: 0.07, peak: 0.55, filter: 'bandpass' },
        thump: { from: 100, to: 45, dur: 0.12, peak: 0.45, wave: 'square' },
        tail: { hz: 650, dur: 0.28, peak: 0.18 },
    },
    waterpipe: {
        crack: { hz: 1300, dur: 0.32, peak: 0.7, filter: 'lowpass' },
        thump: { from: 75, to: 30, dur: 0.36, peak: 0.6, wave: 'sine' },
        tail: { hz: 500, dur: 0.7, peak: 0.28 },
    },
    pump_shotgun: {
        crack: { hz: 1600, dur: 0.28, peak: 0.68, filter: 'lowpass' },
        thump: { from: 85, to: 35, dur: 0.3, peak: 0.55, wave: 'sine' },
        tail: { hz: 550, dur: 0.6, peak: 0.25 },
        // Rack back, rack forward.
        mech: [0.38, 0.52],
    },
    rocket_launcher: {
        crack: { hz: 700, dur: 0.75, peak: 0.5, filter: 'bandpass' },
        thump: { from: 60, to: 38, dur: 0.35, peak: 0.45, wave: 'sine' },
        tail: { hz: 400, dur: 0.9, peak: 0.2 },
    },
};
