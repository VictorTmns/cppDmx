#pragma once

#include <string>
#include <vector>

/** One candidate destination the engine could be pointed at. Kept deliberately
    transport-agnostic (just a human label plus host/port) so the UI can list
    Art-Net, sACN and — later — serial nodes side by side. */
struct DiscoveredOutput
{
    std::string description;   // what the user reads in the dropdown
    std::string host;          // unicast or broadcast target
    int         port = 6454;   // Art-Net default
};

/** Enumerate plausible Art-Net destinations reachable from this machine.

    Art-Net has no universal "list every node" call; nodes are normally found by
    broadcasting an ArtPoll and collecting replies (see discoverArtNetNodes). As a
    dependency-free stand-in we offer the destinations a packet can always be sent
    to from this machine:

      - the local loopback (a visualiser/monitor running on this same PC), and
      - the limited broadcast address (any node on the directly-attached link).

    Per-interface *directed*-broadcast enumeration was intentionally dropped when
    discovery moved off JUCE: building one broadcast address per NIC needs
    platform-specific interface enumeration, which no socket library provides. On
    a multi-homed host, reach a node on a secondary subnet by selecting it from
    discoverArtNetNodes() (which reports each responder's own IP) instead.

    Broadcasting works because ArtNetOutput enables SO_BROADCAST on its socket.

    When SacnOutput / UsbProOutput arrive they can append their own candidates
    (multicast groups, serial port names) to this same list. */
inline std::vector<DiscoveredOutput> discoverArtNetOutputs()
{
    return {
        { "Loopback (this PC) — 127.0.0.1",          "127.0.0.1",       6454 },
        { "Broadcast (limited) — 255.255.255.255",   "255.255.255.255", 6454 },
    };
}
