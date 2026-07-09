#pragma once
#include <unordered_map>
#include <vector>
#include <array>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <cstdint>

#include <cppDmx/cppDmx_export.h>
#include <cppDmx/Containers/DmxUniverse.h>

namespace cppDmx
{
	class CPPDMX_API DmxEngine
	{
	public:
		DmxEngine() = default;
		~DmxEngine() = default;

		DmxEngine(const DmxEngine&) = delete;
		DmxEngine& operator= (const DmxEngine&) = delete;

		/** Set a single channel. @p channel is 1..512 (DMX convention). */
		void setChannel(int universe, int channel, std::uint8_t value);

		/** Overwrite a whole universe at once (remaining channels are zeroed). */
		void setUniverse(int universe, const std::uint8_t* data, int numChannels);

		/** Zeroes every known universe. */
		void clear();

		/** Returns if @p universe has any existing data*/
		bool isUniverse(int universe) const;

		/** Gives all the data in @p universe, the data is always 512 bytes long*/
		void getUniverseData(int universe, std::uint8_t* data) const;

		/** Provides all the universes id's that have existing data*/
		void getUniverseNumbers(std::uint32_t* data, std::uint8_t length) const;

		struct UniverseSnapshot
		{
			std::uint32_t                 universe;
			std::array<std::uint8_t, 512> data;
		};

		/** Copies one snapshot per known universe under a single lock/unlock pair.
			@p out is cleared and refilled; a caller that reuses the same vector
			every tick incurs no heap allocation once the universe count
			stabilizes. This is the primitive drivers use to pull data out cheaply
			and do their I/O outside the engine's lock. */
		void snapshotAll(std::vector<UniverseSnapshot>& out) const;

		/** Monotonically increasing counter, bumped once per setChannel/setUniverse/clear. */
		std::uint64_t getChangeVersion() const;

		/** Returns immediately if the current version != lastSeenVersion; otherwise
			blocks up to timeout. Always returns the version observed at return
			time — compare it to lastSeenVersion to know whether something
			actually changed. Lets a push-on-change driver wait efficiently
			instead of polling. */
		std::uint64_t waitForChange(std::uint64_t lastSeenVersion, std::chrono::milliseconds timeout) const;

	private:
		std::unordered_map<std::uint32_t, DmxUniverse> universes;
		std::uint64_t changeVersion = 0;

		mutable std::mutex lock; // locking is synchronization, not observable state; needed so isUniverse() etc. can lock from a const method
		mutable std::condition_variable wakeUp; // wakes waitForChange() when changeVersion is bumped
	};

}
