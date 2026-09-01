import { TAU } from 'src/shared/constants/tau.constant';
import { FloatingText } from 'src/shared/types/floating-text.interface';
import { Particle } from 'src/shared/types/particle.interface';
import { randRange } from 'src/shared/utils/rand-range.util';

const MAX_PARTICLES = 900;

export class Particles {
    items: Particle[] = [];
    texts: FloatingText[] = [];

    private add(p: Particle): void {
        if (this.items.length >= MAX_PARTICLES) this.items.shift();
        this.items.push(p);
    }

    burst(
        x: number,
        y: number,
        count: number,
        color: string,
        opts: {
            speed?: number;
            life?: number;
            size?: number;
            gravity?: number;
            dir?: number;
            spread?: number;
        } = {},
    ): void {
        const speed = opts.speed ?? 120;
        const life = opts.life ?? 0.5;
        const size = opts.size ?? 3;
        const gravity = opts.gravity ?? 0;
        const spread = opts.spread ?? TAU;
        const baseDir = opts.dir ?? 0;
        for (let i = 0; i < count; i++) {
            const a =
                opts.dir === undefined
                    ? Math.random() * TAU
                    : baseDir + randRange(Math.random, -spread / 2, spread / 2);
            const s = speed * (0.4 + Math.random() * 0.9);
            const l = life * (0.6 + Math.random() * 0.8);
            this.add({
                x,
                y,
                vx: Math.cos(a) * s,
                vy: Math.sin(a) * s,
                life: l,
                maxLife: l,
                size: size * (0.6 + Math.random() * 0.8),
                color,
                gravity,
            });
        }
    }

    ember(x: number, y: number): void {
        this.add({
            x: x + randRange(Math.random, -6, 6),
            y: y + randRange(Math.random, -4, 4),
            vx: randRange(Math.random, -12, 12),
            vy: randRange(Math.random, -46, -22),
            life: 0.9,
            maxLife: 0.9,
            size: randRange(Math.random, 1.2, 2.6),
            color: Math.random() < 0.5 ? '#ffb347' : '#ff7b2e',
            gravity: -8,
        });
    }

    text(x: number, y: number, text: string, color = '#f0e6d0'): void {
        if (this.texts.length > 60) this.texts.shift();
        this.texts.push({ x, y, text, color, life: 1.0, vy: -34 });
    }

    update(dt: number): void {
        for (let i = this.items.length - 1; i >= 0; i--) {
            const p = this.items[i];
            p.life -= dt;
            if (p.life <= 0) {
                this.items.splice(i, 1);
                continue;
            }
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.vy += p.gravity * dt;
            p.vx *= 1 - 2.2 * dt;
            p.vy *= 1 - 2.2 * dt;
        }
        for (let i = this.texts.length - 1; i >= 0; i--) {
            const t = this.texts[i];
            t.life -= dt;
            if (t.life <= 0) {
                this.texts.splice(i, 1);
                continue;
            }
            t.y += t.vy * dt;
            t.vy *= 1 - 1.5 * dt;
        }
    }

    clear(): void {
        this.items.length = 0;
        this.texts.length = 0;
    }
}
