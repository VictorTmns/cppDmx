#include "DmxUniverse.h"

#include <algorithm>

namespace cppDmx
{
	DmxUniverse::DmxUniverse()
	{
		data.fill(0);
	}
	DmxUniverse::~DmxUniverse()
	{}
	void DmxUniverse::setChannel(std::uint8_t channel, std::uint8_t value)
	{
		if (channel < 1 || channel > 512)
			return;
		data[channel - 1] = value;
	}
	void DmxUniverse::setUniverse(const std::uint8_t* data, int numChannels)
	{
		this->data.fill(0);
		std::memcpy(this->data.data(), data, std::clamp(numChannels, 0, 512));
	}
	const std::array<std::uint8_t, 512>& DmxUniverse::getData() const
	{
		return data;
	}
}