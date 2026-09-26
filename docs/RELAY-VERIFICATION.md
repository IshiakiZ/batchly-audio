# Relay 0.7 verification

Checked on Windows x64, September 26, 2026. Relay is the seventh original effect, currently awaiting native interface and FL Studio checks.

## Automated checks

The standalone app and VST3 build with pinned JUCE and no new runtime dependencies. All eight CTest suites pass. Relay tests cover 8, 44.1, 48, 96, 192 and 384 kHz; exact disabled and dry paths; first-echo timing; alternating stereo repeats; mono isolation; silence; low-pass coloration; moving pitch; nominal tail decay; identical output across block sizes; extreme automation; invalid input; and clearing old delay history after disable/re-enable.

Independent VST3 checks at 44.1, 48 and 96 kHz verify echo timing, stereo bounce, dry/bypass, mono and silence. All seven effects and edited delay parameters survive state recall. The test compares the actual accepted normalized values because the independent host rounds text-inferred physical parameter assignments. This avoids misreporting its assignment rounding as a plugin recall fault.

Released states from 0.1 through 0.6 keep Relay off. The released 0.6 six-effect state renders exactly the same audio in this build. All five Relay presets render with decayed tails and no clipping. A local 37-second render took about 0.66 seconds; this is not a real-time guarantee. Updater validation and replacement tests pass.

## Pending native checks

Desktop verification was stopped with the user's Escape key. Relay's new page, icon, preset interactions, native export and FL Studio project recall have not yet been verified on screen. The previous six-effect release has its own completed native checks. Relay is not qualified for release until these new checks are completed.

## Limits

Time and modulation rate run freely without host tempo synchronization. Motion and time changes deliberately bend pitch. This is a fractional delay, not a BBD circuit model or pitch sequencer. Strong feedback can generate long tails; the allowance grows with delay interval and feedback. Artistic listening approval, exact reference equivalence, recorded DAW automation, broader devices/hosts and code signing remain unqualified.
