/**
 * How a firearm sounds, built from a few layers of synthesized noise and tone.
 * Every gun is described by the same parts, so a new gun is a new row of
 * numbers rather than a new sound routine.
 */
export interface GunSound {
    /** The crack: the sharp front of the report. */
    crack: { hz: number; dur: number; peak: number; filter: BiquadFilterType };
    /** The thump: the low body of it, a tone sliding down. */
    thump: { from: number; to: number; dur: number; peak: number; wave: OscillatorType };
    /** The tail: the report rolling off, a long quiet low rumble. */
    tail?: { hz: number; dur: number; peak: number };
    /** Mechanical clicks after the shot, as offsets in seconds: a pump racking. */
    mech?: number[];
}
