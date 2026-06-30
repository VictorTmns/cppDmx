#pragma once
#include <cppDmx/cppDmx_export.h>

#include <string>
#include <array>
#include <optional>
#include <functional>
#include <system_error>

namespace cppDmx
{
	// Public API: consumers implement this interface to add their own output
	// protocol (the "swap seam"). Exported so a consumer's derived driver has a
	// proper dll-interface base in shared builds.
	class CPPDMX_API IDmxDriver
	{
	public:
		using ErrorCallback = std::function<void(const std::error_code&)>;

		IDmxDriver() = default;
		virtual ~IDmxDriver() = default;

		IDmxDriver(const IDmxDriver&) = delete;
		IDmxDriver& operator= (const IDmxDriver&) = delete;

		/** Initialize the driver*/
		virtual std::optional<std::error_code> Initialize() = 0;

		/** Shutdown the driver*/
		virtual std::optional<std::error_code> Shutdown() = 0;

		/**
		* Send DMX data, called by the engine
		* Data will be pushed through this, the driver is responsible for sending it to the correct destination
		* RefreshRate determines how often this is called
		*/
		virtual void SendDmxData(std::uint32_t universe, const std::array<std::uint8_t, 512>& Data) = 0;

		/** The driver name is only used to identify options*/
		virtual std::string GetDriverName() const = 0;

		virtual bool IsRunning() const = 0;

		virtual void SetErrorCallback(ErrorCallback callback) = 0;
	};
}
