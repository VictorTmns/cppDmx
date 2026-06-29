#include "DmxEngine.h"

namespace cppDmx
{
	DmxEngine::DmxEngine()
		: refreshRate(0), isRunning(false)
	{
	}
	DmxEngine::~DmxEngine()
	{
		_ASSERT(!isRunning);
	}
	void DmxEngine::setOutputDriver(std::unique_ptr<IDmxDriver> newDriver)
	{
		driver = std::move(newDriver);
	}
	void DmxEngine::start()
	{
		_ASSERT(driver);


		if (refreshRate > 0)
		{
			{
				const std::lock_guard<std::mutex> sl(lock);
				if (isRunning)
					return;
				isRunning = true;
			}

			sendThread = std::thread(&DmxEngine::run, this);
		}

	}
	void DmxEngine::stop()
	{
		_ASSERT(driver);

		if (isRunning)
		{
			{
				const std::lock_guard<std::mutex> sl(lock);
				isRunning = false;
			}

			sendThread.join();
		}


	}

	void DmxEngine::setChannel(int universe, int channel, std::uint8_t value)
	{
		_ASSERT(channel <= 512);

		universes[universe].setChannel(channel, value);
	}

	void DmxEngine::setUniverse(int universe, const std::uint8_t* data, int numChannels)
	{
		_ASSERT(numChannels <= 512);

		universes[universe].setUniverse(data, numChannels);
	}

	void DmxEngine::clear()
	{
		for (auto& u : universes)
			u.second.setUniverse(nullptr, 0);
	}

	void DmxEngine::flushOnce()
	{
		std::lock_guard<std::mutex> sl(lock);
		flushLocked();
	}

	// ---------------------------------------------------------------------------------

	void DmxEngine::run()
	{
		std::unique_lock<std::mutex> sl(lock);

		while (isRunning)
		{
			// For a localhost test loop, sending under the lock is fine (it takes
			// microseconds). For production, snapshot each universe under the lock
			// and perform the socket write outside it.
			flushLocked();

			const auto period = std::chrono::duration<double>(1.0 / refreshRate);

			// wait_for releases the lock while waiting and re-acquires on wake; the
			// predicate makes stopSending() return immediately rather than sleeping
			// out the remainder of the frame.
			wakeUp.wait_for(sl, std::chrono::duration_cast<std::chrono::steady_clock::duration> (period),
				[this] { return !isRunning; });
		}
	}

	void DmxEngine::flushLocked()
	{
		_ASSERT(driver);

		for (auto& u : universes)
			driver->SendDmxData(u.first, u.second.getData());
	}
}