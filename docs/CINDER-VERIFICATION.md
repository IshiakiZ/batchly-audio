# Cinder 0.9 verification

Checked on Windows x64, September 26, 2026. Cinder is the ninth original effect.

## Automated checks

The standalone and VST3 build with pinned JUCE and no added runtime dependencies. All eleven CTest suites pass. Cinder tests cover 8, 44.1, 48, 96, 192 and 384 kHz; exact off/dry/zero-addition paths; zero-drive silence in the tonal layer; silence with no prior input; deterministic noise and identical results across block sizes; independent tone/noise frequency selection; release and tail bounds; stereo width and intermediate-width power balance; preservation of the dry stereo image; mono; extreme automation and invalid input.

The actual VST3 passes independent checks at 44.1, 48 and 96 kHz for dry mix, texture, stereo spread, width zero, mono, bypass and idle silence. All nine enable states and edited focus/release settings survive state recall. States from 0.1 through 0.8 disable Cinder. A released eight-effect 0.8 state produces exactly unchanged audio, maximum difference zero. All five presets render original drum audio with decayed noise tails and no clipping.

Updater validation, damaged-package rejection, host waiting and verified replacement pass.

## Interface and export

The actual JUCE editor components render all nine pages in desktop and VST3 layouts. Controls fit, selected tabs remain visible, initial enable indicators agree with the recalled preset, and preset callbacks work. The tests also exercise Cinder's Noise Focus text conversion in kHz and Decay conversion in seconds. The native component snapshot was inspected: grain icon, separate band controls, readable units and original dotted texture display.

The app's native export code processed an original two-second stereo probe with Rough edge. Its 48 kHz, 24-bit WAV lasts 3.18 seconds including the tail, matches the independent VST3 within 1.2e-7 and ends with a final 100 ms peak of one 24-bit sample step. Source overwrite is rejected. Evidence remains locally in `build-next/cinder-native-export-verification.json`, `build-next/cinder-legacy-verification.json`, `build-next/cinder-listening` and `build-next/editor-previews`.

## Pending native checks and limits

Desktop interaction, system VST3 installation and FL Studio save/reopen checks remain pending. The desktop tool repeatedly reports `foreground window did not report a process id`; the owner has been asked to restore a targetable foreground window. Actual component rendering and independent host tests do not establish those native results. The system-installed VST3 is still 0.7 while the existing test host remains open.

Noise intentionally continues after input according to Decay. Its synthesized sequence is repeatable on reset. The displayed bands illustrate settings and activity rather than a measured spectrum. Tone uses broad filters and non-oversampled nonlinear coloration, which can alias. Adding layers can raise level, so the normal export clipping check still applies. Artistic listening approval, recorded DAW automation, other hosts/devices and signing remain unqualified.
