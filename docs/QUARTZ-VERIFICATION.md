# Quartz 0.12 verification

Checked on Windows x64, September 26, 2026. Quartz is the twelfth original effect.

## Automated checks

The standalone and VST3 build with pinned JUCE and no added runtime dependencies. All fourteen CTest suites pass. Quartz tests cover exact neutral/dry/off paths, independently isolated low/mid/high tone response, soft character, the fully wet sample ceiling, shared limiting of unequal stereo channels, adjustable release recovery, mono, silence, block invariance, six sample rates from 8 to 384 kHz, extreme automation, invalid input recovery and filter-tail decay.

A saved 0.11 eleven-effect state produces exactly unchanged audio, maximum difference zero. Updater integrity, damaged-package rejection, host waiting and verified replacement pass.

Independent hosting passes 126 processing and state checks. Quartz is checked at 44.1, 48 and 96 kHz for dry/wet behavior, a -6 dB sample ceiling, unequal stereo linking, mono, bypass and silence. All eleven older saved-state fixtures leave Quartz off. Edited signed EQ, release and ceiling values survive state restoration, and all five original presets render an original generated mix without clipping. Evidence: `build-next/quartz-listening/verification.json`.

## Interface and export

Actual JUCE editor components render all twelve pages in standalone and VST3 layouts. Controls fit, selected sidebar entries remain visible, and preset callbacks work. Additional tests exercise Quartz's signed Low gain and Release text conversion and parameter attachments. The rendered interface was inspected, including the crystal icon, tone settings, units and reduction meter.

Native export with Soft edges produces a stereo 48 kHz PCM-24 WAV lasting 2.23 seconds from an original two-second probe. It matches independently hosted VST3 audio within 1.2e-7 and ends with a silent final 100 ms. Source overwrite is rejected. Local evidence: `build-next/quartz-native-export-verification.json`, `build-next/quartz-legacy-verification.json` and `build-next/editor-previews`.

## Native desktop and FL Studio

The native standalone's sidebar scrolls to Quartz and selecting Dense cut enables it, setting Input 6 dB, Low 1 dB, Mid 1 dB, High 0.5 dB, Character 15%, Ceiling -3 dB, Release 220 ms and Mix 100%. The installed system VST3 matches the tested binary.

The native standalone saved `build-next/Quartz-native-stack.bapreset`; FL Studio 2024 loaded that file with the same eight values and shared Output at 0 dB. After saving `build-next/Quartz-verification.flp`, closing FL Studio normally and starting a fresh process, Quartz remained selected and enabled with Dense cut and all eight values intact. Preceding Forge, Cinder and Ember remained enabled, while Vista remained off. The verification project was then closed normally.

## Limits

Tone bands overlap with first-order slopes. The limiter bounds fully wet sample peaks after enable transitions, without lookahead or a true-peak guarantee. Dry blend and shared Output can exceed Ceiling. Strong limiting can distort transients; soft coloration is not oversampled and can alias. The display shows tone settings and measured gain reduction, not a spectrum or loudness measurement. Artistic listening approval, recorded DAW automation, full DAW-render comparison, other hosts/devices and signing remain unqualified.
