#pragma once

#include <string>
#include <vector>

/** One Art-Net node that answered an ArtPoll. */
struct DiscoveredNode
{
    std::string shortName;   // ShortName field (<= 17 chars)
    std::string longName;    // LongName field  (<= 63 chars)
    std::string ip;          // the node's reported IP (where to send DMX)
    int         numPorts = 0;
};

/** Actively discover Art-Net nodes on the local links.

    Broadcasts an ArtPoll (OpCode 0x2000) to loopback (127.0.0.1) and the limited
    broadcast address (255.255.255.255), then collects ArtPollReply (0x2100)
    answers for @p timeoutMs milliseconds.

    Blocks for up to @p timeoutMs, so call it off the message thread. Returns the
    de-duplicated set of responders. Binds UDP 6454 with address-reuse for the
    duration of the scan, so it coexists with a monitor/visualiser also on 6454.
    */
std::vector<DiscoveredNode> discoverArtNetNodes (int timeoutMs = 1000);
