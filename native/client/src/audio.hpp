#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <vector>

#include "sim/item.hpp"

namespace client {

/** One layer of a gun's report: a burst of filtered noise, or a sliding tone. */
struct GunSound {
    struct Crack {
        double hz;
        double dur;
        double peak;
        /** 0 lowpass, 1 highpass, 2 bandpass, as the browser game named them. */
        int filter;
    } crack;
    struct Thump {
        double from;
        double to;
        double dur;
        double peak;
        /** 0 sine, 1 square, 2 triangle, 3 sawtooth. */
        int wave;
    } thump;
    struct Tail {
        double hz;
        double dur;
        double peak;
    } tail;
    bool hasTail;
    /** Mechanical clicks after the shot, in seconds: a pump racking. */
    double mech[2];
    int mechCount;
};

/** What a gun sounds like, or nothing for the ones that make no report. */
const GunSound* gunSoundOf(sim::ItemId id);

/**
 * Every sound the game makes, synthesized.
 *
 * No files: a sound here is a handful of tones and bursts of filtered noise,
 * mixed as they play, exactly as the browser game made them. Each impact has
 * its own body so you can tell what you are hitting with your eyes shut.
 */
class Audio {
public:
    bool open();
    void close();

    /** Master level, and whether anything is heard at all. */
    void setVolume(double level);
    double volume() const { return level_; }
    void setEnabled(bool on);
    bool enabled() const { return enabled_; }

    /** Where the ears are: the player, set each frame. */
    void listenAt(double x, double y) {
        listenerX_ = x;
        listenerY_ = y;
    }

    /**
     * Plays a sound as if it happened at a point: full on top of you, fading
     * with distance, and not at all out of earshot. Everything that happens in
     * the world goes through this; your own hands and the interface play flat.
     */
    template <typename Play>
    void from(double x, double y, Play play, double range = 1100) {
        const double d = SDL_sqrt((x - listenerX_) * (x - listenerX_) +
                                  (y - listenerY_) * (y - listenerY_));
        const double near = 1 - d / range;
        if (near <= 0.02) return;
        const double was = scale_;
        scale_ = SDL_pow(near, 1.5);
        play();
        scale_ = was;
    }

    // Each material has its own body, so you know what you hit by ear.
    void chopWood();
    void hitStone();
    void hitMetal();
    void hitSulfur();
    void hitFlesh();
    void hitStructureWood();
    void hitStructureMetal();
    void pluck();
    void treeFall();
    void hit();
    void playerHurt();
    void enemyDie();
    void build();
    void craft();
    void deny();
    void pickup();
    void roar();
    void howl();
    void gunshot(const GunSound& sound, double loudness = 1);

    /**
     * Mixes without a device, for checking the sounds are there at all.
     *
     * A sound card is not something a test can rely on, and a silent mixer is
     * exactly the bug worth catching, so the samples are rendered to memory
     * and looked at.
     */
    void renderTo(std::vector<float>& out, double seconds);
    /** Mix to memory from here on, with no device open. */
    void useMemory();

private:
    /** One voice being mixed: a tone or a burst of noise, under an envelope. */
    struct Voice {
        bool noise;
        double freq;
        double slideTo;
        double dur;
        double peak;
        int wave;
        int filter;
        double at;
        /** How far through it is, in seconds. */
        double t;
        double phase;
        /** State for the one-pole filters the noise is shaped with. */
        double low;
        double high;
    };

    void tone(double freq, double dur, int wave, double peak, double slideTo = 0, double at = 0);
    void noise(double dur, double peak, double filterHz, int filter = 0, double at = 0);
    void add(const Voice& voice);

    static void SDLCALL feed(void* self, SDL_AudioStream* stream, int additional, int total);
    void mix(float* out, int frames);

    SDL_AudioStream* stream_ = nullptr;
    SDL_Mutex* lock_ = nullptr;
    std::vector<Voice> voices_;
    double level_ = 0.35;
    bool enabled_ = true;
    double listenerX_ = 0;
    double listenerY_ = 0;
    /** Loudness of whatever is being played right now, from how far off it is. */
    double scale_ = 1;
    /** The rolling white noise source, kept between callbacks. */
    std::uint32_t noiseSeed_ = 22695477u;
    /** Mixing to memory rather than to a device. */
    bool offline_ = false;
};

}  // namespace client
