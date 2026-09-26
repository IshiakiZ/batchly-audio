# Publishing an update

1. Set `VERSION` to a higher `major.minor.patch` or `major.minor.patch-preview.number`. Keep existing plugin and parameter IDs stable so projects reopen correctly.
2. Build on Windows and run CTest, `tools/verify_plugin.py` and `tools/verify_updater.ps1`. Verify the app and a saved DAW project on screen. Update the verification record with actual results and limits.
3. Commit the source. Keep `vendor/JUCE` clean at the commit pinned in CMake. Run `python tools/package.py`; this creates the Windows package, complete corresponding source including JUCE, and SHA-256 checksums under `dist`.
4. Wait for GitHub's build checks to pass. Create a draft release tagged `v` followed by the exact `VERSION`. Attach both ZIPs and `SHA256SUMS.txt`, then publish. Mark preview versions as prereleases. Do not replace an existing release's files with a different build; publish a higher version.
5. Check `build/Release/update_tests.exe --check-live`. For an explicit download test, `--stage-live` simulates an older installed version and verifies and extracts the public Windows package. It does not launch the installer or replace any installed app.

The updater reads `https://api.github.com/repos/IshiakiZ/batchly-audio/releases?per_page=20`. Its asset must be named `Batchly-Audio-VERSION-Windows-x64.zip`, with GitHub's `sha256:` digest present. It does not use the source ZIP or require Batchly hosting.

The publisher controls executable updates, so protect repository and release permissions. The checksum checks package integrity against GitHub's response; it is not an independent code-signing identity.
