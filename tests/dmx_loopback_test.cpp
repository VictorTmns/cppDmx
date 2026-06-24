// Minimal, framework-free test of the DMX value path through the engine.
// Build target: dmx_tests. Delete this folder (and the LoopbackOutput header and
// the dmx_tests CMake target) to remove the test harness entirely.

#include "../dmx/DmxEngine.h"
#include "../dmx/Outputs/LoopbackOutput.h"

#include <iostream>

static bool check (bool ok, const char* msg)
{
    if (! ok) std::cerr << "FAIL: " << msg << '\n';
    return ok;
}

int main()
{
    DmxEngine engine;

    auto loopback = std::make_unique<LoopbackOutput>();
    auto* probe   = loopback.get();          // non-owning pointer to read back
    engine.setOutput (std::move (loopback));

    engine.setChannel (0, 1, 255);           // universe 0, channel 1
    engine.setChannel (0, 2, 128);
    engine.setChannel (3, 10, 64);           // a second, higher-numbered universe

    engine.flushOnce();                      // synchronous push — no threads, no network

    bool ok = true;
    ok &= check (probe->getChannel (0, 1)  == 255, "universe 0 ch 1 == 255");
    ok &= check (probe->getChannel (0, 2)  == 128, "universe 0 ch 2 == 128");
    ok &= check (probe->getChannel (3, 10) == 64,  "universe 3 ch 10 == 64");
    ok &= check (probe->getChannel (0, 3)  == 0,   "untouched channel stays 0");

    if (ok) std::cout << "DMX loopback tests passed\n";
    return ok ? 0 : 1;
}
