#include "PeriodicDriverPump.h"

#include <cassert>
#include <chrono>

namespace cppDmx
{
	PeriodicDriverPump::PeriodicDriverPump(const DmxEngine& engine, SendMethod sink, int refreshRateHz)
		: engine(engine), sink(std::move(sink)), refreshRateHz(refreshRateHz)
	{
		assert(refreshRateHz > 0);
	}

	PeriodicDriverPump::~PeriodicDriverPump()
	{
		stop();
	}

	void PeriodicDriverPump::start()
	{
		bool expected = false;
		if (!running.compare_exchange_strong(expected, true))
			return; // already running

		thread = std::thread(&PeriodicDriverPump::run, this);
	}

	void PeriodicDriverPump::stop()
	{
		if (!running.exchange(false))
			return; // wasn't running

		wakeUp.notify_all();
		if (thread.joinable())
			thread.join();
	}

	bool PeriodicDriverPump::isRunning() const
	{
		return running.load();
	}

	void PeriodicDriverPump::dispatch()
	{
		const std::lock_guard<std::mutex> dl(dispatchLock);
		engine.snapshotAll(scratch);
		for (auto& s : scratch)
			sink(s.universe, s.data);
	}

	void PeriodicDriverPump::flushOnce()
	{
		dispatch();
	}

	void PeriodicDriverPump::run()
	{
		std::unique_lock<std::mutex> sl(m); // held only to satisfy wait_for's signature

		while (running.load())
		{
			dispatch();

			const auto period = std::chrono::duration<double>(1.0 / refreshRateHz);
			wakeUp.wait_for(sl, std::chrono::duration_cast<std::chrono::steady_clock::duration>(period),
				[this] { return !running.load(); });
		}
	}
}
