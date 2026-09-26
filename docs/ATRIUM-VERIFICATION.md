# Atrium 0.3 verification

Checked on Windows x64, September 26, 2026. Atrium is the third original effect in this collection.

## Automated checks passed

- Release app and VST3 build using the pinned JUCE source. No new runtime dependencies.
- Four CTest suites: original Drift, Patina, Atrium and update validation.
- Atrium tests at 8, 44.1, 48, 96, 192 and 384 kHz: exact off/dry passthrough, silent input, bounded stereo tail, and centered wet output at Width zero.
- A 73 ms pre-delay shifts the impulse response by 3,504 samples at 48 kHz. Fractional ring-buffer interpolation retains its precision across wraps.
- A longer decay sustains more late energy; the four-second setting loses more than 20 dB of integrated energy between the early and late test windows. Low cut suppresses a 60 Hz input. Decay remains a nominal low-frequency target, not an exact broadband RT60 guarantee.
- Identical output across full-buffer and 137-sample blocks, mono/summed-stereo parity, rapidly automated extreme controls, non-finite input recovery and full-rack bypass.
- Independent Pedalboard host at 44.1, 48 and 96 kHz: Atrium dry/wet, stereo, width, mono, bypass and silence; three-effect state recall; actual released 0.1 and 0.2 state migration with Atrium off.
- A separate comparison against the actual 0.2 binary produced exactly the same stereo audio after loading its two-effect state. The captured 0.2 preset is retained in `tests/fixtures/patina-0.2.0.bapreset`.
- Original listening examples for Atrium and the full rack include tails and do not clip. One local 18-second reverb render took about 0.19 seconds; this is not a real-time guarantee.
- Update helper rejects damaged files, waits for the host to close and verifies the installed portable binary.

## Native desktop checks

The actual app was inspected on screen: all three sidebar icons, readable controls and units, room illustration, independent enabled states, demo output and live wet meter. The native file picker loaded the original 12-second stereo test file. Export WAV saved a 48 kHz, stereo, 24-bit file of 17.624 seconds, with nonzero reverb samples continuing after the source ends. It matched the independently hosted VST3 to within 1.2e-7, one 24-bit quantization step. Native Save/Load preset dialogs restored Atrium after it was switched off.

The native export evidence is local at `build-atrium/native-export-verification.json`; generated audio is under `build-atrium/listening/`. Reproduce with Atrium's Open atrium preset, Drift and Patina off, and Export WAV. The WAV should be the source duration plus 5.624 seconds.

## FL Studio

The system VST3 was installed and its SHA-256 matched the build. FL Studio 24.2.2 opened a copy of the earlier two-effect project: Patina remained enabled with Wear at 47.6%, Drift remained enabled, and Atrium was off. Atrium was enabled, Decay was dragged to 4.33 seconds, and the project was saved. A fresh FL Studio process reopened the saved project with all three effects enabled, the Atrium page selected and the edited 4.33-second decay restored. The local test project is `build-atrium/Atrium-verification.flp`; the older project was preserved.

## Limits

Listening-based sound approval, reference matching, recorded DAW automation, broader hosts/audio devices and code signing remain unverified. Size and pre-delay automation can deliberately bend pitch. Atrium is one original room algorithm, not the full commercial reference's collection of reverb and pitch modes.
