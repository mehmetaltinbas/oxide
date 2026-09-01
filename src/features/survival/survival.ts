import { wornId } from 'src/features/items/utils/worn-id.util';
import { BuildSystem } from 'src/features/building/building';
import { ITEMS } from 'src/features/items/constants/items.constant';
import { ItemId } from 'src/features/items/types/item-id.type';
import { addAcross } from 'src/features/items/utils/add-across.util';
import { countAcross } from 'src/features/items/utils/count-across.util';
import { removeAcross } from 'src/features/items/utils/remove-across.util';
import { FIRE_WARMTH_RADIUS } from 'src/features/survival/constants/fire-warmth-radius.constant';
import { FIRE_WARMTH } from 'src/features/survival/constants/fire-warmth.constant';
import { PLAYER } from 'src/features/survival/constants/player.constant';
import { SWIM } from 'src/features/survival/constants/swim.constant';
import { SurvivalHooks } from 'src/features/survival/types/survival-hooks.interface';
import { BIOME_TEMP } from 'src/features/world/constants/biome-temp.constant';
import { World } from 'src/features/world/world';
import { Audio } from 'src/shared/core/audio';
import { Camera } from 'src/shared/core/camera';
import { Particles } from 'src/shared/core/particles';
import { WORLD } from 'src/shared/design/constants/world-palette.constant';
import { dist } from 'src/shared/utils/dist.util';

export class SurvivalSystem {
    constructor(
        private world: World,
        private build: BuildSystem,
        private particles: Particles,
        private audio: Audio,
        private camera: Camera,
        private hooks: SurvivalHooks,
    ) {}

    updateSurvival(dt: number): void {
        const p = this.hooks.player();
        if (!p.alive) {
            p.respawnTimer -= dt;
            return;
        }

        if (this.hooks.sandbox()) {
            // Topped up every tick, so the vitals bars still read normally and
            // nothing in the rest of the sim has to know about creative mode.
            p.calories = PLAYER.maxCalories;
            p.hydration = PLAYER.maxHydration;
            p.temperature = PLAYER.comfortTemp;
            p.radiation = 0;
            p.bleeding = 0;
            p.health = PLAYER.maxHealth;
            return;
        }

        p.calories = Math.max(0, p.calories - PLAYER.calorieDrain * dt);
        p.hydration = Math.max(0, p.hydration - PLAYER.hydrationDrain * dt);

        // Temperature: biome, time of day, clothing and any fire you are near.
        const biome = this.world.biomeAt(p.x, p.y);
        let ambient =
            (biome === 'water' ? SWIM.temperature : BIOME_TEMP[biome]) - this.hooks.darkness() * 18;
        for (const d of this.build.deployables) {
            if (!d.lit) continue;
            const dd = dist(p.x, p.y, d.x, d.y);
            if (dd < FIRE_WARMTH_RADIUS) ambient += FIRE_WARMTH * (1 - dd / FIRE_WARMTH_RADIUS);
        }
        const worn = wornId(p);
        if (worn) ambient += ITEMS[worn].wear?.warmth ?? 0;
        p.temperature += (ambient - p.temperature) * Math.min(1, dt * 0.5);

        // Radiation, reduced by protective clothing.
        const rads = this.world.radiationAt(p.x, p.y);
        const suit = wornId(p);
        const prot = suit ? (ITEMS[suit].wear?.radiation ?? 0) : 0;
        if (rads > 0)
            p.radiation = Math.min(PLAYER.maxRadiation, p.radiation + rads * (1 - prot) * dt);
        else p.radiation = Math.max(0, p.radiation - 2.2 * dt);

        let damage = 0;
        if (p.calories <= 0) damage += PLAYER.starveDamage;
        if (p.hydration <= 0) damage += PLAYER.starveDamage * 1.4;
        if (p.temperature < PLAYER.comfortTemp - 12) {
            // Scales with how cold it actually is, so a chilly night is survivable
            // and a snow storm without clothing is not.
            const below = PLAYER.comfortTemp - 12 - p.temperature;
            damage += PLAYER.coldDamage * (1 + Math.min(2.5, below / 14));
        }
        if (p.radiation > 45) damage += PLAYER.radDamage * ((p.radiation - 45) / 55);
        if (p.bleeding > 0) {
            damage += 1.6;
            p.bleeding -= dt;
        }
        if (damage > 0) {
            p.health -= damage * dt;
            if (p.health <= 0) this.hooks.die();
        }

        if (p.healOverTime > 0) {
            const tick = Math.min(p.healOverTime, 12 * dt);
            p.health = Math.min(PLAYER.maxHealth, p.health + tick);
            p.healOverTime -= tick;
        } else if (p.calories > 40 && p.hydration > 40 && damage === 0) {
            // Well fed and watered, you slowly knit back together.
            p.health = Math.min(PLAYER.maxHealth, p.health + 0.6 * dt);
        }
    }

    hurtPlayer(amount: number, fromX: number, fromY: number): void {
        const p = this.hooks.player();
        if (!p.alive || p.invuln > 0 || this.hooks.sandbox()) return;
        const kit = wornId(p);
        const armor = kit ? (ITEMS[kit].wear?.armor ?? 0) : 0;
        const dealt = amount * (1 - armor);
        p.health -= dealt;
        if (p.using) {
            this.hooks.notify(`${ITEMS[p.using].name} interrupted.`);
            p.using = null;
            p.useLeft = 0;
        }
        if (p.reloadLeft > 0) {
            this.hooks.notify('Reload interrupted.');
            p.reloadLeft = 0;
        }
        p.invuln = 0.35;
        p.hurtFlash = 0.3;
        if (Math.random() < 0.3) p.bleeding = Math.max(p.bleeding, 6);
        this.audio.playerHurt();
        this.camera.shake(6, 0.25);
        const a = Math.atan2(p.y - fromY, p.x - fromX);
        this.particles.burst(p.x, p.y, 8, '#c22b2b', {
            speed: 160,
            life: 0.45,
            size: 2.8,
            dir: a,
            spread: 1.5,
        });
        this.particles.text(p.x, p.y - 32, `-${Math.round(dealt)}`, '#ff8a8a');
        if (p.health <= 0) this.hooks.die();
    }

    consume(id: ItemId): void {
        const p = this.hooks.player();
        const def = ITEMS[id];
        if (!def.food) return;
        if (countAcross(this.hooks.containers(), id) <= 0) {
            this.audio.deny();
            return;
        }
        if (def.useSeconds && def.useSeconds > 0) {
            p.using = id;
            p.useLeft = def.useSeconds;
            p.useTotal = def.useSeconds;
            p.attackTimer = def.useSeconds;
            this.hooks.notify(`Using ${def.name}. Walk if you like, do not run.`);
            return;
        }
        this.applyConsumable(id);
    }

    private applyConsumable(id: ItemId): void {
        const p = this.hooks.player();
        const food = ITEMS[id].food;
        if (!food) return;
        if (removeAcross(this.hooks.containers(), id, 1) <= 0) return;
        if (food.calories) p.calories = Math.min(PLAYER.maxCalories, p.calories + food.calories);
        if (food.hydration)
            p.hydration = Math.min(PLAYER.maxHydration, p.hydration + food.hydration);
        if (food.health) {
            if (id === 'medkit') p.healOverTime += food.health;
            else p.health = Math.min(PLAYER.maxHealth, p.health + food.health);
            p.bleeding = 0;
        }
        this.audio.craft();
        this.particles.burst(p.x, p.y, 10, WORLD.vital, { speed: 80, life: 0.6, size: 2.6 });
        this.hooks.notify(`Used ${ITEMS[id].name}.`);
    }

    /** Tick a use-channel. Running or taking a hit throws it away. */

    updateUse(dt: number, sprinting: boolean): void {
        const p = this.hooks.player();
        if (!p.using) return;
        if (sprinting) {
            this.hooks.notify(`${ITEMS[p.using].name} interrupted.`);
            p.using = null;
            p.useLeft = 0;
            return;
        }
        p.useLeft -= dt;
        if (Math.random() < dt * 8) {
            this.particles.burst(p.x, p.y - 6, 1, WORLD.vital, { speed: 30, life: 0.5, size: 2 });
        }
        if (p.useLeft > 0) return;
        const id = p.using;
        p.using = null;
        p.useLeft = 0;
        this.applyConsumable(id);
    }

    /**
     * [R]. Top the held weapon up from your pack. Reloading is a channel like
     * bandaging: you can walk through it, you cannot run through it, and taking
     * a hit costs you the magazine change.
     */

    reloadHeld(): void {
        const p = this.hooks.player();
        const held = this.hooks.heldItem();
        const def = held ? ITEMS[held.id].gun : undefined;
        if (!held || !def || !def.magazine) {
            this.audio.deny();
            return;
        }
        if (p.reloadLeft > 0) return;
        if ((held.loaded ?? 0) >= def.magazine) {
            this.hooks.notify('Already loaded.');
            return;
        }
        if (this.hooks.sandbox()) {
            held.loaded = def.magazine;
            this.audio.craft();
            return;
        }
        if (countAcross(this.hooks.containers(), def.ammo) <= 0) {
            this.audio.deny();
            this.hooks.notify(`Out of ${ITEMS[def.ammo].name.toLowerCase()}.`);
            return;
        }
        p.using = null;
        p.useLeft = 0;
        p.reloadTotal = def.reloadSeconds ?? 2.5;
        p.reloadLeft = p.reloadTotal;
        this.hooks.notify(`Reloading ${ITEMS[held.id].name}.`);
    }

    /** Tick a magazine change. Running or changing weapon throws it away. */

    updateReload(dt: number, sprinting: boolean): void {
        const p = this.hooks.player();
        if (p.reloadLeft <= 0) return;
        const held = this.hooks.heldItem();
        const def = held ? ITEMS[held.id].gun : undefined;
        if (!held || !def || !def.magazine || sprinting) {
            p.reloadLeft = 0;
            return;
        }
        p.reloadLeft -= dt;
        if (p.reloadLeft > 0) return;
        p.reloadLeft = 0;
        const want = def.magazine - (held.loaded ?? 0);
        const have = countAcross(this.hooks.containers(), def.ammo);
        const take = Math.min(want, have);
        if (take <= 0) return;
        removeAcross(this.hooks.containers(), def.ammo, take);
        held.loaded = (held.loaded ?? 0) + take;
        this.audio.craft();
        this.hooks.notify(`${ITEMS[held.id].name}: ${held.loaded}/${def.magazine}.`);
    }

    /**
     * Take off what you are wearing, back into the pack.
     *
     * On the ground only if there is nowhere for it: taking a coat off should
     * never be how you lose it.
     */
    takeOff(): void {
        const p = this.hooks.player();
        const worn = wornId(p);
        if (!worn) return;
        p.worn.slots[0] = null;
        const left = addAcross(this.hooks.containers(), worn, 1);
        if (left > 0) this.hooks.dropStack({ id: worn, count: left }, p.x, p.y);
        this.hooks.notify(`Took off the ${ITEMS[worn].name.toLowerCase()}.`);
    }

    /** Put something on from the inventory screen. */

    wear(id: ItemId): void {
        const p = this.hooks.player();
        removeAcross(this.hooks.containers(), id, 1);
        // What you had on goes back into the pack, not on the floor.
        const previous = wornId(p);
        if (previous) addAcross(this.hooks.containers(), previous, 1);
        p.worn.slots[0] = { id, count: 1 };
        p.attackTimer = 0.5;
        this.audio.craft();
        this.hooks.notify(`Wearing ${ITEMS[id].name}.`);
    }

    wearItem(id: ItemId): void {
        if (countAcross(this.hooks.containers(), id) <= 0) return;
        this.wear(id);
    }
}
