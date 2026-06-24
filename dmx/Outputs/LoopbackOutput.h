#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>

#include "IDmxOutput.h"

/** TEST-ONLY output. Stores the most recent frame for each universe in memory so
    a unit test can assert on exact byte values — no network, no hardware, fully
    deterministic.

    This file is self-contained and nothing in the core references it. To strip
    the test harness for release: delete this header, the tests/ folder, and the
    dmx_tests target in CMakeLists.txt. The controller and all real outputs are
    unaffected.
*/
class LoopbackOutput : public IDmxOutput
{
public:
    bool open() override  { return true; }
    void close() override {}

    void sendUniverse (int universe, const std::uint8_t* data, int numChannels) override
    {
        const std::lock_guard<std::mutex> sl (lock);
        auto& buf = frames[universe];
        buf.fill (0);
        std::memcpy (buf.data(), data, (size_t) std::clamp (numChannels, 0, 512));
    }

    const char* name() const override { return "Loopback (test)"; }

    /** Read back a channel (1..512) captured from the last frame for a universe. */
    std::uint8_t getChannel (int universe, int channel)
    {
        const std::lock_guard<std::mutex> sl (lock);
        auto it = frames.find (universe);
        if (it == frames.end() || channel < 1 || channel > 512)
            return 0;
        return it->second[(size_t) (channel - 1)];
    }

private:
    std::mutex                                     lock;
    std::map<int, std::array<std::uint8_t, 512>>   frames;
};
