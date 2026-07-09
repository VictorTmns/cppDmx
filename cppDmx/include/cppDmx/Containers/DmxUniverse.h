#pragma once
#include <array>
#include <cstdint>

namespace cppDmx
{
	class DmxUniverse
	{
	public:
		DmxUniverse();
		~DmxUniverse();
		/** @p channel is 1..512 (DMX convention); out-of-range values are ignored. */
		void setChannel(int channel, std::uint8_t value);
		void setUniverse(const std::uint8_t* data, int numChannels);
		const std::array<std::uint8_t, 512>& getData() const;

	private:
		std::array<std::uint8_t, 512> data;
	};
}
