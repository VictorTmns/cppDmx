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
		void setChannel(std::uint8_t channel, std::uint8_t value);
		void setUniverse(const std::uint8_t* data, int numChannels);
		const std::array<std::uint8_t, 512>& getData() const;
		std::uint32_t getUniverseNumber() const;


	private:
		std::array<std::uint8_t, 512> data;
	};

	class DmxUniverseHash
	{
	public:
		std::size_t operator()(const DmxUniverse& universe) const
		{
			return static_cast<std::size_t>(universe.getUniverseNumber());
		}
	};
}
