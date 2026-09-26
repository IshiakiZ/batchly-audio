# Chime 0.4 verification

Checked on Windows x64, September 26, 2026. Chime is the fourth original effect.

## Automated checks

The Release standalone app and VST3 build against the pinned JUCE source with no new runtime dependencies. All five CTest suites pass, including the existing three engines and update validation.

Chime tests cover 8, 44.1, 48, 96 and 192 kHz; exact disabled/dry passthrough; silence; bounded stereo ringing; mono channel isolation; centered width zero; identical output across 257- and 509-sample blocks; spectral discrimination of C3 and a changed D4 root; short and long tail decay; rapidly changing extreme controls and scales; non-finite input/parameter recovery; global rack bypass; and additive serial tail allowances.

The independent Pedalboard host passes dry/wet, stereo, width, mono, bypass and silence checks at 44.1, 48 and 96 kHz. All four enabled states, root, scale, octave and edited Ring survive VST3 state recall. Actual released 0.1, 0.2 and 0.3 states disable Chime. A direct comparison to the released 0.3 binary produced exactly identical audio from its saved three-effect state. All fifteen earlier factory programs retain their original parameter values.

All five new presets render original listening examples without clipping. One local 25-second Chime render took about 0.58 seconds; this is not a real-time performance guarantee. Tests use mathematical input and do not load commercial plugins. The update helper rejects damaged staged files, waits for the host to close and verifies replacement files.

## Native app

The actual interface was inspected on screen: four distinct icons, per-effect on/off status, tuning controls, clear units and the original string display. Changing Oct 3 to Oct 4 updates the modified-preset marker. Native Save wrote octave 4 with Chime and Atrium enabled, confirmed by reading the saved XML.

Native Open/Export processed the original twelve-second stereo file with Atrium defaults and Chime Glass strings at octave 4. The exported WAV is 48 kHz, stereo, 24-bit, 21.074 seconds, including both tails. Its samples match the independently hosted VST3 within 1.2e-7, one 24-bit quantization step. Audio continues after the source ends (tail peak 0.0021). Local evidence: `build-next/native-export-verification.json` and `build-next/listening/01-original - Batchly.wav`.

## FL Studio

The installed VST3 opened a copy of the earlier Atrium project in FL Studio 24.2.2 with the original three modules enabled, edited 4.33-second decay retained, and Chime off. The native preset then loaded in FL Studio with Chime and Atrium on, C minor at octave 4, and the other modules off. Saving and reopening the project in a fresh FL Studio process retained those states and the selected Chime page. The test project is `build-next/Chime-verification.flp`.

## Limits

Listening-based artistic approval, reference matching, MIDI behavior, recorded DAW automation, broader hosts/devices and code signing are not qualified. Chime implements the harmonic-resonator role with its own controls and six scales, not Gamma's complete feature set. Key changes can intentionally glide ringing notes. Release and update-download evidence is recorded with the published preview.
