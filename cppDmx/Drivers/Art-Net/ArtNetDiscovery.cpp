#include "ArtNetDiscovery.h"

#include "Helpers.h"

#include <asio.hpp>
#include <algorithm>
#include <array>
#include <optional>


namespace cppDmx
{
    constexpr unsigned short kArtNetPort = 6454;
    constexpr std::uint16_t  kOpPoll = 0x2000;
    constexpr std::uint16_t  kOpPollReply = 0x2100;
    constexpr std::uint16_t  kProtocolVersion = 14;

    // ArtPoll (outgoing) layout
    constexpr std::size_t kOpcodeOffset = 8;    // opcode, little-endian (the reply's opcode lives here too)
    constexpr std::size_t kVersionOffset = 10;   // protocol version, big-endian

    // ArtPollReply (incoming) field offsets, per the Art-Net 4 spec
    constexpr std::size_t kIpOffset = 10;   // 4 bytes
    constexpr std::size_t kShortNameOffset = 26;
    constexpr std::size_t kShortNameLength = 18;   // null-terminated
    constexpr std::size_t kLongNameOffset = 44;
    constexpr std::size_t kLongNameLength = 64;   // null-terminated
    constexpr std::size_t kNumPortsOffset = 172;  // 2 bytes, big-endian
    constexpr std::size_t kMinReplyLen = 174;  // smallest packet covering all of the above


    // A fixed-width field is a run of characters ending at a null or the field's end.
    std::string readArtNetString(const std::uint8_t* field, std::size_t maxLength)
    {
        const auto* begin = reinterpret_cast<const char*> (field);
        const auto* end = std::find(begin, begin + maxLength, '\0');
        return trim(std::string(begin, end));
    }

    std::string ipv4ToString(const std::uint8_t* fourBytes)
    {
        const asio::ip::address_v4::bytes_type bytes{
            fourBytes[0], fourBytes[1], fourBytes[2], fourBytes[3]
        };
        return asio::ip::address_v4(bytes).to_string();
    }

    // --- Building and parsing packets ---------------------------------------

    std::array<std::uint8_t, 14> makeArtPollPacket()
    {
        std::array<std::uint8_t, 14> packet{};            // zero-initialised
        std::memcpy(packet.data(), "Art-Net", 8);         // 8-byte ID incl. null terminator

        writeLittleEndian16(packet.data() + kOpcodeOffset, kOpPoll);
        writeBigEndian16(packet.data() + kVersionOffset, kProtocolVersion);

        return packet;                                     // TalkToMe + Priority stay zero
    }

    std::optional<DiscoveredArtNetNode> parseArtPollReply(const std::uint8_t* packet,
        std::size_t length,
        const asio::ip::udp::endpoint& sender)
    {
        const bool isPollReply = length >= kMinReplyLen
            && std::memcmp(packet, "Art-Net", 7) == 0
            && readLittleEndian16(packet + kOpcodeOffset) == kOpPollReply;

        if (!isPollReply)
            return std::nullopt;

        DiscoveredArtNetNode node;

        node.ip = ipv4ToString(packet + kIpOffset);
        if (node.ip == "0.0.0.0")
            node.ip = sender.address().to_string();        // fall back to the packet's source

        node.shortName = readArtNetString(packet + kShortNameOffset, kShortNameLength);
        node.longName = readArtNetString(packet + kLongNameOffset, kLongNameLength);
        node.numPorts = readBigEndian16(packet + kNumPortsOffset);

        return node;
    }

    // --- Socket operations --------------------------------------------------

    bool openArtNetSocket(asio::ip::udp::socket& socket)
    {
        asio::error_code ec;

        socket.open(asio::ip::udp::v4(), ec);
        if (ec)
            return false;

        socket.set_option(asio::socket_base::reuse_address(true), ec); // share 6454 with a local monitor
        socket.set_option(asio::socket_base::broadcast(true), ec);     // reach nodes on the local link

        socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), kArtNetPort), ec);
        return !ec;
    }

    void sendArtPoll(asio::ip::udp::socket& socket)
    {
        const auto packet = makeArtPollPacket();
        asio::error_code ec;

        // Loopback reaches a node on this PC; the limited broadcast reaches the local link.
        for (const char* target : { "127.0.0.1", "255.255.255.255" })
        {
            const auto destination = asio::ip::make_address(target, ec);
            if (!ec)
                socket.send_to(asio::buffer(packet),
                    asio::ip::udp::endpoint(destination, kArtNetPort), 0, ec);
        }
    }

    std::vector<DiscoveredArtNetNode> collectArtPollReplies(asio::ip::udp::socket& socket,
        asio::io_context& io,
        int timeoutMs)
    {
        std::vector<DiscoveredArtNetNode>   nodes;
        std::array<std::uint8_t, 600> buffer{};
        asio::ip::udp::endpoint       sender;

        const auto addIfNew = [&nodes](DiscoveredArtNetNode node)
            {
                const auto isSameNode = [&](const DiscoveredArtNetNode& other)
                    {
                        return other.ip == node.ip && other.shortName == node.shortName;
                    };

                if (std::none_of(nodes.begin(), nodes.end(), isSameNode))
                    nodes.push_back(std::move(node));
            };

        // A self-re-arming receive: each handler queues the next one, and run_for()
        // below pumps them until the timeout elapses, abandoning the final pending
        // receive. No manual poll loop or timing arithmetic.
        std::function<void()> receiveNext = [&]
            {
                socket.async_receive_from(asio::buffer(buffer), sender,
                    [&](const asio::error_code& ec, std::size_t bytesReceived)
                    {
                        if (ec)
                            return;   // socket cancelled or closed: stop listening

                        if (auto node = parseArtPollReply(buffer.data(), bytesReceived, sender))
                            addIfNew(std::move(*node));

                        receiveNext();
                    });
            };

        receiveNext();
        io.run_for(std::chrono::milliseconds(std::max(0, timeoutMs)));

        return nodes;
    }
    
    std::vector<DiscoveredArtNetNode> discoverArtNetNodes(int timeoutMs)
    {
        asio::io_context      io;
        asio::ip::udp::socket socket(io);
    
        if (!openArtNetSocket(socket))
            return {};
    
        sendArtPoll(socket);
        return collectArtPollReplies(socket, io, timeoutMs);
    }
}