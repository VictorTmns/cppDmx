#pragma once
#include <cppDmx/DmxEngine.h>

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace cppDmx
{
	// Shared fixed-rate dispatch loop for protocols with a refresh rate 
	class PeriodicDriverPump
	{
	public:
		// the function to call for each universe's snapshot; called from the pump thread, so must be thread-safe
		using SendMethod = std::function<void(std::uint32_t universe, const std::array<std::uint8_t, 512>& data)>;

		PeriodicDriverPump(const DmxEngine& engine, SendMethod sink, int refreshRateHz);
		~PeriodicDriverPump(); // stop()s if still running

		PeriodicDriverPump(const PeriodicDriverPump&) = delete;
		PeriodicDriverPump& operator= (const PeriodicDriverPump&) = delete;

		void start();
		void stop();
		bool isRunning() const;

		void flushOnce(); // one synchronous dispatch

	private:
		void run();
		void dispatch();

		const DmxEngine& engine;
		SendMethod sink;
		int refreshRateHz;

		std::vector<DmxEngine::UniverseSnapshot> scratch; // shared by run() and flushOnce(), guarded by dispatchLock
		std::mutex dispatchLock;

		std::atomic<bool> running{ false };
		std::mutex m;  // exists because wait_for needs a unique_lock argument
		std::condition_variable wakeUp;
		std::thread thread;
	};
}
