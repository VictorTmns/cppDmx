#include "ArtNetOutput.h"

#include <algorithm>
#include <array>
#include <cstring>

#include <asio.hpp>

// All Asio types are confined to this translation unit (pimpl), so no consumer
// of ArtNetOutput.h ever sees a networking-library header.
struct ArtNetOutput::Impl
{
    asio::io_context          io;
    asio::ip::udp::socket     socket { io };
    asio::ip::udp::endpoint   target;
    bool                      opened = false;
};

ArtNetOutput::ArtNetOutput (std::string host, int port)
    : targetHost (std::move (host)),
      targetPort (port),
      impl (std::make_unique<Impl>())
{
}

ArtNetOutput::~ArtNetOutput() = default;

bool ArtNetOutput::open()
{
    asio::error_code ec;

    impl->socket.open (asio::ip::udp::v4(), ec);
    if (ec)
        return false;

    // Bind to any free local port for sending. We send TO 6454 and do not need
    // to receive, so we deliberately do NOT bind 6454 — that leaves it free for
    // the visualiser (BlenderDMX) on the same machine.
    impl->socket.bind (asio::ip::udp::endpoint (asio::ip::udp::v4(), 0), ec);
    if (ec)
        return false;

    // Permit broadcast destinations (e.g. 255.255.255.255); harmless for unicast.
    impl->socket.set_option (asio::socket_base::broadcast (true), ec);

    const auto address = asio::ip::make_address (targetHost, ec);
    if (ec)
        return false;

    impl->target = asio::ip::udp::endpoint (address, (unsigned short) targetPort);
    impl->opened = true;
    return true;
}

void ArtNetOutput::close()
{
    if (! impl->opened)
        return;

    asio::error_code ec;
    impl->socket.close (ec);
    impl->opened = false;
}

void ArtNetOutput::sendUniverse (int universe, const std::uint8_t* data, int numChannels)
{
    if (! impl->opened)
        return;

    // Art-Net data length must be even and in the range 2..512.
    int len = std::clamp (numChannels, 2, 512);
    if ((len & 1) != 0)
        ++len;

    std::array<std::uint8_t, 18 + 512> packet {};

    // 0..7  : ID = "Art-Net" + null terminator
    std::memcpy (packet.data(), "Art-Net", 7);
    packet[7] = 0;

    // 8..9  : OpCode 0x5000 (OpDmx), transmitted little-endian (low byte first)
    packet[8]  = 0x00;
    packet[9]  = 0x50;

    // 10..11: protocol version 14, transmitted high byte first
    packet[10] = 0x00;
    packet[11] = 14;

    // 12    : sequence (1..255; 0 disables packet resequencing)
    packet[12] = sequence;
    sequence   = (std::uint8_t) (sequence == 255 ? 1 : sequence + 1);

    // 13    : physical input (informational only)
    packet[13] = 0x00;

    // 14    : SubUni — low 8 bits of the 15-bit Port-Address
    // 15    : Net    — high 7 bits of the 15-bit Port-Address
    packet[14] = (std::uint8_t) (universe & 0xFF);
    packet[15] = (std::uint8_t) ((universe >> 8) & 0x7F);

    // 16..17: data length, transmitted high byte first
    packet[16] = (std::uint8_t) ((len >> 8) & 0xFF);
    packet[17] = (std::uint8_t) (len & 0xFF);

    // 18..  : channel data (channel 1 == data[0])
    const int toCopy = std::clamp (numChannels, 0, len);
    std::memcpy (packet.data() + 18, data, (size_t) toCopy);

    asio::error_code ec;
    impl->socket.send_to (asio::buffer (packet.data(), (size_t) (18 + len)), impl->target, 0, ec);
}
