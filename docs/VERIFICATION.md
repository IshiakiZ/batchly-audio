# Verification record

Local Windows x64 preview checked September 26, 2026. This is the first module, Drift, not the complete 36-plugin collection.

## Passed

- Visual Studio 2022 Release build of the desktop app and VST3 using JUCE 9.0.2, pinned in CMake. The C++ runtime is linked into the binaries.
- CTest DSP checks at 8, 44.1, 48, 96 and 192 kHz: exact dry signal, bypass, stereo movement, finite output at extreme settings, silent output when noise is off, mono processing, identical results across buffer sizes, and recovery from invalid input values.
- Independent Pedalboard host loaded the actual VST3 binary at 44.1, 48 and 96 kHz. Dry, wet, stereo width, bypass, mono and edited parameter state recall passed. Noise-off silence remained exactly silent.
- Original 12-second listening examples rendered without clipping. The default chorus example peaked at approximately 0.131 linear amplitude.
- Native desktop interface inspected on screen: labels, units, knobs, controls and display are readable. The demo and file player generated output on the meter. The movement display reflects the engine's modulation.
- Installation into the standard system VST3 folder completed through Windows administrator approval. Installed files matched the build hashes. FL Studio 24.2.2 scanned Batchly Audio with **Verify plugins** enabled and reported **ok / VST3 / 64 / Effect**.
- FL Studio loaded Drift on the master mixer and displayed its native controls. Dragging Depth changed the host-reported parameter to 0.55 and the interface to 55%.
- A separate FL Studio project was saved, FL Studio was closed, the VST3 was updated to the build with the Updates button, and the project was reopened in a fresh FL Studio process. Drift restored Depth to 55% and the edited-preset marker correctly.
- Update tests cover numeric version ordering, stable/preview selection, foreign download URL rejection, required SHA-256 data, malformed replies and modified download rejection. A separate helper test rejected a damaged package, preserved the installed file while a test host remained open, and replaced and verified a portable app after that host exited.
- The desktop file dialog opened an original WAV. Export produced a stereo 48 kHz, 24-bit PCM WAV with the intended 80 ms tail. Its first 12 seconds matched the independent VST3 render within 0.0000001193, one 24-bit sample step. Export was checked before the final preset-marker and filename protection changes; the DSP code was unchanged afterward.

The exact commands to reproduce the automated checks are in the README. `tools/verify_plugin.py` generates its own audio and JSON report, without commercial plugins or sound libraries. Automated assertions are processing checks, not a substitute for listening.

## Still unverified

- Recorded FL Studio automation curves and a full DAW render comparison. A custom search path did not discover the VST3; the standard system location is required by this FL Studio version.
- Other DAWs, other computers, fresh Windows installations and non-Windows builds.
- Listening-based sound approval and a matched musical comparison with the reference. No sample-for-sample emulation claim is made.
- Signed installer and signed binaries, enterprise-managed PowerShell restrictions, and a multi-module rack. The portable preview includes one processor. Administrator cancellation and every possible multi-DAW file-lock combination are not covered by the automated tests.

No Batchly deployment or hosting changes are part of this preview.
