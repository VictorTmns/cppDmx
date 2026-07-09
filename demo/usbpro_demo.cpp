// Visual/manual bring-up for UsbProDriver: sweeps a lit fixture across one
// universe so you can confirm the whole path end-to-end against fake_usb_node
// (or real hardware). Build target: usbpro_demo.
//
// Requires a COM port — either a real Enttec DMX USB Pro widget, or one end
// of a virtual null-modem pair (e.g. com0com on Windows) with fake_usb_node
// listening on the other end.
//
//   usbpro_demo <portName> [universe]
//     portName  e.g. "COM3" (Windows) or "/dev/ttyUSB0" (Linux/macOS) — required,
//               there's no network-wide default the way Art-Net has localhost.
//     universe  which DMX universe to sweep (default 0) — this driver only
//               ever talks to one universe, matching the widget's hardware.
//
// Assumes a simple test rig: 8 x RGB fixtures, 3 channels each, addresses
// 1, 4, 7, ... 22.

#include "cppDmx/DmxEngine.h"
#include "cppDmx/Drivers/UsbPro/UsbProDriver.h"

#include <chrono>
#include <iostream>
#include <thread>

int main (int argc, char** argv)
{
    const std::string portName = argc > 1 ? argv[1] : "COM8";
    const int universe = argc > 2 ? std::stoi (argv[2]) : 0;

    cppDmx::DmxEngine engine;

    auto usbpro = std::make_unique<cppDmx::UsbProDriver> (portName, (std::uint32_t) universe);
    if (auto err = usbpro->Initialize())
    {
        std::cerr << "Failed to open " << portName << ": " << err.message() << "\n";
        return 1;
    }

    usbpro->Start (engine);                 // 25 Hz refresh rate (UsbProDriver's conservative default)

    std::cout << "Sending Enttec USB Pro frames to " << portName
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

    usbpro->Stop();
    usbpro->Shutdown();
}
