# Forge 0.8 verification

Checked on Windows x64, September 26, 2026. Forge is the eighth original effect.

## Automated checks

The standalone and VST3 build with pinned JUCE and no new runtime dependencies. All ten CTest suites pass. Forge processing checks cover 8, 44.1, 48, 96, 192 and 384 kHz; exact disabled, dry and neutral paths; onset boost and softening; sustain lift; independent low/high tone response; added harmonics; linked unequal stereo levels; mono and width zero; sample ceiling after width; tone-tail decay; block-size invariance; extreme automation and invalid input.

The independent VST3 host passes at 44.1, 48 and 96 kHz for shaping, dry mix, width, ceiling, mono, bypass and silence. All eight enable states, signed Punch and Ceiling survive state recall. Released states from 0.1 through 0.7 keep Forge off. A saved seven-effect 0.7 state renders exactly identical audio in 0.8, with maximum error zero. All five original presets render an original synthesized kick/snare/hat loop without clipping or residual tails. The nine-second render took about 0.33 seconds locally; this does not guarantee real-time performance on every machine.

Updater validation and its separate damaged-package, host-wait and replacement checks pass.

## Interface and native export

The actual JUCE editor renders all eight pages in both desktop and VST3 configurations. The checks verify control bounds, formatted values, selected-icon visibility, initial enable indicators, factory preset callbacks, Relay Time, signed Forge Punch and Forge Ceiling attachments. The Forge component snapshot was inspected: readable units, eight working controls, shared Output, strike icon and original envelope illustration.

The same native export implementation used by the app produces a 48 kHz, 24-bit stereo WAV from an original two-second probe and the Parallel iron preset. The result is 2.16 seconds including the tone-filter tail, and differs from the independent VST3 by at most 1.2e-7, one 24-bit sample step. Overwriting the source is rejected. Evidence remains locally in `build-next/forge-native-export-verification.json`, `build-next/forge-legacy-verification.json`, `build-next/forge-listening` and `build-next/editor-previews`.

## Native follow-up in 0.9 and limits

The unchanged Forge engine and page were checked on screen in the 0.9 collection after the desktop connection was restored. Selecting Soft mallet applied Punch -50%, Body 15%, Weight 2 dB, Edge 0 dB, Drive 5%, Ceiling -2 dB, Width 100% and Mix 100%. The native Save dialog wrote the combined rack preset; FL Studio loaded it. The separate Cinder-verification.flp project was saved, the host exited normally, and a fresh FL Studio 24.2.2 process restored Forge with every expected setting and its enable state. Screenshots are `build-next/Forge-native-preview.jpg` and `build-next/Forge-FL-recall.jpg`. The system VST3 is now the verified 0.9 build. This follow-up qualifies Forge within 0.9, rather than claiming the 0.8 executable was separately tested in FL Studio.

Ceiling bounds the fully wet sample peaks before shared Output. Dry mix, output gain and inter-sample peaks can exceed it. Saturation is not oversampled and can alias. There is no lookahead, true-peak guarantee, full Diablo mode/EQ recreation, or sample-equivalence claim. Listening approval, recorded DAW automation, other hosts/devices and code signing remain unqualified.
