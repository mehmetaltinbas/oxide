// Every sound the game makes, rendered without a sound card and looked at: a
// silent mixer is exactly the bug a test should catch, and a machine running
// this may have no speakers at all.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "audio.hpp"

namespace {

struct Case {
    const char* name;
    double seconds;
};

/** How loud it got, and how much of it was not silence. */
void measure(const char* name, const std::vector<float>& samples, int& failures) {
    float peak = 0;
    double energy = 0;
    for (const float sample : samples) {
        peak = std::max(peak, std::abs(sample));
        energy += static_cast<double>(sample) * sample;
    }
    const double rms = std::sqrt(energy / std::max<std::size_t>(1, samples.size()));
    const bool ok = peak > 0.01 && peak <= 1.0;
    if (!ok) ++failures;
    std::printf("  %-18s peak %.3f  rms %.4f  %s\n", name, peak, rms, ok ? "" : "SILENT");
}

}  // namespace

int main() {
    client::Audio audio;
    audio.useMemory();
    int failures = 0;
    std::vector<float> samples;

    const auto run = [&](const char* name, double seconds, auto&& play) {
        play();
        audio.renderTo(samples, seconds);
        measure(name, samples, failures);
    };

    std::printf("sounds:\n");
    run("chop wood", 0.3, [&] { audio.chopWood(); });
    run("hit stone", 0.3, [&] { audio.hitStone(); });
    run("hit metal", 0.4, [&] { audio.hitMetal(); });
    run("hit sulfur", 0.3, [&] { audio.hitSulfur(); });
    run("hit flesh", 0.3, [&] { audio.hitFlesh(); });
    run("tree fall", 0.9, [&] { audio.treeFall(); });
    run("player hurt", 0.4, [&] { audio.playerHurt(); });
    run("build", 0.3, [&] { audio.build(); });
    run("craft", 0.3, [&] { audio.craft(); });
    run("pickup", 0.2, [&] { audio.pickup(); });
    run("roar", 1.2, [&] { audio.roar(); });

    for (const sim::ItemId gun :
         {sim::ItemId::Bow, sim::ItemId::Revolver, sim::ItemId::Rifle, sim::ItemId::Ak47,
          sim::ItemId::Waterpipe, sim::ItemId::PumpShotgun, sim::ItemId::RocketLauncher}) {
        const client::GunSound* report = client::gunSoundOf(gun);
        if (!report) {
            std::printf("  %-18s NO SOUND\n", sim::itemDef(gun).name);
            ++failures;
            continue;
        }
        audio.gunshot(*report);
        audio.renderTo(samples, 1.2);
        measure(sim::itemDef(gun).name, samples, failures);
    }

    // And one from across a field, which should be quieter than one at your feet.
    audio.listenAt(0, 0);
    audio.from(700, 0, [&] { audio.chopWood(); });
    audio.renderTo(samples, 0.3);
    float far = 0;
    for (const float sample : samples) far = std::max(far, std::abs(sample));
    audio.from(20, 0, [&] { audio.chopWood(); });
    audio.renderTo(samples, 0.3);
    float near = 0;
    for (const float sample : samples) near = std::max(near, std::abs(sample));
    std::printf("  distance: near %.3f, far %.3f, %s\n", near, far,
                far < near ? "fades with distance" : "DOES NOT FADE");
    if (far >= near) ++failures;

    std::printf("%s\n", failures == 0 ? "all sounds make a noise" : "SOME SOUNDS ARE SILENT");
    return failures == 0 ? 0 : 1;
}
