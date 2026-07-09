#pragma once
#include <cppDmx/cppDmx_export.h>

#include <string>
#include <functional>
#include <system_error>

namespace cppDmx
{
	class DmxEngine;

	// Public API: consumers implement this interface to add their own output
	// protocol (the "swap seam"). Exported so a consumer's derived driver has a
	// proper dll-interface base in shared builds.
	//
	// The lifecycle is fully generic across protocols: Initialize/Shutdown open
	// and close whatever transport this driver uses; Start/Stop/Flush drive
	// engine's data to it however this driver likes internally (a constant-rate
	// pump, push-on-change, a hardware-paced serial loop, ...). Consumers never
	// need to know which strategy a given driver uses.
	class CPPDMX_API IDmxDriver
	{
	public:
		using ErrorCallback = std::function<void(const std::error_code&)>;

		IDmxDriver() = default;
		virtual ~IDmxDriver() = default;

		IDmxDriver(const IDmxDriver&) = delete;
		IDmxDriver& operator= (const IDmxDriver&) = delete;

		/** Initialize the driver. Returns a default-constructed (falsy) std::error_code on success. */
		virtual std::error_code Initialize() = 0;

		/** Shutdown the driver. Always leaves it fully stopped, regardless of
			prior Start()/Stop() state. Returns a default-constructed (falsy)
			std::error_code on success. */
		virtual std::error_code Shutdown() = 0;

		/** Begin continuously dispatching engine's data via whatever internal
			mechanism this driver uses. @p engine must outlive this call through
			to the matching Stop(). */
		virtual void Start(const DmxEngine& engine) = 0;

		/** Stop dispatching. Idempotent; safe even if never started. */
		virtual void Stop() = 0;

		/** Synchronously push whatever's currently in the engine passed to the
			most recent Start(), once. Safe to call at any time, including while
			still running, or before any Start() (no-op). */
		virtual void Flush() = 0;

		/** The driver name is only used to identify options*/
		virtual std::string GetDriverName() const = 0;

		virtual bool IsRunning() const = 0;

		virtual void SetErrorCallback(ErrorCallback callback) = 0;
	};
}
