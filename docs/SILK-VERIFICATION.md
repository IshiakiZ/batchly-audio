# Silk 0.13 verification

Windows x64 checks on September 26, 2026. Silk is the thirteenth original musical-role counterpart.

## DSP and compatibility

The engine tests isolated resonance reduction, coverage across the frequency range, protected bass, broadband-noise preservation, exact dry/off/zero-depth paths, matched and unequal stereo linking, mono, silence, removed-signal reconstruction, attack response, filter-tail decay, block invariance, six sample rates from 8 to 384 kHz, automated controls and invalid-input recovery. At the tested 1.8 kHz resonance with Depth 18 dB and Selectivity 15%, settled output energy is below one quarter of input energy; this does not establish the same reduction at every frequency.

All fifteen CTest suites pass. Independent VST3 hosting passes 143 checks, including Silk processing and difference reconstruction at 44.1, 48 and 96 kHz, all twelve older state fixtures, and edited frequency, timing, signed trim and audition recall. Five original synthesized listening examples render without clipping or a lingering tail. The saved 0.12 twelve-effect state produces exactly unchanged audio, with maximum difference zero.

Actual editor components render all thirteen pages with working controls and visible selected sidebar entries. Silk's frequency, attack, signed trim and audition attachments pass, including resetting audition through a factory preset. Its rendered interface was visually inspected. Native Soft fabric export produces a 48 kHz stereo PCM-24 WAV lasting 3.08 seconds from a two-second probe. It matches the independently hosted VST3 within 1.2e-7 and ends with a silent final 100 ms. Source overwrite is rejected.

The updater now publishes whole status JSON atomically and tolerates a temporary reader lock. Its regression test holds that lock, confirms waiting for the host, rejects a damaged package, verifies installed bytes and checks the Windows PowerShell rollback replacement primitive.

Local evidence: `build-next/silk-listening/verification.json`, `build-next/silk-legacy-verification.json`, `build-next/silk-native-export-verification.json` and `build-next/editor-previews`.

## Native desktop and FL Studio

The native standalone scrolls to Silk and applies Vocal ease with Depth 10 dB, Selectivity 25%, Low 1.2 kHz, High 10 kHz, Attack 6 ms, Release 150 ms, Trim 0 dB and Mix 100%. Audition switches on and off and the preset menu correctly indicates an edited state. The app saves `build-next/Silk-native-stack.bapreset` with audition off and shared Output 0 dB.

FL Studio 2024 loaded that preset, preserving all eight values, Silk enabled and audition off. After saving `build-next/Silk-verification.flp`, closing normally and reopening in a fresh process, the selected Silk page, Vocal ease values and preceding rack states remained intact. Cinder, Ember and Quartz remained enabled; Vista remained off. The final system VST3 matches the rebuilt binary byte for byte. The verification project was closed normally.

## Limits

Fixed overlapping notches produce frequency-dependent cuts and phase rotation. Requested Depth is a per-band limit rather than a guarantee for the combined response. Detection averages about 12 ms before the adjustable attack stage; no lookahead or playback latency is added. The displayed values are measured detector cut requests, not a full spectrum. Wanted tonal notes can trigger suppression. Extreme settings can change transients and timbre. External sidechain, mid/side modes, artistic listening approval, recorded DAW automation, full DAW-render comparison, other hosts/devices and signing are not qualified.
