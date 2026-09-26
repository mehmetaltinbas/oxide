#include "audio.hpp"

#include <algorithm>
#include <cmath>

namespace client {

namespace {

constexpr int kSampleRate = 48000;
/** Beyond this many at once the oldest is dropped rather than the frame. */
constexpr std::size_t kMostVoices = 48;
constexpr double kTau = 6.28318530717959;

/**
 * What each firearm sounds like, loosely from what the real thing does: the
 * bow has no report at all, only the string; the revolver is a bright crack;
 * the semi-auto is heavier; the AK punchier and shorter so it stays clean at
 * seven a second; the shotguns are low booms, and the launcher is a rushing
 * whoosh rather than a bang.
 */
constexpr GunSound kBow{{2400, 0.12, 0.12, 1}, {190, 95, 0.16, 0.28, 2}, {}, false, {}, 0};
constexpr GunSound kRevolver{
    {2600, 0.12, 0.55, 2}, {150, 60, 0.14, 0.35, 1}, {900, 0.35, 0.18}, true, {}, 0};
constexpr GunSound kRifle{
    {3200, 0.1, 0.6, 2}, {115, 45, 0.2, 0.45, 1}, {700, 0.55, 0.24}, true, {}, 0};
constexpr GunSound kAk{
    {2800, 0.07, 0.55, 2}, {100, 45, 0.12, 0.45, 1}, {650, 0.28, 0.18}, true, {}, 0};
constexpr GunSound kWaterpipe{
    {1300, 0.32, 0.7, 0}, {75, 30, 0.36, 0.6, 0}, {500, 0.7, 0.28}, true, {}, 0};
// Rack back, rack forward.
constexpr GunSound kPump{
    {1600, 0.28, 0.68, 0}, {85, 35, 0.3, 0.55, 0}, {550, 0.6, 0.25}, true, {0.38, 0.52}, 2};
constexpr GunSound kLauncher{
    {700, 0.75, 0.5, 2}, {60, 38, 0.35, 0.45, 0}, {400, 0.9, 0.2}, true, {}, 0};

/** One sample of a wave, by its shape. */
double waveAt(int wave, double phase) {
    switch (wave) {
        case 1: return std::sin(phase) >= 0 ? 1.0 : -1.0;
        case 2: return 2.0 / 3.14159265358979 * std::asin(std::sin(phase));
        case 3: return 2 * (phase / kTau - std::floor(phase / kTau + 0.5));
        default: return std::sin(phase);
    }
}

}  // namespace

const GunSound* gunSoundOf(sim::ItemId id) {
    switch (id) {
        case sim::ItemId::Bow: return &kBow;
        case sim::ItemId::Revolver: return &kRevolver;
        case sim::ItemId::Rifle: return &kRifle;
        case sim::ItemId::Ak47: return &kAk;
        case sim::ItemId::Waterpipe: return &kWaterpipe;
        case sim::ItemId::PumpShotgun: return &kPump;
        case sim::ItemId::RocketLauncher: return &kLauncher;
        default: return nullptr;
    }
}

bool Audio::open() {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    lock_ = SDL_CreateMutex();
    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;
    spec.freq = kSampleRate;
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, feed, this);
    if (!stream_) return false;
    SDL_ResumeAudioStreamDevice(stream_);
    return true;
}

void Audio::close() {
    if (stream_) SDL_DestroyAudioStream(stream_);
    stream_ = nullptr;
    if (lock_) SDL_DestroyMutex(lock_);
    lock_ = nullptr;
}

void Audio::setVolume(double level) { level_ = std::clamp(level, 0.0, 1.0); }

void Audio::setEnabled(bool on) { enabled_ = on; }

void Audio::add(const Voice& voice) {
    if (!enabled_ || (!stream_ && !offline_)) return;
    SDL_LockMutex(lock_);
    if (voices_.size() >= kMostVoices) voices_.erase(voices_.begin());
    voices_.push_back(voice);
    SDL_UnlockMutex(lock_);
}

void Audio::tone(double freq, double dur, int wave, double peak, double slideTo, double at) {
    Voice voice{};
    voice.noise = false;
    voice.freq = freq;
    voice.slideTo = slideTo > 0 ? slideTo : freq;
    voice.dur = dur;
    voice.peak = std::max(0.0002, peak * scale_);
    voice.wave = wave;
    voice.at = at;
    add(voice);
}

void Audio::noise(double dur, double peak, double filterHz, int filter, double at) {
    Voice voice{};
    voice.noise = true;
    voice.freq = filterHz;
    voice.dur = dur;
    voice.peak = std::max(0.0002, peak * scale_);
    voice.filter = filter;
    voice.at = at;
    add(voice);
}

void Audio::useMemory() {
    offline_ = true;
    if (!lock_) lock_ = SDL_CreateMutex();
}

void Audio::renderTo(std::vector<float>& out, double seconds) {
    useMemory();
    const int frames = static_cast<int>(seconds * kSampleRate);
    out.assign(static_cast<std::size_t>(frames), 0.0f);
    for (int at = 0; at < frames; at += 512) {
        mix(out.data() + at, std::min(512, frames - at));
    }
}

void SDLCALL Audio::feed(void* self, SDL_AudioStream* stream, int additional, int) {
    auto* audio = static_cast<Audio*>(self);
    // Asked for bytes; a frame here is one float.
    int frames = additional / static_cast<int>(sizeof(float));
    while (frames > 0) {
        float chunk[512];
        const int take = std::min(frames, 512);
        audio->mix(chunk, take);
        SDL_PutAudioStreamData(stream, chunk, take * static_cast<int>(sizeof(float)));
        frames -= take;
    }
}

void Audio::mix(float* out, int frames) {
    for (int i = 0; i < frames; ++i) out[i] = 0;
    SDL_LockMutex(lock_);
    const double step = 1.0 / kSampleRate;
    for (Voice& voice : voices_) {
        for (int i = 0; i < frames; ++i) {
            if (voice.at > 0) {
                // Not yet: a pump racks a moment after the shot.
                voice.at -= step;
                continue;
            }
            if (voice.t >= voice.dur) break;
            // The envelope: up in eight milliseconds, then down to nothing,
            // which is the shape every one of these sounds has.
            const double t = voice.t / voice.dur;
            const double attack = std::min(1.0, voice.t / 0.008);
            const double fall = std::pow(0.0002 / std::max(0.0002, voice.peak), t);
            const double gain = voice.peak * attack * fall;

            double sample = 0;
            if (voice.noise) {
                // White noise through a one-pole filter, which is as much
                // shaping as any of these need.
                noiseSeed_ = noiseSeed_ * 1103515245u + 12345u;
                const double white =
                    static_cast<double>((noiseSeed_ >> 16) & 0x7fff) / 16384.0 - 1.0;
                const double cut = std::clamp(voice.freq / (kSampleRate * 0.5), 0.0005, 0.99);
                voice.low += (white - voice.low) * cut;
                voice.high = white - voice.low;
                sample = voice.filter == 1   ? voice.high
                         : voice.filter == 2 ? voice.low - (voice.low - voice.high) * 0.5
                                             : voice.low;
            } else {
                // The tone slides from where it starts to where it ends.
                const double freq = voice.freq + (voice.slideTo - voice.freq) * t;
                voice.phase += kTau * freq * step;
                if (voice.phase > kTau) voice.phase -= kTau;
                sample = waveAt(voice.wave, voice.phase);
            }
            out[i] += static_cast<float>(sample * gain);
            voice.t += step;
        }
    }
    voices_.erase(std::remove_if(voices_.begin(), voices_.end(),
                                 [](const Voice& v) { return v.at <= 0 && v.t >= v.dur; }),
                  voices_.end());
    SDL_UnlockMutex(lock_);

    // The master level, and a soft clip so a hatchet in a firefight does not
    // tear: everything here is synthesized and the raw tones are hot.
    for (int i = 0; i < frames; ++i) {
        const float value = static_cast<float>(out[i] * level_);
        out[i] = std::tanh(value);
    }
}

// ---- The sounds themselves, each with its own body.

void Audio::chopWood() {
    // Axe into a trunk: a dull woody thock.
    noise(0.11, 0.4, 900);
    tone(150, 0.11, 2, 0.3, 78);
    tone(320, 0.05, 0, 0.12, 210);
}

void Audio::hitStone() {
    // Pick into rock: a bright sharp crack with grit.
    noise(0.07, 0.5, 4200, 2);
    tone(760, 0.05, 1, 0.13, 420);
    noise(0.16, 0.18, 1800);
}

void Audio::hitMetal() {
    // Metal ore: the crack plus a ring.
    noise(0.06, 0.45, 5200, 2);
    tone(1180, 0.22, 0, 0.16, 880);
    tone(1760, 0.16, 0, 0.08, 1500);
}

void Audio::hitSulfur() {
    // Duller and more crumbly than metal.
    noise(0.09, 0.45, 2600, 2);
    tone(520, 0.08, 2, 0.14, 300);
}

void Audio::hitFlesh() {
    // Wet, low, no ring at all.
    noise(0.09, 0.45, 700);
    tone(190, 0.09, 3, 0.2, 96);
}

void Audio::hitStructureWood() {
    noise(0.13, 0.42, 1100);
    tone(130, 0.14, 2, 0.26, 70);
}

void Audio::hitStructureMetal() {
    // Sheet metal in a wall: loud, ringing, unmistakable.
    noise(0.05, 0.4, 6000, 2);
    tone(880, 0.3, 0, 0.2, 620);
    tone(1320, 0.22, 2, 0.1, 990);
}

void Audio::pluck() {
    noise(0.09, 0.28, 3000, 2);
    tone(620, 0.06, 2, 0.1, 900);
}

void Audio::treeFall() {
    noise(0.7, 0.6, 700);
    tone(90, 0.6, 0, 0.3, 40);
}

void Audio::hit() {
    noise(0.09, 0.5, 2200, 2);
    tone(300, 0.07, 1, 0.15, 140);
}

void Audio::playerHurt() { tone(240, 0.22, 3, 0.35, 110); }

void Audio::enemyDie() {
    tone(300, 0.3, 3, 0.3, 70);
    noise(0.25, 0.3, 900);
}

void Audio::build() {
    tone(440, 0.1, 1, 0.25);
    tone(660, 0.12, 1, 0.2);
}

void Audio::craft() {
    tone(523, 0.09, 2, 0.3);
    tone(784, 0.14, 2, 0.25);
}

void Audio::deny() { tone(160, 0.14, 1, 0.25, 110); }

void Audio::pickup() { tone(880, 0.07, 2, 0.18); }

void Audio::roar() {
    tone(90, 0.9, 3, 0.35, 55);
    noise(0.8, 0.25, 500);
}

void Audio::howl() { tone(320, 0.8, 3, 0.22, 520); }

void Audio::gunshot(const GunSound& sound, double loudness) {
    if (loudness <= 0.01) return;
    noise(sound.crack.dur, sound.crack.peak * loudness, sound.crack.hz, sound.crack.filter);
    tone(sound.thump.from, sound.thump.dur, sound.thump.wave, sound.thump.peak * loudness,
         sound.thump.to);
    if (sound.hasTail) noise(sound.tail.dur, sound.tail.peak * loudness, sound.tail.hz);
    for (int i = 0; i < sound.mechCount; ++i) {
        noise(0.035, 0.22 * loudness, 3200, 1, sound.mech[i]);
    }
}

}  // namespace client
