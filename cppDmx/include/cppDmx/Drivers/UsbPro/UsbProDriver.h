#pragma once
#include <cppDmx/cppDmx_export.h>
#include <cppDmx/IDmxDriver.h>

#include <array>
#include <memory>
#include <vector>

namespace cppDmx
{
	class PeriodicDriverPump; // defined in the private PeriodicDriverPump.h/.cpp; pimpl'd here

	class CPPDMX_API UsbProDriver : public IDmxDriver
	{
	public:
		using ErrorCallback = std::function<void(const std::error_code&)>;

		// portName: e.g. "COM3" on Windows (Asio prepends the \\.\  device-namespace
		// prefix internally — pass the plain name)
		// universe: this widget talks to exactly one DMX universe
		// refreshRateHz: conservative default — the widget's actual achievable rate
		// over its USB host link is unconfirmed in this environment (no hardware);
		UsbProDriver(std::string portName, std::uint32_t universe, int refreshRateHz = 25);
		~UsbProDriver();

		std::error_code Initialize() override; // opens the serial port
		std::error_code Shutdown() override;   // Stop()s first, then closes

		void Start(const DmxEngine& engine) override;
		void Stop() override;
		void Flush() override;

		std::string GetDriverName() const override { return "Enttec USB Pro"; }

		bool IsRunning() const override;

		void SetErrorCallback(ErrorCallback callback) override { errorCallback = callback; }

	private:
		void sendOneUniverse(std::uint32_t u, const std::array<std::uint8_t, 512>& data); // no-ops if u != universe

		std::string   portName;
		std::uint32_t universe;
		int refreshRateHz;

		struct AsioImpl;
		std::unique_ptr<AsioImpl> impl;             // owns the Asio io_context + serial_port
		std::unique_ptr<PeriodicDriverPump> pump;   // declared after impl: destructs first

		ErrorCallback errorCallback;
	};

	/** Builds a raw Enttec "Output Only Send DMX Packet" */
	CPPDMX_API std::vector<std::uint8_t> encodeUsbProSendDmx(std::uint8_t startCode, const std::array<std::uint8_t, 512>& data);
}
