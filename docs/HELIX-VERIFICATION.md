# Helix 0.5 verification

Checked on Windows x64, September 26, 2026. Helix is the fifth original effect.

## Automated checks

The standalone app and VST3 build with the pinned JUCE version and no new runtime dependencies. All six CTest suites pass. Helix checks cover 8, 44.1, 48, 96, 192 and 384 kHz; exact disabled/dry paths; mono isolation; matched stereo sweeps at zero width; silent input; bounded output; identical output with different processing block sizes; static all-pass magnitude; dry/wet phase-cancellation notches; feedback tail decay; rapid extreme automation; and invalid input recovery.

Independent VST3 tests at 44.1, 48 and 96 kHz verify dry/wet, moving stereo phases, width, mono, global bypass and silence. Width-zero channel comparisons allow 1e-7 floating-point rounding tolerance. All five effect states and signed feedback survive recall. Released states from 0.1 through 0.4 disable Helix. The released 0.4 four-effect state produces exactly identical output in the new build.

All five original presets render without clipping. One local 14-second render took about 0.41 seconds; this is not a real-time guarantee. The updater rejects damaged packages, waits for host exit and verifies installed files.

## Native interface

On-screen inspection confirmed the five icons, per-effect state, readable signed feedback, rate/frequency units and original orbital display. The collection fits five effects and scrolls when it grows further; selecting a module reveals its sidebar button.

A native check found that the earlier timer-driven preset label could overwrite a pending menu selection before JUCE delivered its asynchronous callback. The label now updates only when the processor's program or modified status changes, with popup activity respected. Selecting Hollow metal now correctly applies its 0.16 Hz rate, -65% feedback and remaining settings, rather than returning to Slow orbit.

Native Save wrote the correct Hollow metal settings, including negative feedback, confirmed by inspecting the preset XML. Native Open/Export rendered the original 12-second source with Atrium, Chime (octave 4) and Helix enabled. The 48 kHz stereo 24-bit WAV is 22.574 seconds including serial tails, and matches an independent VST3 render within 1.2e-7. Evidence is local at `build-next/helix-native-export-verification.json` and `build-next/helix-listening/01-original - Batchly.wav`.

## FL Studio

The system VST3 opened the previous Chime project with its selected page, C minor octave 4 and enabled Chime/Atrium intact; Helix defaulted off. Loading the native Hollow metal preset restored -65% feedback, 0.16 Hz rate, the Helix page and Atrium/Chime/Helix enabled. The project was saved and reopened in a fresh FL Studio 24.2.2 process; those states persisted. The local project is `build-next/Helix-verification.flp`.

## Limits

Artistic listening approval, reference equivalence, recorded DAW automation, broader audio devices/hosts and code signing remain unqualified. Rate is free-running Hz; no tempo synchronization is claimed. The orbital display shows modulation positions, not a measured spectrum. Fully wet phasing has a different sound from a half-wet notch blend.
