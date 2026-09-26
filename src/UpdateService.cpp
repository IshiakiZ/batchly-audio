// SPDX-License-Identifier: AGPL-3.0-or-later
#include "UpdateService.h"
#include "BinaryData.h"
#include <juce_cryptography/juce_cryptography.h>
#include <array>
#include <regex>
#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace batchly {
namespace {
constexpr juce::int64 maximumDownload = 100 * 1024 * 1024;
const juce::String downloadRoot = "https://github.com/IshiakiZ/batchly-audio/releases/download/";
bool versionParts(const juce::String& text, std::array<int, 4>& parts) {
    std::smatch match;
    const auto value = text.toStdString();
    static const std::regex pattern(R"(^v?([0-9]{1,5})\.([0-9]{1,5})\.([0-9]{1,5})(?:-preview\.([0-9]{1,5}))?$)");
    if (!std::regex_match(value, match, pattern)) return false;
    for (int i = 0; i < 3; ++i) parts[i] = std::stoi(match[i + 1].str());
    parts[3] = match[4].matched ? std::stoi(match[4].str()) : 100000;
    return true;
}
std::unique_ptr<juce::InputStream> openUrl(const juce::String& url, int& status) {
    return juce::URL(url).createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(10000).withNumRedirectsToFollow(5).withStatusCode(&status)
        .withExtraHeaders("User-Agent: Batchly-Audio\r\nAccept: application/vnd.github+json\r\n"));
}
juce::File updateRoot() {
    return juce::File(juce::SystemStats::getEnvironmentVariable("LOCALAPPDATA", ""))
        .getChildFile("BatchlyAudio/updates");
}
}

juce::Result UpdateService::parseReleaseFeed(const juce::String& json, UpdateRelease& release, const juce::String& current) {
    release = {};
    std::array<int, 4> installed {};
    if (!versionParts(current, installed)) return juce::Result::fail("This build has an invalid version number.");
    const auto data = juce::JSON::parse(json);
    const auto* releases = data.getArray();
    if (releases == nullptr) return juce::Result::fail("The update service returned an unreadable response.");
    auto newest = installed;
    for (const auto& item : *releases) {
        if (static_cast<bool>(item["draft"])) continue;
        const auto tag = item["tag_name"].toString();
        std::array<int, 4> candidate {};
        if (!versionParts(tag, candidate) || candidate <= newest) continue;
        // Stable builds do not opt into previews; preview builds can advance to either.
        if (installed[3] == 100000 && candidate[3] != 100000) continue;
        const auto version = tag.startsWithChar('v') ? tag.substring(1) : tag;
        const auto filename = "Batchly-Audio-" + version + "-Windows-x64.zip";
        const auto assetList = item["assets"];
        const auto* assets = assetList.getArray();
        if (assets == nullptr) continue;
        for (const auto& asset : *assets) {
            if (asset["name"].toString() != filename || asset["state"].toString() != "uploaded") continue;
            const auto url = asset["browser_download_url"].toString();
            const auto digest = asset["digest"].toString();
            const auto size = static_cast<juce::int64>(asset["size"]);
            if (url != downloadRoot + tag + "/" + filename || !digest.startsWith("sha256:")
                || digest.length() != 71 || !digest.substring(7).containsOnly("0123456789abcdefABCDEF")
                || size <= 0 || size > maximumDownload)
                return juce::Result::fail("The new release has incomplete verification data. Try again after its upload finishes.");
            release = { version, url, digest.substring(7).toLowerCase(), size };
            newest = candidate;
        }
    }
    return juce::Result::ok();
}

juce::Result UpdateService::check(UpdateRelease& release, const juce::String& current) {
    int status = 0;
    auto stream = openUrl("https://api.github.com/repos/IshiakiZ/batchly-audio/releases?per_page=20", status);
    if (status == 404) return juce::Result::fail("No public releases are available yet.");
    if (status == 403 || status == 429) return juce::Result::fail("GitHub's update check limit was reached. Please try again later.");
    if (!stream || status != 200) return juce::Result::fail("Could not reach GitHub. Check your connection and try again.");
    juce::MemoryBlock response;
    stream->readIntoMemoryBlock(response, 1024 * 1024);
    if (response.getSize() >= 1024 * 1024) return juce::Result::fail("The update response was unexpectedly large.");
    return parseReleaseFeed(response.toString(), release, current);
}

juce::Result UpdateService::verifyArchive(const juce::File& archive, const juce::String& expectedHash) {
    if (!archive.existsAsFile() || expectedHash.length() != 64
        || !expectedHash.containsOnly("0123456789abcdefABCDEF")
        || juce::SHA256(archive).toHexString() != expectedHash.toLowerCase())
        return juce::Result::fail("The update download failed its verification. Your installed version was not changed.");
    return juce::Result::ok();
}

juce::Result UpdateService::stage(const UpdateRelease& release, juce::File& requestFile, bool standalone) {
    if (!release.available() || !release.url.startsWith(downloadRoot) || release.size <= 0 || release.size > maximumDownload)
        return juce::Result::fail("The update information is invalid.");
    const auto directory = updateRoot().getChildFile(juce::Uuid().toString());
    if (auto result = directory.createDirectory(); result.failed()) return result;
    const auto archive = directory.getChildFile("download.zip");
    int status = 0;
    auto input = openUrl(release.url, status);
    if (!input || status != 200) return juce::Result::fail("The update could not be downloaded. Please try again.");
    auto output = archive.createOutputStream();
    if (!output) return juce::Result::fail("The update folder could not be written.");
    std::array<char, 65536> buffer {};
    juce::int64 received = 0;
    const auto started = juce::Time::getMillisecondCounterHiRes();
    while (!input->isExhausted()) {
        const int count = input->read(buffer.data(), static_cast<int>(buffer.size()));
        if (count <= 0) break;
        received += count;
        if (received > release.size || juce::Time::getMillisecondCounterHiRes() - started > 180000)
            return juce::Result::fail("The update download exceeded its size or time limit. Please try again.");
        if (!output->write(buffer.data(), static_cast<size_t>(count))) return juce::Result::fail("Could not save the update. Check free disk space.");
    }
    output.reset();
    if (received != release.size) return juce::Result::fail("The update download was interrupted. Please try again.");
    if (auto result = verifyArchive(archive, release.sha256); result.failed()) return result;
    juce::ZipFile zip(archive);
    if (zip.getNumEntries() <= 0 || zip.getNumEntries() > 300) return juce::Result::fail("This update package is invalid.");
    juce::int64 expanded = 0;
    for (int i = 0; i < zip.getNumEntries(); ++i) {
        const auto* entry = zip.getEntry(i);
        const auto name = entry->filename.replaceCharacter('\\', '/');
        expanded += entry->uncompressedSize;
        if (name.startsWithChar('/') || name.contains(":") || name.contains("../") || name == ".."
            || entry->isSymbolicLink || entry->uncompressedSize < 0 || expanded > 300 * 1024 * 1024)
            return juce::Result::fail("The update contains an invalid archive entry.");
    }
    const auto package = directory.getChildFile("package");
    if (auto result = zip.uncompressTo(package); result.failed()) return result;
    const juce::StringArray required { "Batchly Audio.exe", "Batchly Audio.vst3/Contents/x86_64-win/Batchly Audio.vst3",
                                       "Batchly Audio.vst3/Contents/Resources/moduleinfo.json" };
    auto* files = new juce::DynamicObject();
    for (const auto& relative : required) {
        const auto file = package.getChildFile(relative);
        if (!file.existsAsFile() || file.getSize() == 0) { delete files; return juce::Result::fail("The update package is missing a required file."); }
        files->setProperty(relative, juce::SHA256(file).toHexString());
    }
    auto* request = new juce::DynamicObject();
    juce::var requestData(request);
    request->setProperty("version", release.version);
    request->setProperty("files", juce::var(files));
    const auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    request->setProperty("hostExecutable", executable.getFullPathName());
#if JUCE_WINDOWS
    request->setProperty("hostProcessId", static_cast<int>(GetCurrentProcessId()));
#endif
    request->setProperty("standaloneTarget", standalone ? executable.getFullPathName() : juce::String());
    request->setProperty("installPlugin", !standalone || juce::File(juce::SystemStats::getEnvironmentVariable("CommonProgramFiles", "C:\\Program Files\\Common Files"))
        .getChildFile("VST3/Batchly Audio.vst3").exists());
    request->setProperty("showResult", true);
    request->setProperty("restartApp", standalone);
    requestFile = directory.getChildFile("request.json");
    const auto helper = directory.getChildFile("apply-update.ps1");
    if (!helper.replaceWithData(BinaryData::applyupdate_ps1, BinaryData::applyupdate_ps1Size)
        || !requestFile.replaceWithText(juce::JSON::toString(requestData)))
        return juce::Result::fail("The update installer could not be prepared.");
    return juce::Result::ok();
}

juce::Result UpdateService::launch(const juce::File& request) {
    const auto powershell = juce::File(juce::SystemStats::getEnvironmentVariable("SystemRoot", "C:\\Windows"))
        .getChildFile("System32/WindowsPowerShell/v1.0/powershell.exe");
    juce::ChildProcess helper;
    if (!helper.start(juce::StringArray { powershell.getFullPathName(), "-NoProfile", "-File",
                      request.getSiblingFile("apply-update.ps1").getFullPathName(), "-RequestFile", request.getFullPathName() }, 0))
        return juce::Result::fail("Windows could not start the update installer.");
    const auto ready = request.getSiblingFile("ready.signal");
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (ready.existsAsFile()) return juce::Result::ok();
        if (!helper.isRunning()) break;
        juce::Thread::sleep(50);
    }
    if (helper.isRunning()) helper.kill();
    return juce::Result::fail("Windows did not start the update installer. Another update may already be waiting, or a system policy may block it. Your installed version was not changed.");
}
}
