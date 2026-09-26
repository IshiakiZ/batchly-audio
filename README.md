# Batchly Audio

An original open-source audio collection for Windows and VST3 hosts. Version 0.11 adds **Vista**, a three-band stereo width effect, alongside **Drift** chorus/vibrato, **Patina** tape coloration, **Atrium** reverb, **Chime** harmonic resonance, **Helix** stereo phasing, **Gleam** presence/air, **Relay** moving delay **Forge** drum shaping **Cinder** tonal/noise texture and **Ember** bass saturation. Each effect has its own controls, illustration and sidebar icon.

Drift combines smooth randomized delay modulation, periodic modulation, stereo spread, an envelope-controlled low-pass filter and optional generated hiss. Its sound engine, layout, presets, graphics and demo audio were written for this project. It does not load or require a Cymatics plugin.

## Try it

Open `Batchly Audio.exe` in the Windows package, then click **Demo**. Choose **Soft focus** or **Pure vibrato**, adjust **Depth**, and toggle **Bypass** to compare. **Mix** near 50% blends the dry sound into a chorus; 100% wet produces vibrato.

The desktop app can open or accept a dropped mono/stereo WAV, AIFF, FLAC, MP3 or Ogg file. Click **Play**, adjust the effects, and use **Export WAV** to create a stereo 24-bit WAV at the source sample rate. Export includes the effect tail and refuses to save clipped output. The desktop app plays files and its demo; it does not monitor the microphone. Use your DAW for live recording through the VST3.

## The rack and Patina

Select an effect in the sidebar, then click its **ON/OFF** button to enable or disable it. Each sidebar button shows its effect's on/off state; the collection scrolls when needed. Selecting a page only changes the controls you see; it does not change the audio routing. Audio flows through **Drift, Patina, Atrium, Chime, Helix, Gleam, Relay, Forge, Cinder, Ember, then Vista**, followed by shared Output. **BYPASS** bypasses the whole rack. New instances start with Drift on, the other effects off.

Patina's **Sample rate** softens high frequencies through a filtered rate reducer. **Drive** adds original soft saturation; **Wear** and **Flutter** add slow and fast pitch variation. **Hiss** generates noise, **Chorus** adds a stereo voice, **Tone** rolls off the top end, and **Mix** blends the effect. Hiss defaults to zero. Rates above the host's rate use the host rate; the tape display shows the effective rate. Its filters have finite slopes and are not brick-wall filters.

Try **Fresh spool**, **Pocket cassette**, **Submerged**, **Sun-bleached**, and **Midnight dub**. The on-screen preset menu affects the displayed effect and enables it. DAW program selection recalls a single-effect starting point. Saved `.bapreset` files contain the entire rack. The wet tape path has an intentional variable delay of approximately 9-25 ms; partially wet settings can add comb coloration. The rack still reports zero latency because its dry path is immediate. With Atrium, Chime, Helix, Gleam, Relay, Forge, Cinder, Ember and Vista off, exports include a 160 ms tail when Patina is enabled.

Existing 0.1 Drift projects and presets load with Drift enabled and Patina disabled. Version 0.1 and 0.2 states both load with Atrium off. Original parameter IDs, order, plugin identity and the first fifty host program names remain unchanged.

## Atrium

Select **Atrium**, then turn **ATRIUM OFF** on. Try **Open atrium**, **Close walls**, **Velvet hall**, **Glass canopy**, or **After hours**. For reverb alone, turn Drift and Patina off. For a send/return track, set Atrium's **Mix** to 100%.

**Decay** controls the nominal low-frequency fade from 0.2 to 12 seconds. **Size** changes reflection spacing, from a compact room to an open hall. **Pre-delay** leaves up to 250 ms before the room responds. **Damping** makes the high frequencies fade faster; **Low cut** keeps bass out of the wet room. **Motion** gently modulates the reflection paths. **Width** narrows the wet signal to mono at zero, and **Mix** blends it with the unchanged dry path. Mono tracks remain mono.

The room illustration responds to Size and the measured wet level; it is not a measured acoustic response. Size and pre-delay changes can bend the tail's pitch. Atrium is one original room algorithm with five starting points, not a physical room model or a set of sampled impulse responses.

With Atrium enabled, exports reserve twice Decay, plus Pre-delay and 0.8 seconds, for the tail. The VST3 reports the same allowance to its host. A DAW can still apply its own render-tail setting. With Atrium, Chime, Helix, Gleam, Relay, Forge, Cinder, Ember and Vista off, exports retain the earlier 80 ms allowance, or 160 ms when Patina is on. Processing adds no delay to the dry path.

**Save** and **Load** store `.bapreset` files. A DAW also saves the plugin's controls in its project. The starting-point menu selects factory settings; controls can then be edited freely.

## Chime

Select **Chime**, then choose **Glass strings**, **Minor bells**, **Copper choir**, **Small music box**, or **Suspended air**. Choose a root note, scale and base octave below the strings. Incoming sound excites a bank of tuned resonators; Chime does not change the pitch of the dry signal or generate notes from silence. It works especially well with percussion, plucks and textured sounds.

**Ring** sets the nominal decay from 0.08 to 6 seconds. **Color** softens the ringing layer, while **Drive** adds harmonics before it. **Spread** introduces a second octave. **Detune** separates the stereo tunings slightly; **Motion** adds slow pitch movement. **Width** centers the wet layer at zero and **Mix** blends it with the input. The string display responds to measured resonator levels. Major, minor, Dorian, major/minor pentatonic and whole-tone scales are available, with base octaves 2 through 4.

Key changes smoothly retune ringing notes, so moving them during a tail can create a pitch glide. The engine is a modal resonator, not a pitch-correction or MIDI instrument. It does not include an arpeggiator or Gamma's full control set. The five presets, tuning design, graphics and processing code are original.

When Chime is enabled, export and host tail allowances add twice Ring plus 0.25 seconds to the upstream rack's allowance, because a reverb tail can continue exciting the strings. All 0.1, 0.2 and 0.3 projects load with Chime off. The earlier parameter IDs, order, program numbers and plugin identity remain unchanged.

## Helix

Select **Helix**, then try **Slow orbit**, **Silver sweep**, **Deep current**, **Retro spin**, or **Hollow metal**. **Rate** controls the freely running sweep speed, **Depth** its range, and **Center** the frequency around which it moves. **Feedback** adds resonance with positive or negative polarity. **Tone** softens the wet layer, **Drive** adds input saturation, and **Width** offsets the two stereo sweeps. Mono input stays mono.

**Mix** near 50% produces moving cancellations between dry and phase-shifted audio. At 100% wet, those dry/wet cancellations disappear; you hear phase rotation, tone and feedback instead. The eight-stage engine and its orbital illustration are original designs. The display shows sweep positions, not a measured spectrum. Rate is in Hz without host tempo synchronization.

Helix adds a 1.5-second tail allowance to exports and the VST3 host report when enabled. States from 0.1 through 0.4 load with Helix off. Existing parameter IDs, order, plugin identity and the first twenty host programs are retained.

## Gleam

Select **Gleam**, then try **Clear vocal**, **Silver top**, **Drum shine**, **Soft lift**, or **Open mix**. **Presence** adds a broad upper-mid lift, while **Air** raises the top end above **Focus**. The bands overlap gently; they are not surgical EQ bands. Their dB amounts set the individual lift gains and do not simply add together.

**Excite** adds soft harmonic color to the high-frequency layer. **Tame** turns down the added brightness during strong high-frequency peaks; it is not a full-signal de-esser. **Width** narrows or widens only the added layer while preserving the dry stereo image. It cannot create stereo width from a mono input. **Trim** compensates for added level inside Gleam, and **Mix** blends with the original. Turn Presence, Air and Excite to zero and Trim to 0 dB for a neutral signal.

The prism illustration responds to measured high-frequency input level, and Tame shows the reduction of the added layer. Focus is limited by the host sample rate. The enhancer uses broad first-order filters and original nonlinear coloration; it is not an exact recreation of Halo's macros. Gleam adds 80 ms to the tail allowance when enabled and stays off when loading older projects.

## Relay

Select **Relay** and try **Soft answer**, **Cross town**, **Short circuit**, **Long return**, or **Bent signal**. **Time** sets echo spacing from 10 to 2,000 ms. **Feedback** sets how much of each echo returns, while **Tone** softens each pass. A gentle bass filter and bounded soft coloration keep the repeat path controlled.

**Motion** moves the delay read position to bend the echoes' pitch, and **Rate** sets its speed. **Bounce** sends repeats between speakers. At full Bounce, the input is centered into the first left echo and subsequent repeats alternate sides. Zero preserves separate stereo paths; mono tracks stay mono. **Glide** controls how slowly Time follows a new setting. **Mix** blends dry input and echoes; use 100% for a send.

Time and Rate are free-running, without host tempo synchronization. Changing Time or using Motion deliberately changes pitch. This is an original fractional-delay design, not a circuit model, pitch sequencer or recreation of Illusion's complete feature set. Strong feedback creates long trails; the host and export tail allowance grows with Time, Motion and Feedback to cover nominal decay below -80 dB. Existing projects load with Relay off. The repeated-block illustration responds to measured left/right wet levels; its spacing is decorative rather than a timing ruler.

## Forge

Select **Forge** and try **First strike**, **Heavy floor**, **Snare press**, **Soft mallet**, or **Parallel iron**. **Punch** emphasizes new hits; negative values soften their attack. **Body** blends in linked compression with makeup gain for sustain. Both use a shared stereo detector, so one side does not receive a different envelope gain.

**Weight** adds a broad low shelf around 120 Hz, and **Edge** lifts the region above 3.2 kHz with a gentle slope. **Drive** adds soft saturation, **Width** changes the stereo image, and **Ceiling** smoothly rounds fully wet sample peaks. **Mix** blends with the original. The clipper follows width, so widening cannot exceed the wet sample ceiling. Dry blends, downstream Output and inter-sample peaks can exceed it; this is not a true-peak limiter.

The envelope drawing illustrates the Punch and Body settings. The Attack and Clip bars show measured detector and clipping activity. The design uses no lookahead, oversampling or extra processing delay; strong saturation can alias. It provides the core drum-shaping role rather than Diablo's full mode selection, resonator and multi-band EQ. Old states load with Forge off, with all earlier controls and program numbers retained. Forge adds 80 ms to the export/host tail allowance for its tone filters.

## Cinder

Select **Cinder** and try **Fine grain**, **Copper dust**, **Paper speaker**, **Ash cloud**, or **Rough edge**. **Grit** adds a driven tonal layer, and **Noise** adds generated texture. **Tone focus** and **Noise focus** choose each layer's broad frequency region independently. **Drive** increases tonal coloration; zero makes the tonal addition silent.

The noise follows incoming sound. **Decay** controls its release after a hit; it generates no idle hiss before sound enters. **Width** centers the additions at zero and spreads their texture at higher settings, while preserving the dry stereo image. **Mix** scales both additions together, so zero is exactly dry. Mono tracks stay mono. Lower Output when the extra layers increase level.

The grain display shows selected band regions and measured layer activity. It is an illustration, not a measured spectrum. The noise is synthesized in the engine with a repeatable seed, using no sampled libraries. Broad filters adapt to the host sample rate. The nonlinear tone layer is not oversampled and can alias. Tail allowance adds ten Decay time constants plus 0.3 seconds for nominal decay below -80 dB. Older states keep Cinder off and retain their prior audio.

## Ember

Select **Ember** and try **Warm foundation**, **Wire bass**, **Dense floor**, **Folded metal**, or **Quiet ember**. **Drive** pushes the sound into four original saturation curves. **Shape** moves continuously through Round, Edge, Dense and Fold; the display labels the nearest curve while drawing the actual blend. Fold bends loud signals back on themselves for metallic harmonics.

**Color** darkens or brightens the signal before saturation. **Filter** softens the output afterward. **Anchor** restores some of the original bass below roughly 150 Hz before level trimming; it is not a subharmonic generator. **Bias** makes the curves asymmetric for even harmonics, with DC removal afterward. **Trim** compensates for level, and **Mix** blends with the original signal.

The engine averages each curve between adjacent input samples to reduce nonlinear aliasing. This changes the wet frequency response and introduces a small frequency-dependent phase shift; it does not eliminate aliasing or add an integer sample of reported host latency. The dry branch remains immediate. The displayed curve illustrates the static shaper before filtering, Anchor and Trim; the Input and Wet bars show measured peaks. This is an original bass coloration design, not a hardware circuit model. Ember adds 0.6 seconds to the tail allowance and remains off in older projects.

## Vista

Select **Vista** and try **Open window**, **Centered bass**, **Mono bloom**, **Narrow room**, or **Air frame**. **Low Width**, **Mid Width** and **High Width** change the stereo difference in three broad frequency regions. 100% keeps a region's original width, zero centers it, and 200% doubles its side level. **Low Split** and **High Split** move the gentle transitions. The bands overlap; they are not brick-wall crossovers.

**Spread** creates an additional stereo difference from the center signal above Low Split. **Delay** sets this layer's time offset from 1 to 30 ms. Moving Delay can bend its pitch. The added signal goes to the two channels with opposite polarity, preserving the original left-plus-right sum. **Mix** blends the changed image with the original. True mono host tracks stay unchanged; a stereo track carrying the same sound in both channels can be widened by Spread.

With all widths at 100% and Spread at zero, Vista is exactly neutral. Wider settings can increase individual channel peaks, so watch Output. Mono compatibility refers to Vista's own output; later nonlinear effects or separate channel processing can change it. The display illustrates the width settings and measures Mid/Side peaks, rather than showing a vectorscope. Vista adds 0.5 seconds to the tail allowance and stays off in all older projects.

## FL Studio and other VST3 hosts

For FL Studio on 64-bit Windows, copy the **entire** `Batchly Audio.vst3` folder from the package into `C:\Program Files\Common Files\VST3`. Accept Windows' administrator prompt if requested. In FL Studio, use **Options > Manage plugins**, enable **Verify plugins**, then choose **Find installed plugins**. Insert **Batchly Audio** on a mixer track. The plugin receives its audio from that track, so file-player controls only appear in the desktop app.

The package also includes `install-vst3.ps1`, which performs that copy, asks Windows for administrator approval and verifies the installed files. Right-click it and choose **Run with PowerShell**. Close any host using the previous version before updating.

FL Studio requires a standard VST3 installation folder; adding a custom search path does not make it scan VST3 bundles there. See [Image-Line's installation instructions](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm). Other hosts may support the current-user folder `%LOCALAPPDATA%\Programs\Common\VST3`.

The VST3 has passed independent host tests and FL Studio's verified plugin scan as a 64-bit effect. See the verification record for the exact checks and limits.

This preview supports mono-in/mono-out and stereo-in/stereo-out. It reports zero host latency because each dry branch is immediate. Wet delays and reverb reflections are intentional parts of the effects. It does not claim sample-for-sample equivalence to any commercial product.

## Updates

Click the small **Updates** button in the collection sidebar. It checks this project's public GitHub releases only when clicked. If a newer version is available, choose **Install** to download and verify the Windows package. Save your work and close the desktop app or DAW normally; the updater then replaces the files and restarts the desktop app, or tells you to reopen your DAW. It never closes a DAW for you. Windows may request administrator approval for the system VST3 folder.

Preview builds can receive newer previews or stable releases. Stable builds only offer stable releases. Download size, source URL and GitHub's SHA-256 digest are verified, and changed staged files are rejected. An installation failure restores files already replaced. A second copy of a DAW holding the plugin open can prevent the update; close it and retry. Binaries are currently unsigned.

Only an update check or an approved download contacts GitHub. Audio and presets stay local. No Batchly website integration is required. Install logs are in `%LOCALAPPDATA%\BatchlyAudio`; verified packages remain in its `updates` folder.

## Build and verify

Requirements: CMake 3.22 or later, Git, and Visual Studio 2022 with Desktop development with C++ and a Windows SDK. The first configure fetches the pinned JUCE 9.0.2 source unless it is already present at `vendor/JUCE`.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 6
ctest --test-dir build -C Release --output-on-failure
./tools/verify_updater.ps1
```

The `editor_rendering` check also renders every actual JUCE page in desktop and VST3 layouts, checks attachments and selected-sidebar visibility, and exports an original probe through the native WAV renderer. Its images and audio are in the build folder under `editor-previews`. These component checks do not replace interaction tests in a running DAW.

The resulting app is in `build/BatchlyAudio_artefacts/Release/Standalone/`; the plugin bundle is in the adjacent `VST3/` directory.

For DSP-only tests with no audio-framework download:

```powershell
cmake -S . -B build-dsp -DBATCHLY_BUILD_APP=OFF
cmake --build build-dsp --config Release
ctest --test-dir build-dsp -C Release --output-on-failure
```

`tools/verify_plugin.py` additionally loads the compiled plugin in Pedalboard, verifies audio processing and state recall, and generates original listening examples. This optional check uses Python 3.12 with `numpy` for numerical comparisons, `pedalboard` as an independent VST3 host and `soundfile` to save the examples. These are test tools, not app dependencies.

```powershell
python -m pip install -r tools/requirements-verify.txt
python tools/verify_plugin.py --plugin "build/BatchlyAudio_artefacts/Release/VST3/Batchly Audio.vst3/Contents/x86_64-win/Batchly Audio.vst3" --output build/listening
```

## Design and scope

- A consistent collection frame with a distinct working surface for each effect.
- Large primary controls, readable units, keyboard-accessible controls and explanatory tooltips.
- Original materials, names, presets and graphics suited to the sound.
- One effect completed and checked at a time. No empty future-effect buttons.
- Local processing. No account, activation, telemetry, audio upload or Batchly service dependency. Update checks are requested explicitly.

See [the inventory and build order](docs/ROADMAP.md), [implementation provenance](docs/PROVENANCE.md), and [verification record](docs/VERIFICATION.md).

Batchly hosting and download distribution are deferred to a later owner decision. A browser version is not part of this preview.

## License

The original project source is licensed under **GNU AGPL version 3 or later**. The combined build with JUCE is distributed under AGPLv3. See `LICENSE`, `THIRD_PARTY_NOTICES.md` and the retained licenses in `third-party/`. This is an independent project and is not affiliated with or endorsed by Cymatics.
