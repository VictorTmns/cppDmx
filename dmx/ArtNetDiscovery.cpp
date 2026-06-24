#include "ArtNetDiscovery.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>

#include <asio.hpp>

namespace
{
    constexpr unsigned short kArtNetPort = 6454;

    // Field offsets within an ArtPollReply (per the Art-Net 4 spec).
    constexpr int kIpOffset        = 10;   // 4 bytes, node's IP
    constexpr int kShortNameOffset = 26;   // 18 bytes, null-terminated
    constexpr int kLongNameOffset  = 44;   // 64 bytes, null-terminated
    constexpr int kNumPortsOffset  = 172;  // hi, lo
    constexpr int kMinReplyLen     = 174;  // enough to read everything above

    std::string readFixedString (const std::uint8_t* p, int maxLen)
    {
        int len = 0;
        while (len < maxLen && p[len] != 0)
            ++len;

        std::string s (reinterpret_cast<const char*> (p), (size_t) len);

        // Trim leading/trailing whitespace (names are often space- or null-padded).
        const auto isWs = [] (unsigned char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
        size_t b = 0, e = s.size();
        while (b < e && isWs ((unsigned char) s[b]))     ++b;
        while (e > b && isWs ((unsigned char) s[e - 1])) --e;
        return s.substr (b, e - b);
    }

    std::string ipFromBytes (const std::uint8_t* p)
    {
        return std::to_string (p[0]) + "." + std::to_string (p[1]) + "."
             + std::to_string (p[2]) + "." + std::to_string (p[3]);
    }
}

std::vector<DiscoveredNode> discoverArtNetNodes (int timeoutMs)
{
    std::vector<DiscoveredNode> nodes;

    try
    {
        asio::io_context      io;
        asio::ip::udp::socket socket (io);
        asio::error_code      ec;

        socket.open (asio::ip::udp::v4(), ec);
        if (ec)
            return nodes;

        // Share 6454 with a local monitor for the scan. SO_REUSEADDR covers the
        // Windows case; on some Unixes coexisting live sockets also want
        // SO_REUSEPORT, which we deliberately do not reach for (no platform code).
        socket.set_option (asio::socket_base::reuse_address (true), ec);
        socket.set_option (asio::socket_base::broadcast (true), ec);

        socket.bind (asio::ip::udp::endpoint (asio::ip::udp::v4(), kArtNetPort), ec);
        if (ec)
            return nodes;

        // --- Build and broadcast the ArtPoll ---------------------------------
        std::array<std::uint8_t, 14> poll {};
        std::memcpy (poll.data(), "Art-Net", 7);
        poll[7]  = 0;
        poll[8]  = 0x00;   // OpPoll 0x2000, little-endian
        poll[9]  = 0x20;
        poll[10] = 0x00;   // protocol version 14, high byte first
        poll[11] = 14;
        poll[12] = 0x00;   // TalkToMe
        poll[13] = 0x00;   // Priority

        // A node on this same PC, plus the limited broadcast for the local link.
        // (Per-interface directed broadcast was dropped with the JUCE removal.)
        for (const char* target : { "127.0.0.1", "255.255.255.255" })
        {
            const auto addr = asio::ip::make_address (target, ec);
            if (! ec)
                socket.send_to (asio::buffer (poll), asio::ip::udp::endpoint (addr, kArtNetPort), 0, ec);
        }

        // --- Collect ArtPollReply answers until the timeout ------------------
        // An async receive that re-arms itself, driven by run_for(): the io_context
        // runs handlers as replies arrive and returns once the window elapses,
        // abandoning the final pending receive. No manual poll loop or timing math.
        std::array<std::uint8_t, 600> buffer {};
        asio::ip::udp::endpoint       sender;

        std::function<void()> receiveNext = [&]
        {
            socket.async_receive_from (asio::buffer (buffer), sender,
                [&] (const asio::error_code& recErr, std::size_t n)
                {
                    if (recErr)
                        return;   // socket closing / cancelled: stop re-arming

                    const bool looksValid = n >= (std::size_t) kMinReplyLen
                                         && std::memcmp (buffer.data(), "Art-Net", 7) == 0
                                         && buffer[8] == 0x00 && buffer[9] == 0x21;  // OpPollReply 0x2100

                    if (looksValid)
                    {
                        DiscoveredNode node;
                        node.ip = ipFromBytes (buffer.data() + kIpOffset);
                        if (node.ip == "0.0.0.0")
                            node.ip = sender.address().to_string();    // fall back to packet source

                        node.shortName = readFixedString (buffer.data() + kShortNameOffset, 18);
                        node.longName  = readFixedString (buffer.data() + kLongNameOffset, 64);
                        node.numPorts  = (buffer[kNumPortsOffset] << 8) | buffer[kNumPortsOffset + 1];

                        const bool seen = std::any_of (nodes.begin(), nodes.end(),
                            [&] (const DiscoveredNode& e) { return e.ip == node.ip && e.shortName == node.shortName; });

                        if (! seen)
                            nodes.push_back (std::move (node));
                    }

                    receiveNext();   // wait for the next reply
                });
        };

        receiveNext();
        io.run_for (std::chrono::milliseconds (std::max (0, timeoutMs)));
    }
    catch (const std::exception&)
    {
        // Return whatever was collected before the failure.
    }

    return nodes;
}
