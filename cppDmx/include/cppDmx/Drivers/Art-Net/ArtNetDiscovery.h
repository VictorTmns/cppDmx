#pragma once
#include <cppDmx/cppDmx_export.h>

#include <vector>
#include <string>

namespace cppDmx
{
	struct DiscoveredArtNetNode
	{
		std::string shortName;   // ShortName field (<= 17 chars)
		std::string longName;    // LongName field  (<= 63 chars)
		std::string ip;          // the node's reported IP (where to send DMX)
		int numPorts = 0;
	};

	/** Enumerate plausible DMX destinations reachable from this machine. */
	CPPDMX_API std::vector<DiscoveredArtNetNode> discoverArtNetNodes(int timeoutMs = 1000);
}
