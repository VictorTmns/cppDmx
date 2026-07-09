#pragma once
#include <cppDmx/IDmxDriver.h>
#include <cppDmx/DmxEngine.h>

#include <map>
#include <algorithm>
#include <mutex>
#include <vector>

/** TEST-ONLY output. Stores the most recent frame for each universe in memory so
    a unit test can assert on exact byte values — no network, no hardware, fully
    deterministic.

    This file is self-contained and nothing in the core references it. To strip
    the test harness for release: delete this header, the tests/ folder, and the
    dmx_tests target in CMakeLists.txt. The controller and all real outputs are
    unaffected.
*/

class LoopbackOutput : public cppDmx::IDmxDriver
{
public:
	~LoopbackOutput() override = default;

    std::error_code Initialize() override { return {}; }
    std::error_code Shutdown() override { engine = nullptr; return {}; }

    void Start(const cppDmx::DmxEngine& e) override { engine = &e; }
    void Stop() override { engine = nullptr; }

    /** Synchronously copies whatever's currently in engine into frames. */
    void Flush() override
    {
        if (!engine)
            return;

        std::vector<cppDmx::DmxEngine::UniverseSnapshot> snaps;
        engine->snapshotAll(snaps);

        const std::lock_guard<std::mutex> sl(lock);
        for (auto& s : snaps)
            frames[s.universe] = s.data;
    }

    std::string GetDriverName() const override { return "Loopback (test)"; }

    /** Read back a channel (1..512) captured from the last frame for a universe. */
    std::uint8_t getChannel(int universe, int channel)
    {
        const std::lock_guard<std::mutex> sl(lock);
        auto it = frames.find(universe);
        if (it == frames.end() || channel < 1 || channel > 512)
            return 0;
        return it->second[(size_t)(channel - 1)];
    }

	bool IsRunning() const override { return true; }

	void SetErrorCallback(ErrorCallback callback) override {}

private:
    const cppDmx::DmxEngine*                       engine = nullptr;
    std::mutex                                     lock;
    std::map<int, std::array<std::uint8_t, 512>>   frames;
};
