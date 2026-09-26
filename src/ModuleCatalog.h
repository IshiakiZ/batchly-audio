// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>

namespace batchly {
// One small table keeps navigation and saved program numbers in the same order.
// New modules are appended so existing DAW automation and programs stay stable.
inline constexpr int moduleCount = 4;
inline constexpr int presetsPerModule = 5;
inline constexpr const char* moduleNames[moduleCount] { "Drift", "Patina", "Atrium", "Chime" };
inline constexpr const char* enabledIds[moduleCount] { "drift_enabled", "patina_enabled", "atrium_enabled", "chime_enabled" };
inline constexpr const char* programKeys[moduleCount] { "driftProgram", "patinaProgram", "atriumProgram", "chimeProgram" };
inline constexpr const char* presetNames[moduleCount][presetsPerModule] {
    { "Soft focus", "Slow tide", "Wide room", "Worn motor", "Pure vibrato" },
    { "Fresh spool", "Pocket cassette", "Submerged", "Sun-bleached", "Midnight dub" },
    { "Open atrium", "Close walls", "Velvet hall", "Glass canopy", "After hours" },
    { "Glass strings", "Minor bells", "Copper choir", "Small music box", "Suspended air" }
};
inline constexpr int factoryControlCount[moduleCount] { 9, 8, 8, 11 };
inline constexpr const char* factoryIds[moduleCount][11] {
    { "depth", "rate", "wander", "tone", "follow", "noise", "width", "mix", "output" },
    { "patina_sample", "patina_drive", "patina_wear", "patina_flutter", "patina_hiss", "patina_chorus", "patina_tone", "patina_mix" },
    { "atrium_decay", "atrium_size", "atrium_predelay", "atrium_damping", "atrium_lowcut", "atrium_motion", "atrium_width", "atrium_mix" },
    { "chime_ring", "chime_color", "chime_drive", "chime_spread", "chime_detune", "chime_motion", "chime_width", "chime_mix", "chime_root", "chime_scale", "chime_octave" }
};
inline constexpr float factoryValues[moduleCount][presetsPerModule][11] {
    {
        { .35f,.45f,.65f,7000,0,0,.75f,.5f,0 }, { .65f,.14f,.9f,4800,.2f,0,.55f,.65f,-1 },
        { .22f,.85f,.15f,12000,0,0,1,.45f,0 }, { .85f,1.7f,.95f,2300,.55f,.22f,.4f,.8f,-2 },
        { .35f,2.1f,.15f,14500,0,0,0,1,0 }
    }, {
        {16000,.25f,.2f,.12f,0,.18f,11000,1}, {12500,.42f,.42f,.34f,.14f,.1f,5600,1},
        {4500,.18f,.1f,.04f,0,.08f,1800,1}, {22000,.38f,.3f,.2f,0,.62f,9000,1},
        {8500,.7f,.52f,.25f,.1f,.32f,3400,1}
    }, {
        {2.4f,.55f,24,6500,120,.2f,1,.25f}, {.55f,.15f,3,9500,180,.05f,.65f,.2f},
        {4.8f,.75f,42,2200,180,.3f,.9f,.32f}, {3.2f,.65f,35,14000,220,.55f,1,.3f},
        {9,.95f,80,4200,280,.8f,1,.45f}
    }, {
        {1.6f,8000,.12f,.5f,.25f,.15f,.85f,.4f,0,1,3},
        {.8f,11000,.05f,.7f,0,0,.7f,.55f,9,4,3},
        {2.8f,2800,.4f,.25f,.35f,.3f,.8f,.5f,7,0,2},
        {.24f,13000,0,.8f,0,0,.45f,.7f,2,2,4},
        {5.f,5800,.18f,1,.6f,.65f,1,.6f,5,5,3}
    }
};
}
