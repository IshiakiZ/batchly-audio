# Ember 0.10 verification

Checked on Windows x64, September 26, 2026. Ember is the tenth original effect.

## Automated checks

The standalone and VST3 build with pinned JUCE and no new runtime dependencies. All twelve CTest suites pass. Ember tests compare analytic curve averages with independent numerical integration, verify distinct harmonic generation across four shapes, even harmonics from Bias, settled DC removal, original-bass restoration, Color and Filter response, and a reduced first folded harmonic relative to the fundamental for a specific hot 9 kHz probe. The spectral test is limited evidence of alias reduction, not an alias-free claim.

Checks cover 8, 44.1, 48, 96, 192 and 384 kHz, exact off/dry paths, matched stereo, mono, bias-safe silence, block invariance, extreme automation, invalid input recovery and filter tail decay. The independent VST3 host passes 95 collection checks including Ember at 44.1, 48 and 96 kHz, ten-effect state recall, signed Color and Shape/Bias recall. All nine older state fixtures disable Ember. A released 0.9 nine-effect state produces exactly unchanged audio, maximum difference zero.

Five presets process independently synthesized bass notes without clipping or residual tails. Updater package-integrity checks, damaged-package rejection, host waiting and verified replacement pass.

## Interface and export

The real JUCE editor components render all ten modules in standalone and VST3 layouts. Controls fit, selected sidebar entries remain visible, preset callbacks work, and Ember Drive, signed Color and Trim controls update their parameters with correct units. The native component image was inspected, including its original flame icon and curve display.

The native export function processes an original two-second stereo probe with Quiet ember. The 48 kHz PCM-24 WAV lasts approximately 2.68 seconds including tail, matches the independent VST3 within 1.2e-7 and has a silent final 100 ms. Source overwrite is rejected. Local evidence: `build-next/ember-native-export-verification.json`, `build-next/ember-legacy-verification.json`, `build-next/ember-listening` and `build-next/editor-previews`.

## Native desktop and FL Studio

The native 0.10 standalone app was inspected on screen. Its new tenth sidebar entry and flame icon scroll into view. Selecting Folded metal applies Drive 20 dB, Shape 100%, Color 30%, Filter 8 kHz, Anchor 20%, Bias 20%, Trim -10 dB and Mix 65%, and updates the displayed curve. The native Save dialog writes `build-next/Ember-native-stack.bapreset`.

The installed system VST3 matches the built binary. Fresh FL Studio opens the prior Cinder project with its old controls intact. Its native Load dialog restores the saved Ember rack. After saving `build-next/Ember-verification.flp` and exiting normally, a fresh FL Studio 24.2.2 process restores all eight Folded metal controls, the enabled state, selected Ember page and the preceding enabled modules. Both verification apps were closed normally afterward.

## Limits

These shapers are original designs, not circuit emulations or complete reference feature replicas. Antiderivative averaging and filters alter wet-path phase and frequency response. High drive and folding can still alias. The curve illustration excludes filtering, Anchor and Trim. Artistic listening approval, recorded DAW automation, full DAW-render comparison, other hosts/devices and signing remain unqualified.
