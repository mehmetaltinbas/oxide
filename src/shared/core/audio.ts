import { AUDIO_DEFAULT_LEVEL } from 'src/shared/design/constants/audio-default-level.constant';
import { GunSound } from 'src/shared/types/gun-sound.interface';
import { SOUND_HEARING } from 'src/shared/constants/sound-hearing.constant';
/** Tiny procedural sound bank, no asset files, everything is synthesized on the fly. */
export class Audio {
    private ctx: AudioContext | null = null;
    private master: GainNode | null = null;
    enabled = true;

    /**
     * Master level, 0 to 1, as the settings panel sets it.
     *
     * Kept as a field rather than read off the gain node, because the node does
     * not exist until the first user gesture unlocks audio and the setting has
     * to survive being changed before then.
     */
    private level = AUDIO_DEFAULT_LEVEL;

    /** Where the ears are: the player. Set each frame. */
    private listenerX = 0;
    private listenerY = 0;
    /** Loudness for the sound being played right now, from how far away it is. */
    private scale = 1;

    listenAt(x: number, y: number): void {
        this.listenerX = x;
        this.listenerY = y;
    }

    /**
     * Play a sound as if it happened at (x, y): full volume on top of you,
     * fading with distance, and not at all out of earshot. Everything that
     * happens somewhere in the world goes through here; only what is yours
     * (your own hands, the interface) plays flat.
     */
    from(x: number, y: number, play: () => void, range: number = SOUND_HEARING.range): void {
        const d = Math.hypot(x - this.listenerX, y - this.listenerY);
        const near = 1 - d / range;
        if (near <= 0.02) return;
        const was = this.scale;
        this.scale = Math.pow(near, SOUND_HEARING.falloff);
        play();
        this.scale = was;
    }

    /** Browsers block audio until a user gesture, so this is called on first input. */
    unlock(): void {
        if (this.ctx) {
            if (this.ctx.state === 'suspended') void this.ctx.resume();
            return;
        }
        const Ctor =
            window.AudioContext ??
            (window as unknown as { webkitAudioContext: typeof AudioContext }).webkitAudioContext;
        if (!Ctor) return;
        this.ctx = new Ctor();
        this.master = this.ctx.createGain();
        this.master.gain.value = this.enabled ? this.level : 0;
        this.master.connect(this.ctx.destination);
    }

    /** Current master level, 0 to 1. */
    get volume(): number {
        return this.level;
    }

    setVolume(value: number): void {
        this.level = Math.max(0, Math.min(1, value));
        this.applyLevel();
    }

    setEnabled(on: boolean): void {
        this.enabled = on;
        this.applyLevel();
    }

    private applyLevel(): void {
        if (this.master) this.master.gain.value = this.enabled ? this.level : 0;
    }

    private env(dur: number, peak0: number, at = 0): GainNode | null {
        if (!this.ctx || !this.master || !this.enabled) return null;
        const peak = Math.max(0.0002, peak0 * this.scale);
        const g = this.ctx.createGain();
        const t = this.ctx.currentTime + at;
        g.gain.setValueAtTime(0.0001, t);
        g.gain.exponentialRampToValueAtTime(peak, t + 0.008);
        g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
        g.connect(this.master);
        return g;
    }

    private tone(
        freq: number,
        dur: number,
        type: OscillatorType,
        peak = 0.5,
        slideTo?: number,
        at = 0,
    ): void {
        const g = this.env(dur, peak, at);
        if (!g || !this.ctx) return;
        const o = this.ctx.createOscillator();
        o.type = type;
        const t = this.ctx.currentTime + at;
        o.frequency.setValueAtTime(freq, t);
        if (slideTo !== undefined)
            o.frequency.exponentialRampToValueAtTime(Math.max(20, slideTo), t + dur);
        o.connect(g);
        o.start(t);
        o.stop(t + dur + 0.02);
    }

    private noise(
        dur: number,
        peak: number,
        filterHz: number,
        filterType: BiquadFilterType = 'lowpass',
        at = 0,
    ): void {
        const g = this.env(dur, peak, at);
        if (!g || !this.ctx) return;
        const frames = Math.floor(this.ctx.sampleRate * dur);
        const buf = this.ctx.createBuffer(1, Math.max(1, frames), this.ctx.sampleRate);
        const data = buf.getChannelData(0);
        for (let i = 0; i < data.length; i++) data[i] = Math.random() * 2 - 1;
        const src = this.ctx.createBufferSource();
        src.buffer = buf;
        const f = this.ctx.createBiquadFilter();
        f.type = filterType;
        f.frequency.value = filterHz;
        src.connect(f);
        f.connect(g);
        src.start(this.ctx.currentTime + at);
    }

    /** A firearm going off, from its own description. See GunSound. */
    gunshot(s: GunSound, loudness = 1): void {
        if (loudness <= 0.01) return;
        const k = loudness;
        this.noise(s.crack.dur, s.crack.peak * k, s.crack.hz, s.crack.filter);
        this.tone(s.thump.from, s.thump.dur, s.thump.wave, s.thump.peak * k, s.thump.to);
        if (s.tail) this.noise(s.tail.dur, s.tail.peak * k, s.tail.hz, 'lowpass');
        for (const at of s.mech ?? []) this.noise(0.035, 0.22 * k, 3200, 'highpass', at);
    }

    // ---- material-specific impacts. Each one has its own body so you can tell
    // what you are hitting with your eyes shut.
    /** Axe into a trunk: a dull woody thock. */
    chopWood(): void {
        this.noise(0.11, 0.4, 900);
        this.tone(150, 0.11, 'triangle', 0.3, 78);
        this.tone(320, 0.05, 'sine', 0.12, 210);
    }

    /** Pick into rock: a bright sharp crack with grit. */
    hitStone(): void {
        this.noise(0.07, 0.5, 4200, 'bandpass');
        this.tone(760, 0.05, 'square', 0.13, 420);
        this.noise(0.16, 0.18, 1800);
    }

    /** Metal ore: the crack plus a ring. */
    hitMetal(): void {
        this.noise(0.06, 0.45, 5200, 'bandpass');
        this.tone(1180, 0.22, 'sine', 0.16, 880);
        this.tone(1760, 0.16, 'sine', 0.08, 1500);
    }

    /** Sulfur: duller and more crumbly than metal. */
    hitSulfur(): void {
        this.noise(0.09, 0.45, 2600, 'bandpass');
        this.tone(520, 0.08, 'triangle', 0.14, 300);
    }

    /** Flesh: wet, low, no ring at all. */
    hitFlesh(): void {
        this.noise(0.09, 0.45, 700);
        this.tone(190, 0.09, 'sawtooth', 0.2, 96);
    }

    /** Timber or planks in a wall. */
    hitStructureWood(): void {
        this.noise(0.13, 0.42, 1100);
        this.tone(130, 0.14, 'triangle', 0.26, 70);
    }

    /** Sheet metal in a wall: loud, ringing, unmistakable. */
    hitStructureMetal(): void {
        this.noise(0.05, 0.4, 6000, 'bandpass');
        this.tone(880, 0.3, 'sine', 0.2, 620);
        this.tone(1320, 0.22, 'triangle', 0.1, 990);
    }

    /** Picking a plant. */
    pluck(): void {
        this.noise(0.09, 0.28, 3000, 'bandpass');
        this.tone(620, 0.06, 'triangle', 0.1, 900);
    }

    chop(): void {
        this.chopWood();
    }
    treeFall(): void {
        this.noise(0.7, 0.6, 700);
        this.tone(90, 0.6, 'sine', 0.3, 40);
    }
    mine(): void {
        this.noise(0.1, 0.45, 3000, 'bandpass');
    }
    hit(): void {
        this.noise(0.09, 0.5, 2200, 'bandpass');
        this.tone(300, 0.07, 'square', 0.15, 140);
    }
    playerHurt(): void {
        this.tone(240, 0.22, 'sawtooth', 0.35, 110);
    }
    enemyDie(): void {
        this.tone(300, 0.3, 'sawtooth', 0.3, 70);
        this.noise(0.25, 0.3, 900);
    }
    build(): void {
        this.tone(440, 0.1, 'square', 0.25);
        this.tone(660, 0.12, 'square', 0.2);
    }
    craft(): void {
        this.tone(523, 0.09, 'triangle', 0.3);
        this.tone(784, 0.14, 'triangle', 0.25);
    }
    deny(): void {
        this.tone(160, 0.14, 'square', 0.25, 110);
    }
    pickup(): void {
        this.tone(880, 0.07, 'triangle', 0.18);
    }
    waveStart(): void {
        this.tone(110, 1.1, 'sawtooth', 0.4, 220);
        this.noise(1.0, 0.2, 400);
    }
    waveClear(): void {
        [523, 659, 784, 1047].forEach((f, i) =>
            setTimeout(() => this.tone(f, 0.25, 'triangle', 0.3), i * 110),
        );
    }
    howl(): void {
        this.tone(320, 0.8, 'sawtooth', 0.22, 520);
    }
    roar(): void {
        this.tone(90, 0.9, 'sawtooth', 0.35, 55);
        this.noise(0.8, 0.25, 500);
    }
    gameOver(): void {
        [392, 330, 262, 196].forEach((f, i) =>
            setTimeout(() => this.tone(f, 0.5, 'sine', 0.35), i * 220),
        );
    }
}
