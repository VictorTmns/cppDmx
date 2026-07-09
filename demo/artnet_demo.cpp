// Visual bring-up: sweeps a lit fixture across two universes so you can confirm
// the whole path end-to-end in BlenderDMX. Build target: artnet_demo.
//
// Assumes a simple test rig patched in BlenderDMX:
//   Universe 0: 8 x RGB fixtures, 3 channels each, addresses 1, 4, 7, ... 22
//   Universe 1: 8 x RGB fixtures, 3 channels each, addresses 1, 4, 7, ... 22
// (Art-Net universes count from 0.)

#include "cppDmx/DmxEngine.h"
#include "cppDmx/Drivers/Art-Net/ArtNetDriver.h"

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    cppDmx::DmxEngine engine;

    auto artnet = std::make_unique<cppDmx::ArtNetDriver> ("127.0.0.1");
    if (artnet->Initialize())
    {
        std::cerr << "Failed to open Art-Net socket\n";
        return 1;
    }

    artnet->Start (engine);                 // 40 Hz refresh rate (ArtNetDriver's default)

    std::cout << "Sending Art-Net to 127.0.0.1:6454. Press Ctrl+C to stop.\n";

    const int fixturesPerUniverse = 8;
    const int channelsPerFixture  = 3;         // R, G, B
    const int totalFixtures       = fixturesPerUniverse * 2;

    int step = 0;

    for (;;)
    {
        engine.clear();                        // turn everything off this frame

        const int lit      = step % totalFixtures;
        const int universe = lit / fixturesPerUniverse;         // 0 or 1
        const int index    = lit % fixturesPerUniverse;
        const int address  = 1 + index * channelsPerFixture;    // 1-based DMX address

        engine.setChannel (universe, address + 0, 255);   // R
        engine.setChannel (universe, address + 1, 255);   // G
        engine.setChannel (universe, address + 2, 255);   // B

        ++step;
        std::this_thread::sleep_for (std::chrono::milliseconds (150));
    }

    artnet->Stop();
    artnet->Shutdown();
}
