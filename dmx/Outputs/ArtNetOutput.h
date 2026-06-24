#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "IDmxOutput.h"

/** Sends DMX as Art-Net (ArtDmx, OpCode 0x5000) over UDP.

    Defaults to 127.0.0.1:6454 so it reaches a receiver on the same machine, e.g.
    BlenderDMX. To drive real fixtures later, point @p host at a hardware node's
    IP address — nothing else in the program changes.

    Note: this is a *send-only* socket. It transmits TO UDP port 6454 from an
    ephemeral local port, so it never competes with BlenderDMX (which binds 6454
    to receive).

    The UDP socket lives behind a pimpl so this header stays free of any
    networking-library include — the swap seam depends only on the standard
    library. @p host must be a numeric IP literal (no DNS lookup is performed).
*/
class ArtNetOutput : public IDmxOutput
{
public:
    explicit ArtNetOutput (std::string host = "127.0.0.1", int port = 6454);
    ~ArtNetOutput() override;

    bool open() override;
    void close() override;
    void sendUniverse (int universe, const std::uint8_t* data, int numChannels) override;
    const char* name() const override { return "Art-Net"; }

    ArtNetOutput (const ArtNetOutput&) = delete;
    ArtNetOutput& operator= (const ArtNetOutput&) = delete;

private:
    std::string  targetHost;
    int          targetPort;
    std::uint8_t sequence = 1;   // 1..255; 0 would disable resequencing

    struct Impl;                 // owns the Asio io_context + UDP socket
    std::unique_ptr<Impl> impl;
};
