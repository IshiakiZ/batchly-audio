# Relay 0.7 verification

Checked on Windows x64, September 26, 2026. Relay is the seventh original effect.

## Automated checks

The standalone app and VST3 build with pinned JUCE and no new runtime dependencies. The eight audio/update suites and the new editor suite pass. Relay tests cover 8, 44.1, 48, 96, 192 and 384 kHz; exact disabled and dry paths; first-echo timing; alternating stereo repeats; mono isolation; silence; low-pass coloration; moving pitch; nominal tail decay; identical output across block sizes; extreme automation; invalid input; and clearing old delay history after disable/re-enable.

Independent VST3 checks at 44.1, 48 and 96 kHz verify echo timing, stereo bounce, dry/bypass, mono and silence. All seven effects and edited delay parameters survive state recall. The test compares the actual accepted normalized values because the independent host rounds text-inferred physical parameter assignments. This avoids misreporting its assignment rounding as a plugin recall fault.

Released states from 0.1 through 0.6 keep Relay off. The released 0.6 six-effect state renders exactly the same audio in this build. All five Relay presets render with decayed tails and no clipping. A local 37-second render took about 0.66 seconds; this is not a real-time guarantee. Updater validation and replacement tests pass.

## Native interface and export

On-screen inspection confirmed the seventh icon, scroll navigation, readable units and original echo display. Selecting Cross town correctly applied 240 ms Time, 55% Feedback, 9 kHz Tone, 8% Motion, 0.6 Hz Rate, full Bounce, 20% Glide and 40% Mix.

The new `editor_rendering` test constructs the actual plugin components without opening a desktop window. It checks all seven pages in standalone and VST3 layouts, control bounds, selected-tab visibility, immediate on/off status, preset callbacks and Relay's Time attachment. Its actual component snapshots were inspected. It found and fixed a first-frame sidebar status that previously waited for the timer refresh.

The test saves a native preset and calls the app's actual export implementation with an original two-second stereo probe. Bent signal produces a 48 kHz, 24-bit stereo WAV of 4.672979 seconds including the tail. Its output matches the independent VST3 within 1.2e-7; the final 100 ms peak is one 24-bit sample step. Export rejects overwriting the source. This checks the native export code; the file-dialog export workflow was already checked in earlier releases. Evidence is local at `build-next/relay-native-export-verification.json` and `build-next/editor-previews/Relay-export.wav`.

## FL Studio

The installed VST3 loaded in FL Studio 24.2.2. The previous Gleam project restored its existing effects with Relay off. Loading the native Bent signal preset displayed all eight expected Relay settings, the correct selected icon and enable states. FL Studio saved the separate `build-next/Relay-verification.flp`. A fresh FL process opened that file. After refreshing the desktop connection, on-screen inspection confirmed Bent signal, all eight controls, the selected Relay page and enable states. The saved evidence is `build-next/Relay-FL-recall.jpg`. Independent VST3 state round trips also pass.

## Limits

Time and modulation rate run freely without host tempo synchronization. Motion and time changes deliberately bend pitch. This is a fractional delay, not a BBD circuit model or pitch sequencer. Strong feedback can generate long tails; the allowance grows with delay interval and feedback. Artistic listening approval, exact reference equivalence, recorded DAW automation, broader devices/hosts and code signing remain unqualified.
