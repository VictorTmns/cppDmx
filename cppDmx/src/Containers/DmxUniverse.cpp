#include <cppDmx/Containers/DmxUniverse.h>

#include <algorithm>
#include <cstring>

namespace cppDmx
{
	DmxUniverse::DmxUniverse()
	{
		data.fill(0);
	}
	DmxUniverse::~DmxUniverse()
	{}
	void DmxUniverse::setChannel(int channel, std::uint8_t value)
	{
		if (channel < 1 || channel > 512)
			return;
		data[static_cast<std::size_t>(channel - 1)] = value;
	}
	void DmxUniverse::setUniverse(const std::uint8_t* data, int numChannels)
	{
		this->data.fill(0);

		const int toCopy = std::clamp(numChannels, 0, 512);
		if (toCopy > 0)
			std::memcpy(this->data.data(), data, static_cast<std::size_t>(toCopy));
	}
	const std::array<std::uint8_t, 512>& DmxUniverse::getData() const
	{
		return data;
	}
}
