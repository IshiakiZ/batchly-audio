# Implementation provenance

## Drift

The functional reference is the public description of [Cymatics Memory](https://cymatics.fm/products/memory-plugin), consulted September 25-26, 2026. It describes an analog-style chorus/vibrato effect with random modulation, noise and an envelope-following filter. The user's installed interface was viewed to understand the presentation style and exposed controls. Previously recorded host parameter names were consulted as interface information.

No commercial source code was supplied, decompiled, disassembled, translated or incorporated. No factory preset data, sample library, noise recording, impulse response, logo, screenshot or extracted artwork is used in the app. This is independently written code based on observable functionality and general audio techniques, not a claim of a formally separated clean-room process.

`src/DriftEngine.h` implements a fractional delay, a seeded pseudorandom modulation source with quintic interpolation, a sine/random blend, per-channel movement, an envelope follower, a one-pole low-pass filter, generated noise and a DC blocker. Parameter smoothing and a smoothed bypass avoid abrupt changes. Allocation happens in `prepare`, not the processing loop.

Drift does not model the full nonlinear behavior of an analog BBD chip and does not reproduce Memory's private algorithms. Its parameter ranges and presets are original choices. A functional comparison should assess musical roles, not assume equivalent knob values.

The demo and listening examples are synthesized from mathematical oscillators. The interface is drawn with native vector shapes. Its light ground, dark ink, red accent, straight borders and Archivo/JetBrains Mono typography follow the owner's Batchly theme. The two fonts are embedded from Google Fonts under their retained SIL Open Font Licenses. These assets require no Cymatics installation or sound library.

## Patina

The functional reference is [Cymatics Origin's public product description](https://cymatics.fm/products/origin-vintage-plugin), consulted September 26, 2026. It describes rate reduction, saturation, noise, pitch movement and chorus. Patina implements those broad musical roles with independently written code. No commercial code, factory settings, recordings, graphics or extracted assets were used.

`src/PatinaEngine.h` uses a fractional sample clock with interpolation at capture boundaries, fourth-order low-pass filters before and after rate reduction, an antiderivative form of tanh saturation, periodic multi-rate pitch modulation, fractional delay chorus and seeded generated hiss. Its fixed-rate periodic movement and original chorus are deliberate design choices, not a model of Origin's private randomizer, tape circuits or Juno chorus. The filters have finite slopes; the engine makes no brick-wall or alias-free claim. All processing storage is allocated during preparation.

The five presets and vector reel-deck artwork are original. Patina keeps Batchly's established typography and colors, with warm brown tape inside the dark transport window. `RackEngine.h` fixes the order to Drift then Patina, with shared gain and global bypass at the end. Old states are filled with defaults for new parameters, keeping Patina off when a 0.1 state is loaded.

## Atrium

The functional reference is the spatial processing described on [Cymatics Space Lite's public page](https://cymatics.fm/products/space-lite-plugin), consulted September 26, 2026. Atrium fills that broad reverb role with one original modulated room network and five original starting points. It does not reproduce Space's algorithms, modes, factory presets, pitch effects or artwork. No commercial source or binary internals were inspected or incorporated.

The intended sound ranges from a short reflective room to a long, gently moving hall. `src/AtriumEngine.h` uses a stereo pre-delay, input high-pass and all-pass diffusion, eight fractional delay lines, an energy-preserving feedback mixing matrix, per-line decay and damping, and a mid/side width control. Delay times, modulation, gains, UI graphics and presets are project choices. Decay is a nominal low-frequency target; damping and interpolation make higher frequencies decay faster. Size and pre-delay automation deliberately move delay read positions and can bend pitch. Processing storage is allocated in preparation only.

The interface uses original nested architectural frames, a room-depth illustration and an arch icon, with the collection's Batchly typography and colors. The three-effect order is Drift, Patina, Atrium. See `ATRIUM-VERIFICATION.md` for completed checks and remaining limits.

## Framework

JUCE 9.0.2 is pinned to commit `72782788ce18c2d4d760b28e0921d6ffc6431102`. It supplies the GUI, audio-device management, file reading/writing, parameter management, and VST3/standalone wrappers. It does not implement Drift's effect algorithm.

Before adding another module, record its public references, intended audible behavior, original design choices and verification results here. Sample-based instruments need independently created or redistributable sound sources. An installed commercial sound library is not automatically an open-source asset source.
