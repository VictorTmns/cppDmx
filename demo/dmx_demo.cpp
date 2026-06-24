// Visual bring-up: sweeps a lit fixture across two universes so you can confirm
// the whole path end-to-end in BlenderDMX. Build target: dmx_demo.
//
// Assumes a simple test rig patched in BlenderDMX:
//   Universe 0: 8 x RGB fixtures, 3 channels each, addresses 1, 4, 7, ... 22
//   Universe 1: 8 x RGB fixtures, 3 channels each, addresses 1, 4, 7, ... 22
// (Art-Net universes count from 0.)

#include "../dmx/DmxEngine.h"
#include "../dmx/Outputs/ArtNetOutput.h"

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    DmxEngine engine;

    auto artnet = std::make_unique<ArtNetOutput> ("127.0.0.1", 6454);
    if (! artnet->open())
    {
        std::cerr << "Failed to open Art-Net socket\n";
        return 1;
    }

    engine.setOutput (std::move (artnet));
    engine.start (40.0);                       // 40 frames per second

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
}
