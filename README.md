# Batchly Audio

An original open-source audio collection for Windows and VST3 hosts. Version 0.3 adds **Atrium**, a spacious stereo reverb, alongside **Drift**, a chorus/vibrato effect, and **Patina**, a tape/lo-fi processor. Each effect has its own controls, illustration and sidebar icon.

Drift combines smooth randomized delay modulation, periodic modulation, stereo spread, an envelope-controlled low-pass filter and optional generated hiss. Its sound engine, layout, presets, graphics and demo audio were written for this project. It does not load or require a Cymatics plugin.

## Try it

Open `Batchly Audio.exe` in the Windows package, then click **Demo**. Choose **Soft focus** or **Pure vibrato**, adjust **Depth**, and toggle **Bypass** to compare. **Mix** near 50% blends the dry sound into a chorus; 100% wet produces vibrato.

The desktop app can open or accept a dropped mono/stereo WAV, AIFF, FLAC, MP3 or Ogg file. Click **Play**, adjust the effects, and use **Export WAV** to create a stereo 24-bit WAV at the source sample rate. Export includes the effect tail and refuses to save clipped output. The desktop app plays files and its demo; it does not monitor the microphone. Use your DAW for live recording through the VST3.

## Patina and the rack

Select an effect in the sidebar, then click its **ON/OFF** button to enable or disable it. The sidebar always shows all three effects' states. Selecting a page only changes the controls you see; it does not change the audio routing. Audio flows through **Drift, then Patina, then Atrium**, followed by shared Output. **BYPASS** bypasses the whole rack. New instances start with Drift on, Patina and Atrium off.

Patina's **Sample rate** softens high frequencies through a filtered rate reducer. **Drive** adds original soft saturation; **Wear** and **Flutter** add slow and fast pitch variation. **Hiss** generates noise, **Chorus** adds a stereo voice, **Tone** rolls off the top end, and **Mix** blends the effect. Hiss defaults to zero. Rates above the host's rate use the host rate; the tape display shows the effective rate. Its filters have finite slopes and are not brick-wall filters.

Try **Fresh spool**, **Pocket cassette**, **Submerged**, **Sun-bleached**, and **Midnight dub**. The on-screen preset menu affects the displayed effect and enables it. DAW program selection recalls a single-effect starting point. Saved `.bapreset` files contain the entire rack. The wet tape path has an intentional variable delay of approximately 9-25 ms; partially wet settings can add comb coloration. The rack still reports zero latency because its dry path is immediate. Exports include a 160 ms tail when Patina is enabled.

Existing 0.1 Drift projects and presets load with Drift enabled and Patina disabled. Version 0.1 and 0.2 states both load with Atrium off. Original parameter IDs, order, plugin identity and the first ten host program names remain unchanged.

## Atrium

Select **Atrium**, then turn **ATRIUM OFF** on. Try **Open atrium**, **Close walls**, **Velvet hall**, **Glass canopy**, or **After hours**. For reverb alone, turn Drift and Patina off. For a send/return track, set Atrium's **Mix** to 100%.

**Decay** controls the nominal low-frequency fade from 0.2 to 12 seconds. **Size** changes reflection spacing, from a compact room to an open hall. **Pre-delay** leaves up to 250 ms before the room responds. **Damping** makes the high frequencies fade faster; **Low cut** keeps bass out of the wet room. **Motion** gently modulates the reflection paths. **Width** narrows the wet signal to mono at zero, and **Mix** blends it with the unchanged dry path. Mono tracks remain mono.

The room illustration responds to Size and the measured wet level; it is not a measured acoustic response. Size and pre-delay changes can bend the tail's pitch. Atrium is one original room algorithm with five starting points, not a physical room model or a set of sampled impulse responses.

With Atrium enabled, exports reserve twice Decay, plus Pre-delay and 0.8 seconds, for the tail. The VST3 reports the same allowance to its host. A DAW can still apply its own render-tail setting. With Atrium off, exports retain the earlier 80 ms allowance, or 160 ms when Patina is on. Processing adds no delay to the dry path.

**Save** and **Load** store `.bapreset` files. A DAW also saves the plugin's controls in its project. The starting-point menu selects factory settings; controls can then be edited freely.

## FL Studio and other VST3 hosts

For FL Studio on 64-bit Windows, copy the **entire** `Batchly Audio.vst3` folder from the package into `C:\Program Files\Common Files\VST3`. Accept Windows' administrator prompt if requested. In FL Studio, use **Options > Manage plugins**, enable **Verify plugins**, then choose **Find installed plugins**. Insert **Batchly Audio** on a mixer track. The plugin receives its audio from that track, so file-player controls only appear in the desktop app.

The package also includes `install-vst3.ps1`, which performs that copy, asks Windows for administrator approval and verifies the installed files. Right-click it and choose **Run with PowerShell**. Close any host using the previous version before updating.

FL Studio requires a standard VST3 installation folder; adding a custom search path does not make it scan VST3 bundles there. See [Image-Line's installation instructions](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/basics_externalplugins.htm). Other hosts may support the current-user folder `%LOCALAPPDATA%\Programs\Common\VST3`.

The VST3 has passed independent host tests and FL Studio's verified plugin scan as a 64-bit effect. See the verification record for the exact checks and limits.

This preview supports mono-in/mono-out and stereo-in/stereo-out. It reports zero host latency: the dry branch is immediate, while the wet branch has an intentional modulated 8-20 ms delay. It does not claim sample-for-sample equivalence to any commercial product.

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
