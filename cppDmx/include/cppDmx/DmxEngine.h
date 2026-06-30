#pragma once
#include <memory>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

#include <cppDmx/cppDmx_export.h>
#include <cppDmx/IDmxDriver.h>
#include <cppDmx/Containers/DmxUniverse.h>

namespace cppDmx
{
	class CPPDMX_API DmxEngine
	{
	public:
		DmxEngine();
		~DmxEngine();

		DmxEngine(const DmxEngine&) = delete;
		DmxEngine& operator= (const DmxEngine&) = delete;

		void setOutputDriver(std::unique_ptr<IDmxDriver> newDriver);

		/** Startup the driver */
		void start();
		void stop();

		/** Set a single channel. @p channel is 1..512 (DMX convention). */
		void setChannel(int universe, int channel, std::uint8_t value);

		/** Overwrite a whole universe at once (remaining channels are zeroed). */
		void setUniverse(int universe, const std::uint8_t* data, int numChannels);

		/** Flushes all the current data to the driver, this is done automatically at the refresh rate */
		void clear();

		/** Returns if @p universe has any existing data*/
		bool isUniverse(int universe) const;

		/** Gives all the data in @p universe, the data is always 512 bytes long*/
		void getUniverseData(int universe, std::uint8_t* data);

		/** Provides all the universes id's that have existing data*/
		void getUniverseNumbers(std::uint32_t* data, std::uint8_t length);


		/*
		** The refreshrate that the engine sends all the current data to the driver
		** If set to 0 or smaller the data won't be pushed
		*/
		void setRefreshRate(int rate) { refreshRate = rate; }

		/** Flushes all the current data to the driver once */
		void flushOnce();
	private:
		void run();
		void flushLocked();   // assumes 'lock' is already held

		std::unique_ptr<IDmxDriver> driver;

		std::unordered_map<std::uint32_t, DmxUniverse> universes;

		int refreshRate; // in Hz, 0 means no refresh

		bool isRunning;
		std::mutex lock;
		std::condition_variable wakeUp; // wakes the send thread to stop early
		std::thread sendThread;
	};

}
