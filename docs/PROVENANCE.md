# Implementation provenance

## Drift

The functional reference is the public description of [Cymatics Memory](https://cymatics.fm/products/memory-plugin), consulted September 25-26, 2026. It describes an analog-style chorus/vibrato effect with random modulation, noise and an envelope-following filter. The user's installed interface was viewed to understand the presentation style and exposed controls. Previously recorded host parameter names were consulted as interface information.

No commercial source code was supplied, decompiled, disassembled, translated or incorporated. No factory preset data, sample library, noise recording, impulse response, logo, screenshot or extracted artwork is used in the app. This is independently written code based on observable functionality and general audio techniques, not a claim of a formally separated clean-room process.

`src/DriftEngine.h` implements a fractional delay, a seeded pseudorandom modulation source with quintic interpolation, a sine/random blend, per-channel movement, an envelope follower, a one-pole low-pass filter, generated noise and a DC blocker. Parameter smoothing and a smoothed bypass avoid abrupt changes. Allocation happens in `prepare`, not the processing loop.

Drift does not model the full nonlinear behavior of an analog BBD chip and does not reproduce Memory's private algorithms. Its parameter ranges and presets are original choices. A functional comparison should assess musical roles, not assume equivalent knob values.

The demo and listening examples are synthesized from mathematical oscillators. The interface is drawn with native vector shapes. Its light ground, dark ink, red accent, straight borders and Archivo/JetBrains Mono typography follow the owner's Batchly theme. The two fonts are embedded from Google Fonts under their retained SIL Open Font Licenses. These assets require no Cymatics installation or sound library.

## Framework

JUCE 9.0.2 is pinned to commit `72782788ce18c2d4d760b28e0921d6ffc6431102`. It supplies the GUI, audio-device management, file reading/writing, parameter management, and VST3/standalone wrappers. It does not implement Drift's effect algorithm.

Before adding another module, record its public references, intended audible behavior, original design choices and verification results here. Sample-based instruments need independently created or redistributable sound sources. An installed commercial sound library is not automatically an open-source asset source.
