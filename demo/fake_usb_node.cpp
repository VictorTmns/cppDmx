// Fake Enttec USB Pro widget receiver: opens a real COM port and decodes any
// incoming "Output Only Send DMX Packet" (label 6) frames, logging the
// decoded channel count. Requires a real widget-side loopback (real hardware,
// or an OS-level virtual serial port pair such as com0com on Windows / socat
// on Linux) to actually receive anything — NOT exercised end-to-end in this
// session, since neither is available here. Unlike fake_node.cpp there is no
// discovery handshake at this protocol layer (the OS enumerates COM ports
// directly) and label 6 is one-way, so this never sends a reply.
//
//   fake_usb_node <portName> [label]
//     portName  e.g. "COM4" (Windows) or "/dev/ttyUSB0" (Linux/macOS) — required,
//               there's no network-wide default the way Art-Net has localhost.
//     label     an optional display-only tag for your own bookkeeping — the
//               Enttec wire format carries no universe number at all.
//
// JUCE-free: standard library + Asio (same as fake_node.cpp).

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <asio.hpp>

int main (int argc, char** argv)
{

    const std::string portName = argc > 1 ? argv[1] : "COM9";
    const std::string label    = argc > 2 ? argv[2] : "testing usb pro";

    std::cout << std::unitbuf;

    try
    {
        asio::io_context   io;
        asio::serial_port  port (io);
        asio::error_code   ec;

        port.open (portName, ec);
        if (ec) 
        {
            std::cerr << "Could not open " << portName << ": " << ec.message() << "\n";
            return 1;
        }

        port.set_option (asio::serial_port_base::baud_rate (57600), ec);
        port.set_option (asio::serial_port_base::character_size (8), ec);
        port.set_option (asio::serial_port_base::parity (asio::serial_port_base::parity::none), ec);
        port.set_option (asio::serial_port_base::stop_bits (asio::serial_port_base::stop_bits::two), ec);
        // (same unconfirmed-baud-rate caveat as UsbProDriver::Initialize())

        std::cout << "Fake USB Pro widget \"" << label << "\" listening on " << portName << ". Ctrl+C to stop.\n";

        enum class State { SeekStart, ReadHeader, ReadPayload, ReadEnd };
        State state = State::SeekStart;

        std::uint8_t label6 = 0;
        std::size_t  payloadLen = 0, needed = 0;
        std::vector<std::uint8_t> header, payload;

        for (;;)
        {
            std::uint8_t byte = 0;
            asio::read (port, asio::buffer (&byte, 1), ec);
            if (ec) break; // port closed / error

            switch (state)
            {
            case State::SeekStart:
                if (byte == 0x7E) { state = State::ReadHeader; header.clear(); needed = 3; }
                break; // discard anything else while resyncing

            case State::ReadHeader:
                header.push_back (byte);
                if (--needed == 0)
                {
                    label6     = header[0];
                    payloadLen = header[1] | (header[2] << 8); // length, little-endian
                    payload.clear();
                    needed = payloadLen;
                    state  = payloadLen > 0 ? State::ReadPayload : State::ReadEnd;
                }
                break;

            case State::ReadPayload:
                payload.push_back (byte);
                if (--needed == 0) state = State::ReadEnd;
                break;

            case State::ReadEnd:
                if (byte == 0xE7)
                {
                    const int channels = payload.empty() ? 0 : (int) payload.size() - 1;
                    std::cout << "[" << label << "] label=" << (int) label6
                              << " startCode=0x" << std::hex << (payload.empty() ? 0 : (int) payload[0]) << std::dec
                              << " channels=" << channels << "\n";
                }
                else
                {
                    std::cerr << "[" << label << "] framing error: expected 0xE7, got 0x"
                              << std::hex << (int) byte << std::dec << " - resyncing\n";
                }
                state = State::SeekStart;
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "fake_usb_node error: " << e.what() << "\n";
        return 1;
    }
}
