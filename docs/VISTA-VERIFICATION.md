# Vista 0.11 verification

Checked on Windows x64, September 26, 2026. Vista is the eleventh original effect.

## Automated checks

The standalone and VST3 build with pinned JUCE and no added runtime dependencies. All thirteen CTest suites pass. Vista tests cover exact off/dry/neutral paths, three-band width changes, zero-width centering, doubled side gain, bass/treble separation, generated stereo from dual mono, preservation of left-plus-right, untouched mono host tracks, silence, block invariance, six sample rates from 8 to 384 kHz, extreme automation, invalid stereo input and tail decay.

The independent VST3 host passes 110 collection checks. Vista is checked at 44.1, 48 and 96 kHz, including mono fold preservation, stereo generation, mono tracks and bypass. All eleven enable states and edited width/crossover/delay controls survive state recall. Ten older state fixtures disable Vista. A released 0.10 ten-effect state produces exactly unchanged audio, maximum difference zero. All five presets process original synthesized stereo audio without clipping, while preserving its mono sum and ending with a silent tail.

Updater integrity, damaged-package rejection, host waiting and verified replacement pass.

## Interface and export

Actual JUCE editor components render all eleven pages in standalone and VST3 layouts. Controls fit, selected sidebar entries remain visible, and preset callbacks work. Additional attachment tests cover Vista's Delay in milliseconds and width values above 100%. The rendered interface was inspected, including the outward-arrow icon, three width regions, crossover units and Mid/Side meter labels.

Native export with Air frame produces a stereo 48 kHz PCM-24 WAV approximately 2.58 seconds long from an original two-second probe. It matches independently hosted VST3 audio within 1.2e-7 and ends with a silent final 100 ms. Source overwrite is rejected. Local evidence: `build-next/vista-native-export-verification.json`, `build-next/vista-legacy-verification.json`, `build-next/vista-listening` and `build-next/editor-previews`.

## Native FL Studio

The complete 0.11 VST3 bundle was installed in the standard system location. FL Studio 24.2.2 loaded the prior Ember project with its controls intact and Vista off. The native sidebar scrolled to Vista and selecting Mono bloom enabled it, setting all three widths to 100%, Low Split to 250 Hz, High Split to 3.5 kHz, Spread to 65%, Delay to 17 ms and Mix to 100%.

The project was saved as `build-next/Vista-verification.flp` and FL Studio exited normally. A fresh FL Studio process restored all eight Mono bloom controls, Vista's enabled state and selected page, and the earlier enabled effects. The verification host was then closed normally.

## Limits

The width bands use broad first-order transitions. Stereo widening can raise individual channel peaks even while the mono sum stays unchanged. Changing Delay deliberately modulates the generated layer. Mono preservation applies to Vista's output; downstream nonlinear or unequal channel processing can change it. The graphic shows width settings and measured activity rather than a vectorscope. Vista's standalone window and preset-file dialogs were not separately driven in this version; component rendering and native export were checked. Artistic listening approval, recorded DAW automation, full DAW-render comparison, other hosts/devices and signing remain unqualified.
