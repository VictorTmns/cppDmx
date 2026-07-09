// Framework-free test of the Enttec USB Pro wire format. Pure byte-layout
// check — no engine, no driver instantiation, no real or virtual serial
// port (none is available in CI or this environment). Build target:
// usbpro_framing_test.

#include <cppDmx/Drivers/UsbPro/UsbProDriver.h>

#include <iostream>

static bool check (bool ok, const char* msg)
{
    if (! ok) std::cerr << "FAIL: " << msg << '\n';
    return ok;
}

int main()
{
    std::array<std::uint8_t, 512> data{};
    data[0]   = 255;   // channel 1
    data[1]   = 128;   // channel 2
    data[511] = 64;    // channel 512

    const auto frame = cppDmx::encodeUsbProSendDmx (0x00, data);

    bool ok = true;
    ok &= check (frame.size() == 4 + 513 + 1, "frame length == 4 header + 513 payload + 1 trailer");
    ok &= check (frame[0] == 0x7E, "starts with 0x7E delimiter");
    ok &= check (frame[1] == 6,    "label == 6 (Output Only Send DMX Packet)");
    ok &= check (frame[2] == 0x01 && frame[3] == 0x02, "length == 513, little-endian (0x01 0x02)");
    ok &= check (frame[4] == 0x00, "payload[0] == DMX start code");
    ok &= check (frame[5] == 255,  "payload[1] == channel 1");
    ok &= check (frame[6] == 128,  "payload[2] == channel 2");
    ok &= check (frame[4 + 512] == 64, "payload[512] == channel 512");
    ok &= check (frame.back() == 0xE7, "ends with 0xE7 delimiter");

    if (ok) std::cout << "USB Pro framing tests passed\n";
    return ok ? 0 : 1;
}
