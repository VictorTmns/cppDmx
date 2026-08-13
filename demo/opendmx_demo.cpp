// Visual/manual bring-up for OpenDmxDriver: sweeps a lit fixture across one
// universe over a raw RS-485 / FTDI serial adapter speaking DMX-512 directly.
// Build target: opendmx_demo.
//
// Requires a serial port wired to a DMX line — an FTDI/RS-485 adapter to real
// fixtures, or a loopback/analyzer for inspection. Unlike the USB Pro path there
// is no smart widget generating the timing: this driver asserts the BREAK itself,
// so it needs a port whose adapter honours 250 kbaud and break control.
//
//   opendmx_demo <portName> [universe]
//     portName  e.g. "COM3" (Windows) or "/dev/ttyUSB0" (Linux/macOS) — required,
//               there's no network-wide default the way Art-Net has localhost.
//     universe  which DMX universe to sweep (default 0) — one line, one universe.
//
// Assumes a simple test rig: 8 x RGB fixtures, 3 channels each, addresses
// 1, 4, 7, ... 22.

#include "cppDmx/DmxEngine.h"
#include "cppDmx/Drivers/OpenDmx/OpenDmxDriver.h"

#include <chrono>
#include <iostream>
#include <thread>

int main (int argc, char** argv)
{
    const std::string portName = argc > 1 ? argv[1] : "COM8";
    const int universe = argc > 2 ? std::stoi (argv[2]) : 0;

    cppDmx::DmxEngine engine;

    auto driver = std::make_unique<cppDmx::OpenDmxDriver> (portName, (std::uint32_t) universe);
    if (auto err = driver->Initialize())
    {
        std::cerr << "Failed to open " << portName << ": " << err.message() << "\n";
        return 1;
    }

    driver->Start (engine);                 // 30 Hz refresh rate (OpenDmxDriver's conservative default)

    std::cout << "Sending raw DMX-512 to " << portName
              << " on universe " << universe << ". Press Ctrl+C to stop.\n";

    constexpr int fixtures       = 8;
    constexpr int channelsPerFix = 3;         // R, G, B

    int step = 0;

    for (;;)
    {
        engine.clear();                        // turn everything off this frame

        const int lit     = step % fixtures;
        const int address = 1 + lit * channelsPerFix;    // 1-based DMX address

        engine.setChannel (universe, address + 0, 255);   // R
        engine.setChannel (universe, address + 1, 255);   // G
        engine.setChannel (universe, address + 2, 255);   // B

        ++step;
        std::this_thread::sleep_for (std::chrono::milliseconds (150));
    }

    driver->Stop();
    driver->Shutdown();
}
