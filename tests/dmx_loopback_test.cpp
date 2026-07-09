// Minimal, framework-free test of the DMX value path through the engine.
// Build target: dmx_tests. Delete this folder (and the LoopbackOutput header and
// the dmx_tests CMake target) to remove the test harness entirely.

#include <cppDmx/DmxEngine.h>
#include "LoopbackDriver.h"

#include <iostream>
#include <thread>
#include <chrono>

static bool check (bool ok, const char* msg)
{
    if (! ok) std::cerr << "FAIL: " << msg << '\n';
    return ok;
}

int main()
{
    cppDmx::DmxEngine engine;

    LoopbackOutput loopback;
    loopback.Initialize();
    loopback.Start (engine);

    engine.setChannel (0, 1, 255);           // universe 0, channel 1
    engine.setChannel (0, 2, 128);
    engine.setChannel (3, 10, 64);           // a second, higher-numbered universe
    engine.setChannel (0, 300, 200);         // channel beyond a byte's range (regression test)

    loopback.Flush();                        // synchronous push — no threads, no network

    bool ok = true;
    ok &= check (loopback.getChannel (0, 1)   == 255, "universe 0 ch 1 == 255");
    ok &= check (loopback.getChannel (0, 2)   == 128, "universe 0 ch 2 == 128");
    ok &= check (loopback.getChannel (3, 10)  == 64,  "universe 3 ch 10 == 64");
    ok &= check (loopback.getChannel (0, 3)   == 0,   "untouched channel stays 0");
    ok &= check (loopback.getChannel (0, 300) == 200, "universe 0 ch 300 == 200 (not truncated to ch 44)");
    ok &= check (loopback.getChannel (0, 44)  == 0,   "ch 44 unaffected by ch 300 write (would collide if truncated)");

    ok &= check (engine.isUniverse (0)  == true,  "isUniverse(0) true after writes");
    ok &= check (engine.isUniverse (99) == false, "isUniverse(99) false, never written");

    std::uint8_t universeData[512];
    engine.getUniverseData (0, universeData);
    ok &= check (universeData[0] == 255, "getUniverseData(0) ch 1 == 255");
    ok &= check (universeData[299] == 200, "getUniverseData(0) ch 300 == 200");

    std::uint32_t universeNumbers[8] = {};
    engine.getUniverseNumbers (universeNumbers, 8);
    bool sawUniverse0 = false, sawUniverse3 = false;
    for (auto n : universeNumbers)
    {
        if (n == 0) sawUniverse0 = true;
        if (n == 3) sawUniverse3 = true;
    }
    ok &= check (sawUniverse0 && sawUniverse3, "getUniverseNumbers reports universes 0 and 3");

    // --- snapshotAll ---------------------------------------------------------
    std::vector<cppDmx::DmxEngine::UniverseSnapshot> snaps;
    engine.snapshotAll (snaps);
    bool foundUniverse0 = false;
    for (auto& s : snaps)
        if (s.universe == 0) { foundUniverse0 = true; ok &= check (s.data[0] == 255, "snapshotAll universe 0 ch1 == 255"); }
    ok &= check (foundUniverse0, "snapshotAll includes universe 0");

    const auto snapCapacity = snaps.capacity();
    engine.snapshotAll (snaps); // same universe count as before
    ok &= check (snaps.capacity() == snapCapacity, "snapshotAll reuses caller buffer capacity (no realloc when universe count is stable)");

    // --- getChangeVersion / waitForChange ------------------------------------
    const auto v0 = engine.getChangeVersion();
    engine.setChannel (7, 1, 9);
    const auto v1 = engine.getChangeVersion();
    ok &= check (v1 != v0, "changeVersion advances after setChannel");

    {
        // already-stale version: returns immediately
        engine.setChannel (7, 2, 5);
        const auto returned = engine.waitForChange (v1, std::chrono::milliseconds (2000));
        ok &= check (returned != v1, "waitForChange returns immediately when version already changed");
    }

    {
        // nothing changes: times out returning the unchanged version
        const auto v = engine.getChangeVersion();
        const auto returned = engine.waitForChange (v, std::chrono::milliseconds (50));
        ok &= check (returned == v, "waitForChange times out returning the unchanged version");
    }

    {
        // wakes promptly from another thread's mutation
        const auto v = engine.getChangeVersion();
        std::thread t ([&]
        {
            std::this_thread::sleep_for (std::chrono::milliseconds (20));
            engine.setChannel (7, 3, 1);
        });

        const auto start   = std::chrono::steady_clock::now();
        const auto returned = engine.waitForChange (v, std::chrono::milliseconds (2000));
        const auto elapsed  = std::chrono::steady_clock::now() - start;
        t.join();

        ok &= check (returned != v, "waitForChange wakes when another thread mutates");
        ok &= check (elapsed < std::chrono::milliseconds (500), "waitForChange wakes promptly, not after the full timeout");
    }

    loopback.Stop();
    loopback.Shutdown();

    if (ok) std::cout << "DMX loopback tests passed\n";
    return ok ? 0 : 1;
}
