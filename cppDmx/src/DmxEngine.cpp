#include <cppDmx/DmxEngine.h>

#include <cassert>
#include <cstring>

namespace cppDmx
{
	void DmxEngine::setChannel(int universe, int channel, std::uint8_t value)
	{
		assert(channel >= 1 && channel <= 512);
		{
			const std::lock_guard<std::mutex> sl(lock);
			universes[universe].setChannel(channel, value);
			++changeVersion;
		}
		wakeUp.notify_all();
	}

	void DmxEngine::setUniverse(int universe, const std::uint8_t* data, int numChannels)
	{
		assert(numChannels <= 512);
		{
			const std::lock_guard<std::mutex> sl(lock);
			universes[universe].setUniverse(data, numChannels);
			++changeVersion;
		}
		wakeUp.notify_all();
	}

	void DmxEngine::clear()
	{
		{
			const std::lock_guard<std::mutex> sl(lock);
			for (auto& u : universes)
				u.second.setUniverse(nullptr, 0);
			++changeVersion;
		}
		wakeUp.notify_all();
	}

	bool DmxEngine::isUniverse(int universe) const
	{
		const std::lock_guard<std::mutex> sl(lock);
		return universes.find(universe) != universes.end();
	}

	void DmxEngine::getUniverseData(int universe, std::uint8_t* data) const
	{
		const std::lock_guard<std::mutex> sl(lock);

		auto it = universes.find(universe);
		if (it == universes.end())
		{
			std::memset(data, 0, 512);
			return;
		}

		std::memcpy(data, it->second.getData().data(), 512);
	}

	void DmxEngine::getUniverseNumbers(std::uint32_t* data, std::uint8_t length) const
	{
		const std::lock_guard<std::mutex> sl(lock);

		std::uint8_t i = 0;
		for (const auto& u : universes)
		{
			if (i >= length)
				break;
			data[i++] = u.first;
		}
	}

	void DmxEngine::snapshotAll(std::vector<UniverseSnapshot>& out) const
	{
		const std::lock_guard<std::mutex> sl(lock);

		out.clear();
		out.reserve(universes.size());
		for (const auto& [num, universe] : universes)
			out.push_back(UniverseSnapshot{ num, universe.getData() });
	}

	std::uint64_t DmxEngine::getChangeVersion() const
	{
		const std::lock_guard<std::mutex> sl(lock);
		return changeVersion;
	}

	std::uint64_t DmxEngine::waitForChange(std::uint64_t lastSeenVersion, std::chrono::milliseconds timeout) const
	{
		std::unique_lock<std::mutex> sl(lock);
		wakeUp.wait_for(sl, timeout, [&] { return changeVersion != lastSeenVersion; });
		return changeVersion;
	}
}
