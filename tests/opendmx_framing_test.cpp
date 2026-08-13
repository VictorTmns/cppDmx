// Framework-free test of the raw DMX-512 slot layout produced by OpenDmxDriver.
// The BREAK / mark-after-break is a line-level signal (not bytes) and needs real
// serial hardware, so it is not covered here — only the byte payload is. Build
// target: opendmx_framing_test.

#include <cppDmx/Drivers/OpenDmx/OpenDmxDriver.h>

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

    const auto frame = cppDmx::encodeOpenDmxFrame (0x00, data);

    bool ok = true;
    ok &= check (frame.size() == 1 + 512, "frame length == 1 start code + 512 slots");
    ok &= check (frame[0] == 0x00,   "slot 0 == DMX start code");
    ok &= check (frame[1] == 255,    "slot 1 == channel 1");
    ok &= check (frame[2] == 128,    "slot 2 == channel 2");
    ok &= check (frame[512] == 64,   "slot 512 == channel 512");

    // A non-zero start code (e.g. RDM's 0xCC) must pass through unchanged.
    const auto rdm = cppDmx::encodeOpenDmxFrame (0xCC, data);
    ok &= check (rdm[0] == 0xCC, "custom start code preserved");

    if (ok) std::cout << "Open DMX framing tests passed\n";
    return ok ? 0 : 1;
}
