# Forge 0.8 verification

Checked on Windows x64, September 26, 2026. Forge is the eighth original effect.

## Automated checks

The standalone and VST3 build with pinned JUCE and no new runtime dependencies. All ten CTest suites pass. Forge processing checks cover 8, 44.1, 48, 96, 192 and 384 kHz; exact disabled, dry and neutral paths; onset boost and softening; sustain lift; independent low/high tone response; added harmonics; linked unequal stereo levels; mono and width zero; sample ceiling after width; tone-tail decay; block-size invariance; extreme automation and invalid input.

The independent VST3 host passes at 44.1, 48 and 96 kHz for shaping, dry mix, width, ceiling, mono, bypass and silence. All eight enable states, signed Punch and Ceiling survive state recall. Released states from 0.1 through 0.7 keep Forge off. A saved seven-effect 0.7 state renders exactly identical audio in 0.8, with maximum error zero. All five original presets render an original synthesized kick/snare/hat loop without clipping or residual tails. The nine-second render took about 0.33 seconds locally; this does not guarantee real-time performance on every machine.

Updater validation and its separate damaged-package, host-wait and replacement checks pass.

## Interface and native export

The actual JUCE editor renders all eight pages in both desktop and VST3 configurations. The checks verify control bounds, formatted values, selected-icon visibility, initial enable indicators, factory preset callbacks, Relay Time, signed Forge Punch and Forge Ceiling attachments. The Forge component snapshot was inspected: readable units, eight working controls, shared Output, strike icon and original envelope illustration.

The same native export implementation used by the app produces a 48 kHz, 24-bit stereo WAV from an original two-second probe and the Parallel iron preset. The result is 2.16 seconds including the tone-filter tail, and differs from the independent VST3 by at most 1.2e-7, one 24-bit sample step. Overwriting the source is rejected. Evidence remains locally in `build-next/forge-native-export-verification.json`, `build-next/forge-legacy-verification.json`, `build-next/forge-listening` and `build-next/editor-previews`.

## Limits and pending native checks

Forge's native desktop interaction and FL Studio save/reopen checks are pending. The Windows desktop tool repeatedly returned `foreground window did not report a process id` while finishing Relay's fresh-process recall check. Component rendering and independent VST3 recall do not establish those native interaction results. The system-installed VST3 remains 0.7 until the test host can close normally and installation can be verified.

Ceiling bounds the fully wet sample peaks before shared Output. Dry mix, output gain and inter-sample peaks can exceed it. Saturation is not oversampled and can alias. There is no lookahead, true-peak guarantee, full Diablo mode/EQ recreation, or sample-equivalence claim. Listening approval, recorded DAW automation, other hosts/devices and code signing remain unqualified.
