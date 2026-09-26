// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_core/juce_core.h>

namespace batchly {
struct UpdateRelease {
    juce::String version, url, sha256;
    juce::int64 size = 0;
    bool available() const { return version.isNotEmpty(); }
};

class UpdateService {
public:
    static constexpr const char* currentVersion = BATCHLY_RELEASE_VERSION;
    static juce::Result parseReleaseFeed(const juce::String&, UpdateRelease&, const juce::String& current = currentVersion);
    static juce::Result check(UpdateRelease&, const juce::String& current = currentVersion);
    static juce::Result stage(const UpdateRelease&, juce::File& request, bool standalone);
    static juce::Result launch(const juce::File& request);
    static juce::Result verifyArchive(const juce::File&, const juce::String& expectedHash);
};
}
