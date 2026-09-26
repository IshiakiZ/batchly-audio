# Cinder 0.9 verification

Checked on Windows x64, September 26, 2026. Cinder is the ninth original effect.

## Automated checks

The standalone and VST3 build with pinned JUCE and no added runtime dependencies. All eleven CTest suites pass. Cinder tests cover 8, 44.1, 48, 96, 192 and 384 kHz; exact off/dry/zero-addition paths; zero-drive silence in the tonal layer; silence with no prior input; deterministic noise and identical results across block sizes; independent tone/noise frequency selection; release and tail bounds; stereo width and intermediate-width power balance; preservation of the dry stereo image; mono; extreme automation and invalid input.

The actual VST3 passes independent checks at 44.1, 48 and 96 kHz for dry mix, texture, stereo spread, width zero, mono, bypass and idle silence. All nine enable states and edited focus/release settings survive state recall. States from 0.1 through 0.8 disable Cinder. A released eight-effect 0.8 state produces exactly unchanged audio, maximum difference zero. All five presets render original drum audio with decayed noise tails and no clipping.

Updater validation, damaged-package rejection, host waiting and verified replacement pass.

## Interface and export

The actual JUCE editor components render all nine pages in desktop and VST3 layouts. Controls fit, selected tabs remain visible, initial enable indicators agree with the recalled preset, and preset callbacks work. The tests also exercise Cinder's Noise Focus text conversion in kHz and Decay conversion in seconds. The native component snapshot was inspected: grain icon, separate band controls, readable units and original dotted texture display.

The app's native export code processed an original two-second stereo probe with Rough edge. Its 48 kHz, 24-bit WAV lasts 3.18 seconds including the tail, matches the independent VST3 within 1.2e-7 and ends with a final 100 ms peak of one 24-bit sample step. Source overwrite is rejected. Evidence remains locally in `build-next/cinder-native-export-verification.json`, `build-next/cinder-legacy-verification.json`, `build-next/cinder-listening` and `build-next/editor-previews`.

## Native desktop and FL Studio

The desktop connection was restored and the 0.9 standalone app was inspected on screen. Selecting Ash cloud applied Grit 10%, Noise 65%, Tone Focus 3 kHz, Noise Focus 4.2 kHz, Drive 20%, Decay 0.6 s, Width 100% and Mix 65%. The Forge and Cinder sidebar icons, scroll navigation, units and preset selection worked. The native Save dialog wrote `build-next/Cinder-native-stack.bapreset`, which the native Load dialog successfully restored in FL Studio.

The entire 0.9 VST3 bundle was installed in the standard system folder and verified against the build. FL Studio first opened the older Relay project with its state intact. The combined rack was loaded and saved as `build-next/Cinder-verification.flp`; after normal exit, a fresh FL Studio 24.2.2 process restored Cinder's Ash cloud settings, Forge's Soft mallet settings, their enable states and the selected Cinder page. Evidence: `build-next/Cinder-native-preview.jpg`, `build-next/Cinder-FL-recall.jpg` and `build-next/Forge-FL-recall.jpg`. Both verification apps were closed normally afterward.

## Limits

Noise intentionally continues after input according to Decay. Its synthesized sequence is repeatable on reset. The displayed bands illustrate settings and activity rather than a measured spectrum. Tone uses broad filters and non-oversampled nonlinear coloration, which can alias. Adding layers can raise level, so the normal export clipping check still applies. Artistic listening approval, recorded DAW automation, other hosts/devices and signing remain unqualified.
