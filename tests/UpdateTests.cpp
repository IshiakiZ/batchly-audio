// SPDX-License-Identifier: AGPL-3.0-or-later
#include "UpdateService.h"
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
juce::String feed(const juce::String& version, const juce::String& digest, const juce::String& root = "https://github.com/IshiakiZ/batchly-audio/releases/download/") {
    const auto file = "Batchly-Audio-" + version + "-Windows-x64.zip";
    return "[{\"tag_name\":\"v" + version + "\",\"draft\":false,\"assets\":[{\"name\":\"" + file
        + "\",\"state\":\"uploaded\",\"size\":123,\"digest\":\"sha256:" + digest
        + "\",\"browser_download_url\":\"" + root + "v" + version + "/" + file + "\"}]}]";
}
int main(int argc, char** argv) {
    try {
        batchly::UpdateRelease release;
        const auto hash = juce::String::repeatedString("a", 64);
        require(batchly::UpdateService::parseReleaseFeed(feed("0.1.0-preview.2", hash), release).wasOk() && release.available(), "New preview was not detected");
        require(batchly::UpdateService::parseReleaseFeed(feed("0.1.0-preview.1", hash), release).wasOk() && !release.available(), "Current version offered as update");
        require(batchly::UpdateService::parseReleaseFeed(feed("0.0.9", hash), release).wasOk() && !release.available(), "Downgrade was offered");
        require(batchly::UpdateService::parseReleaseFeed(feed("0.1.0", hash), release).wasOk() && release.available(), "Stable release did not supersede preview");
        require(batchly::UpdateService::parseReleaseFeed(feed("0.2.0-preview.1", hash), release, "0.1.0").wasOk() && !release.available(), "Stable opted into preview");
        require(batchly::UpdateService::parseReleaseFeed(feed("0.2.0", hash, "https://example.com/"), release).failed(), "Foreign download URL accepted");
        require(batchly::UpdateService::parseReleaseFeed(feed("0.2.0", "bad"), release).failed(), "Missing digest accepted");
        require(batchly::UpdateService::parseReleaseFeed("{bad json}", release).failed(), "Malformed response accepted");
        const auto file = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("batchly-update-test", ".zip");
        require(file.replaceWithText("original"), "Could not create test file");
        const auto expected = juce::SHA256(file).toHexString();
        require(batchly::UpdateService::verifyArchive(file, expected).wasOk(), "Valid digest rejected");
        require(file.replaceWithText("changed"), "Could not modify test file");
        require(batchly::UpdateService::verifyArchive(file, expected).failed(), "Changed download accepted");
        file.deleteFile();
        if (argc > 1 && (juce::String(argv[1]) == "--check-live" || juce::String(argv[1]) == "--stage-live")) {
            const bool stage = juce::String(argv[1]) == "--stage-live";
            // Use an older preview so the live download test includes preview releases.
            const auto result = batchly::UpdateService::check(release, stage ? "0.0.0-preview.1" : batchly::UpdateService::currentVersion);
            require(result.wasOk(), result.getErrorMessage().toRawUTF8());
            std::cout << (release.available() ? "Update available: " + release.version.toStdString() : "Live check: up to date") << '\n';
            if (stage) {
                require(release.available(), "Expected a public release to stage");
                juce::File request;
                const auto prepared = batchly::UpdateService::stage(release, request, false);
                require(prepared.wasOk(), prepared.getErrorMessage().toRawUTF8());
                std::cout << "Live download verified and staged (not installed): " << request.getFullPathName() << '\n';
            }
        }
        std::cout << "PASS: version ordering, channels, URL validation, digest checks, malformed responses\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
