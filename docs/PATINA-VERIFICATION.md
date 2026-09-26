# Patina 0.2 verification

Checked on Windows x64, September 26, 2026. This release adds the second effect; it is not the complete reference collection.

## Automated checks passed

- Drift's existing DSP and independent VST3 checks remain green.
- Patina DSP checks at 8, 44.1, 48, 96 and 192 kHz cover dry/off passthrough, stereo chorus, finite output at extreme settings, noise-off silence and bounded generated hiss.
- A 4 kHz internal rate suppresses a 9 kHz input by more than 30 dB relative to a 400 Hz input. This checks the intended low-rate filtering rather than a commercial reference match.
- Rendering is identical across different host block sizes. NaN and infinity parameter/input probes do not poison subsequent output.
- The upgraded rack's Drift-only audio matches the original Drift engine within 0.0000001 with nonzero output attenuation. Global bypass bypasses both modules.
- Pedalboard loaded the compiled VST3 at 44.1, 48 and 96 kHz and verified Patina dry/wet paths, stereo, mono, global bypass, silence and rack state recall.
- An actual 0.1.0 state fixture was loaded after Patina was active: Depth 61% and Mix 42% were restored, Drift was enabled, and Patina was disabled and reset to defaults.
- Original Patina and combined-rack listening files rendered without clipping. A local 12-second Patina render took approximately 0.10 seconds; this is a single-machine observation, not a real-time guarantee.
- Update parsing/integrity tests and the installation helper's damaged-package, host-waiting and portable replacement tests passed with the 0.2 build.

Run the README checks against the selected build directory. `tools/package.py --build-dir build-patina` and `tools/verify_updater.ps1 -BuildDirectory build-patina` support the separate local build used for this release.

## Native app and FL Studio checks

The native desktop interface was inspected on screen. Drift and Patina have distinct sidebar icons and correctly formatted controls. Switching pages preserved the on/off states; Patina's demo generated metered output and animated the reels. The icon and text changes do not alter the audio engine.

The VST3 was installed in the standard system folder, and its hash matched the build. FL Studio 24.2.2 reopened a copy of the 0.1 verification project: Drift restored Depth 55%, and Patina stayed off. Patina was then enabled in the DAW, Wear was changed to 47.6%, and the project was saved. A fresh FL Studio process restored both enabled effects, the Patina page and Wear 47.6%.

## Verification limits

Listening-based sound approval, reference matching, recorded DAW automation, native two-effect WAV export and preset-file dialog round trips, broader host/device testing and code signing remain unverified. The source implements similar musical functions with its own sound; it does not claim sample-for-sample commercial emulation.
