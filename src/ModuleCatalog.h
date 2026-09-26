// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>

namespace batchly {
// One small table keeps navigation and saved program numbers in the same order.
// New modules are appended so existing DAW automation and programs stay stable.
inline constexpr int moduleCount = 12;
inline constexpr int presetsPerModule = 5;
inline constexpr const char* moduleNames[moduleCount] { "Drift", "Patina", "Atrium", "Chime", "Helix", "Gleam", "Relay", "Forge", "Cinder", "Ember", "Vista", "Quartz" };
inline constexpr const char* enabledIds[moduleCount] { "drift_enabled", "patina_enabled", "atrium_enabled", "chime_enabled", "helix_enabled", "gleam_enabled", "relay_enabled", "forge_enabled", "cinder_enabled", "ember_enabled", "vista_enabled", "quartz_enabled" };
inline constexpr const char* programKeys[moduleCount] { "driftProgram", "patinaProgram", "atriumProgram", "chimeProgram", "helixProgram", "gleamProgram", "relayProgram", "forgeProgram", "cinderProgram", "emberProgram", "vistaProgram", "quartzProgram" };
inline constexpr const char* presetNames[moduleCount][presetsPerModule] {
    { "Soft focus", "Slow tide", "Wide room", "Worn motor", "Pure vibrato" },
    { "Fresh spool", "Pocket cassette", "Submerged", "Sun-bleached", "Midnight dub" },
    { "Open atrium", "Close walls", "Velvet hall", "Glass canopy", "After hours" },
    { "Glass strings", "Minor bells", "Copper choir", "Small music box", "Suspended air" },
    { "Slow orbit", "Silver sweep", "Deep current", "Retro spin", "Hollow metal" },
    { "Clear vocal", "Silver top", "Drum shine", "Soft lift", "Open mix" },
    { "Soft answer", "Cross town", "Short circuit", "Long return", "Bent signal" },
    { "First strike", "Heavy floor", "Snare press", "Soft mallet", "Parallel iron" },
    { "Fine grain", "Copper dust", "Paper speaker", "Ash cloud", "Rough edge" },
    { "Warm foundation", "Wire bass", "Dense floor", "Folded metal", "Quiet ember" },
    { "Open window", "Centered bass", "Mono bloom", "Narrow room", "Air frame" },
    { "Clear finish", "Warm facets", "Bright polish", "Dense cut", "Soft edges" }
};
inline constexpr int factoryControlCount[moduleCount] { 9, 8, 8, 11, 8, 8, 8, 8, 8, 8, 8, 8 };
inline constexpr const char* factoryIds[moduleCount][11] {
    { "depth", "rate", "wander", "tone", "follow", "noise", "width", "mix", "output" },
    { "patina_sample", "patina_drive", "patina_wear", "patina_flutter", "patina_hiss", "patina_chorus", "patina_tone", "patina_mix" },
    { "atrium_decay", "atrium_size", "atrium_predelay", "atrium_damping", "atrium_lowcut", "atrium_motion", "atrium_width", "atrium_mix" },
    { "chime_ring", "chime_color", "chime_drive", "chime_spread", "chime_detune", "chime_motion", "chime_width", "chime_mix", "chime_root", "chime_scale", "chime_octave" },
    { "helix_rate", "helix_depth", "helix_feedback", "helix_center", "helix_tone", "helix_drive", "helix_width", "helix_mix" },
    { "gleam_presence", "gleam_air", "gleam_focus", "gleam_excite", "gleam_tame", "gleam_width", "gleam_trim", "gleam_mix" },
    { "relay_time", "relay_feedback", "relay_tone", "relay_motion", "relay_rate", "relay_bounce", "relay_glide", "relay_mix" },
    { "forge_punch", "forge_body", "forge_weight", "forge_edge", "forge_drive", "forge_ceiling", "forge_width", "forge_mix" },
    { "cinder_grit", "cinder_noise", "cinder_tone", "cinder_texture", "cinder_drive", "cinder_decay", "cinder_width", "cinder_mix" },
    { "ember_drive", "ember_shape", "ember_color", "ember_filter", "ember_anchor", "ember_bias", "ember_trim", "ember_mix" },
    { "vista_low", "vista_mid", "vista_high", "vista_low_split", "vista_high_split", "vista_spread", "vista_delay", "vista_mix" },
    { "quartz_input", "quartz_low", "quartz_mid", "quartz_high", "quartz_character", "quartz_ceiling", "quartz_release", "quartz_mix" }
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
    }, {
        {.24f,.65f,.35f,650,11000,.1f,.75f,.5f},
        {.62f,.85f,.55f,1300,16000,.05f,1,.5f},
        {.075f,.9f,.7f,340,3800,.2f,.6f,.55f},
        {2.4f,.55f,.25f,850,8000,.3f,.25f,.5f},
        {.16f,.35f,-.65f,1900,14500,.1f,.85f,.6f}
    }, {
        {2.5f,4,7500,.15f,.5f,1,-3,1},
        {.5f,7,11000,.08f,.25f,1.1f,-3.5f,1},
        {4,5,6500,.3f,.6f,1.15f,-4,1},
        {1.5f,2,9000,0,.7f,1,-1.5f,1},
        {1,3,10000,.05f,.35f,1.2f,-2,1}
    }, {
        {350,.35f,6500,.15f,.3f,.65f,.35f,.3f},
        {240,.55f,9000,.08f,.6f,1,.2f,.4f},
        {85,.2f,4500,.25f,1.2f,.2f,.1f,.35f},
        {780,.72f,2800,.2f,.16f,.85f,.6f,.4f},
        {140,.5f,5500,.9f,2.2f,.8f,.8f,.5f}
    }, {
        {.3f,.2f,2,1,.1f,-1,1,1},
        {.45f,.3f,6,0,.2f,-1.5f,1,1},
        {.55f,.45f,1,5,.25f,-2,1.15f,1},
        {-.5f,.15f,2,0,.05f,-2,1,1},
        {.2f,.8f,3,3,.55f,-3,1.2f,.5f}
    }, {
        {.35f,.12f,1800,6500,.3f,.15f,.7f,1},
        {.55f,.2f,500,2200,.55f,.12f,.4f,.7f},
        {.8f,.08f,1200,8500,.7f,.05f,0,.8f},
        {.1f,.65f,3000,4200,.2f,.6f,1,.65f},
        {.7f,.3f,4500,10000,.65f,.08f,.85f,.6f}
    }, {
        {9,0,-.15f,6500,.65f,.12f,-5,1},
        {15,1.f/3,.35f,10000,.3f,0,-7,1},
        {12,2.f/3,-.3f,4500,.8f,0,-6,1},
        {20,1,.3f,8000,.2f,.2f,-10,.65f},
        {6,.18f,0,14000,.5f,.08f,-3,.65f}
    }, {
        {.8f,1.25f,1.6f,180,4000,.15f,11,1},
        {0,1,1.4f,200,3500,0,11,1},
        {1,1,1,250,3500,.65f,17,1},
        {.5f,.65f,.8f,180,4000,0,11,1},
        {.7f,1,1.8f,200,4500,.2f,7,.75f}
    }, {
        {1,0,0,.5f,.05f,-1,120,1},
        {2,2,-.5f,-1,.25f,-1.5f,180,1},
        {1,-1,.5f,2,.1f,-1,90,1},
        {6,1,1,.5f,.15f,-3,220,1},
        {0,0,-1,-1.5f,.45f,-2,250,.75f}
    }
};
}
