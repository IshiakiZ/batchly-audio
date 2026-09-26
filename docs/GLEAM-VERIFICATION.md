# Gleam 0.6 verification

Checked on Windows x64, September 26, 2026. Gleam is the sixth original effect.

## Automated audio checks

The standalone app and VST3 build with pinned JUCE and no new runtime dependencies. All seven CTest suites pass. Gleam checks cover 8, 44.1, 48, 96, 192 and 384 kHz; neutral, disabled and dry paths; silent input; bass retention; high-frequency lift; focus movement; peak taming; generated harmonics; added-layer width; mono isolation; block-size independence; extreme automation; and non-finite input recovery.

Independent VST3 checks at 44.1, 48 and 96 kHz verify brightness, stereo/mono behavior, bypass and silence. All six effect states survive recall. Released states from 0.1 through 0.5 disable Gleam. The released 0.5 state produces exactly identical audio in this build. All five Gleam presets render without clipping. One local 13-second render took about 0.37 seconds, which is not a real-time guarantee. Updater validation and replacement tests pass.

## Native interface and export

The sixth sidebar icon is reachable by scrolling; loading a preset reveals the selected effect. The original prism display, dB/frequency units, all eight controls and Drum shine preset were inspected on screen. Native Save wrote 4 dB Presence, 5 dB Air, 6.5 kHz Focus, 30% Excite, 60% Tame, 115% Width, -4 dB Trim and full Mix, with the previous rack state intact.

Native Open/Export processed the original 12-second input with Atrium, Chime, Helix and Gleam enabled. The 48 kHz stereo 24-bit WAV is 22.654 seconds including tails. It matches the independent VST3 render within 1.2e-7. Evidence is local at `build-next/gleam-native-export-verification.json` and `build-next/gleam-listening/01-original - Batchly.wav`.

## Limits

Artistic listening approval, exact reference equivalence, recorded DAW automation, broader devices/hosts and code signing remain unqualified. Tame attenuates only the added brightness, not the original signal. The broad overlapping bands are not surgical EQ, and the nonlinear layer is not claimed to be alias-free. The display represents measured level and reduction rather than a spectrum.

## FL Studio

The installed 0.6 VST3 opened the previous Helix project with Hollow metal and its existing enabled effects intact. Native Load restored the Gleam preset, selected its page and scrolled its icon into view. Saving and reopening in a fresh FL Studio 24.2.2 process preserved Drum shine, all eight values, the selected page and enabled Atrium/Chime/Helix/Gleam. The local project is `build-next/Gleam-verification.flp`.
