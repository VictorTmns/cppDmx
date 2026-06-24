// Fake Art-Net node: a test stand-in that pretends to be a DMX output device.
//
// It binds UDP 6454, answers any ArtPoll with an ArtPollReply (so the GUI's
// "Detect" button discovers it by name), and prints any ArtDmx frames it
// receives (so you can confirm the controller is sending to the right universe).
//
// Build target: fake_node. Run it, then press Detect in the DMX Controller.
//
//   fake_node [shortName] [reportIp]
//     shortName  name shown in the controller's list   (default "Fake DMX Node")
//     reportIp   IP the node advertises / DMX target    (default 127.0.0.1)
//
// JUCE-free: standard library + Asio (the same UDP stack as dmx_core).

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <asio.hpp>

namespace
{
    constexpr unsigned short kArtNetPort = 6454;

    constexpr int OpPoll      = 0x2000; // ArtPoll: a discovery request
    constexpr int OpDmx       = 0x5000; // ArtDmx: DMX data for one universe
    constexpr int OpPollReply = 0x2100; // ArtPollReply: announces this node

    void copyFixedString (std::uint8_t* dst, int capacity, const std::string& s)
    {
        int i = 0;
        for (; i < capacity - 1 && i < (int) s.size() && s[(size_t) i] != 0; ++i)
            dst[i] = (std::uint8_t) s[(size_t) i];
        dst[i] = 0;
    }

    // Parse an IPv4 literal into its four octets (0 for any malformed field).
    std::array<int, 4> parseOctets (const std::string& ip)
    {
        std::array<int, 4> out { { 0, 0, 0, 0 } };
        size_t start = 0;
        for (int i = 0; i < 4; ++i)
        {
            const size_t dot = ip.find ('.', start);
            const std::string part = ip.substr (start, dot == std::string::npos ? std::string::npos : dot - start);

            int value = 0;
            for (char c : part)
            {
                if (c < '0' || c > '9') { value = 0; break; }
                value = value * 10 + (c - '0');
            }
            out[(size_t) i] = value & 0xFF;

            if (dot == std::string::npos)
                break;
            start = dot + 1;
        }
        return out;
    }

    std::vector<std::uint8_t> buildPollReply (const std::string& shortName,
                                              const std::string& longName,
                                              const std::string& ip)
    {
        // ArtPollReply packets are always 239 bytes.
        std::vector<std::uint8_t> p (239, 0);

        // Art-Net packets always start with "Art-Net" followed by a null byte.
        std::memcpy (p.data(), "Art-Net", 7);
        p[7] = 0;

        // OpPollReply, transmitted little-endian (low byte first). NB: writing the
        // 16-bit opcode straight into one byte truncates it to 0x00 — both bytes
        // must be set explicitly.
        p[8] = (std::uint8_t) (OpPollReply & 0xFF);
        p[9] = (std::uint8_t) ((OpPollReply >> 8) & 0xFF);

        const auto octets = parseOctets (ip);
        for (int i = 0; i < 4; ++i)
            p[10 + i] = (std::uint8_t) octets[(size_t) i];

        // Port 6454, little-endian.
        p[14] = (std::uint8_t) (kArtNetPort & 0xFF);
        p[15] = (std::uint8_t) ((kArtNetPort >> 8) & 0xFF);

        // Firmware version (arbitrary).
        p[16] = 0x00; p[17] = 14;

        copyFixedString (p.data() + 26, 18, shortName);
        copyFixedString (p.data() + 44, 64, longName);

        p[172] = 0x00; p[173] = 0x01;        // NumPorts = 1
        p[174] = 0x80;                       // PortTypes[0]: output, DMX512
        p[182] = 0x80;                       // GoodOutput[0]: data transmitted

        return p;
    }
}

int main (int argc, char** argv)
{
    const std::string shortName = argc > 1 ? argv[1] : "Fake DMX Node";
    const std::string reportIp  = argc > 2 ? argv[2] : "127.0.0.1";

    std::cout << std::unitbuf;                   // flush every line so logs appear live / when piped

    try
    {
        asio::io_context      io;
        asio::ip::udp::socket socket (io);
        asio::error_code      ec;

        socket.open (asio::ip::udp::v4(), ec);
        if (ec)
        {
            std::cerr << "Could not open UDP socket: " << ec.message() << "\n";
            return 1;
        }

        socket.set_option (asio::socket_base::reuse_address (true), ec);  // coexist with other 6454 listeners
        socket.set_option (asio::socket_base::broadcast (true), ec);      // reply by broadcast as well

        socket.bind (asio::ip::udp::endpoint (asio::ip::udp::v4(), kArtNetPort), ec);
        if (ec)
        {
            std::cerr << "Could not bind UDP " << kArtNetPort
                      << " (is another Art-Net app holding it without port-reuse?): " << ec.message() << "\n";
            return 1;
        }

        std::cout << "Fake Art-Net node \"" << shortName << "\" listening on UDP " << kArtNetPort
                  << ", advertising IP " << reportIp << ".\n"
                  << "Press Detect in the DMX Controller. Ctrl+C to stop.\n";

        const auto reply = buildPollReply (shortName, shortName, reportIp);
        std::array<std::uint8_t, 600> buffer {};

        for (;;)
        {
            asio::ip::udp::endpoint sender;
            const std::size_t numBytes = socket.receive_from (asio::buffer (buffer), sender, 0, ec);

            if (ec)
                continue;   // transient receive error: keep listening

            if (numBytes < 12 || std::memcmp (buffer.data(), "Art-Net", 7) != 0)
                continue;   // not an Art-Net packet

            const std::string senderIp = sender.address().to_string();
            const int opcode = buffer[8] | (buffer[9] << 8);   // Art-Net opcodes are little-endian

            if (opcode == OpPoll)   // ArtPoll: announce ourselves with an ArtPollReply
            {
                std::cout << "ArtPoll from " << senderIp << " - replying as \"" << shortName << "\".\n";
                socket.send_to (asio::buffer (reply),
                                asio::ip::udp::endpoint (sender.address(), kArtNetPort), 0, ec);
                socket.send_to (asio::buffer (reply),
                                asio::ip::udp::endpoint (asio::ip::make_address ("255.255.255.255"), kArtNetPort), 0, ec);
            }
            else if (opcode == OpDmx)   // ArtDmx: print the universe and channel count (ignore the DMX data)
            {
                const int universe = buffer[14] | (buffer[15] << 8);
                const int length   = (buffer[16] << 8) | buffer[17];
                std::cout << "ArtDmx universe " << universe << " (" << length << " channels) from " << senderIp << ".\n";
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "fake_node error: " << e.what() << "\n";
        return 1;
    }
}
