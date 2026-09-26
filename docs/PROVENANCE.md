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

## Chime

Gamma's installed VST3 exposes a harmonic resonator control surface: root, scale, scale degrees, octave range, ringing/decay and modulation. Its public host parameter names were inspected on September 26, 2026. The [official Gamma Lite page](https://cymatics.fm/products/gamma-lite) returned the site's password page, so it was not used as a full specification. No protection was bypassed and no binary internals or commercial source were read. Public host metadata is interface information, not a copied implementation.

`src/ChimeEngine.h` independently implements fourteen damped complex rotations, arranged as seven possible scale degrees in two registers. Six scale choices, energy-scaled excitation, input saturation, per-voice stereo positioning, gentle detuning/motion and an output low-pass provide the musical behavior. Complex rotation contracts the state each sample, keeping rapid retuning stable. Tuning and controls are smoothed; processing uses fixed arrays without allocation. The output saturation bounds resonant peaks. This is a core harmonic-resonance counterpart, not a recreation of Gamma's MIDI, arpeggiator, drone or spectral-selection features.

The five preset settings, tuned-string illustration and hanging-bar icon are original. Chime follows Atrium in the fixed rack. Existing states default Chime to off. The collection navigation now scrolls and displays each effect's state on its button. `ModuleCatalog.h` centralizes the existing program names and values without changing the first fifteen programs.

See `CHIME-VERIFICATION.md` for completed checks and limits.

## Helix

The [official Space Cadet description](https://cymatics.fm/pages/cymatics-space-cadet-free-download), consulted September 26, 2026, describes stereo phasing with rate, depth, feedback, tone, width, drive and blend. Helix implements that musical role using an independently written eight-stage all-pass cascade in `src/HelixEngine.h`. No commercial plugin source, binary internals, factory presets, audio or graphics were used.

Original choices include staggered stage frequencies, sine sweeps across a logarithmic range, a continuously adjustable stereo phase offset, signed and bounded nonlinear feedback, soft input drive, output tone and smoothed parameters. The fixed processing arrays allocate no memory during audio processing. Fully wet processing rotates phase; blending the dry input creates moving notches. The five presets, intertwined-loop icon and paired orbital display are original. Helix runs after Chime and defaults off in older states.

See `HELIX-VERIFICATION.md` for the checks and remaining limits.

## Gleam

The [official Cymatics site](https://cymatics.fm/) identifies Halo's musical role as adding air, clarity and high-end shine. Its installed VST3 host interface was inspected on September 26, 2026, and original impulse probes confirmed upper-frequency enhancement without a sustained reverb tail. Those observations establish a broad role only. No private algorithm, code, factory preset, artwork or reference-processed audio is incorporated or distributed.

`src/GleamEngine.h` uses independently designed parallel first-order high-frequency layers: a fixed presence region and a variable air region. Trapezoidal integrators give complementary high-pass paths with unity response at Nyquist. Original soft-curve excitation adds harmonic content and is filtered again to reduce bass leakage. Linked peak detection attenuates only the added layer, with width, trim and dry/wet blending afterward. The fixed arrays allocate no processing memory. This is a presence/air enhancer, not a recreation of Halo's named macro chains. The nonlinear path is not claimed to be alias-free.

The five presets, prism/ray artwork and diamond icon are original. Gleam follows Helix, with old states keeping it off. See `GLEAM-VERIFICATION.md` for actual checks and limits.

## Relay

The [official Illusion description](https://cymatics.fm/products/illusion-creative-delay-2025), consulted September 26, 2026, describes echo timing, feedback, filtering, pitch movement, glide and ping-pong behavior. Relay fills that broad creative-delay role with an independently written fractional stereo delay. No commercial code, factory settings, graphics, recordings or binary internals were used.

`src/RelayEngine.h` uses two preallocated circular buffers, linear interpolation, a sine-modulated read distance, smoothed time glide, low-pass and bass-removal filters, a convex stereo feedback matrix and a bounded soft write curve. At full bounce the mono input enters the left buffer first and feedback alternates channels. Disabled history is invalidated without clearing multi-second buffers on the audio thread. Tail allowance follows the longest repeat interval and a nominal -80 dB feedback decay. This is not a BBD circuit emulation, tempo-synchronized delay or pitch sequencer.

The five presets, repeated-block display and staggered echo icon are original. Relay follows Gleam; old states keep it off. See `RELAY-VERIFICATION.md` for checked behavior and limits.

## Framework

JUCE 9.0.2 is pinned to commit `72782788ce18c2d4d760b28e0921d6ffc6431102`. It supplies the GUI, audio-device management, file reading/writing, parameter management, and VST3/standalone wrappers. It does not implement Drift's effect algorithm.

Before adding another module, record its public references, intended audible behavior, original design choices and verification results here. Sample-based instruments need independently created or redistributable sound sources. An installed commercial sound library is not automatically an open-source asset source.
