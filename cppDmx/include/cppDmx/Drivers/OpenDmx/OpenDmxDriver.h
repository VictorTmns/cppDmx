#pragma once
#include <cppDmx/cppDmx_export.h>
#include <cppDmx/IDmxDriver.h>

#include <array>
#include <memory>
#include <vector>

namespace cppDmx
{
	class PeriodicDriverPump; // private (PeriodicDriverPump.h/.cpp); pimpl'd here

	// Raw DMX-512 over a generic RS-485 / FTDI serial adapter ("Open DMX" style).
	//
	// This driver speaks the DMX-512 wire protocol itself — it sets the port to 250 kbaud 8N2 and,
	// every frame, asserts a line BREAK followed by a mark-after-break before
	// clocking out the start code and 512 slots. One physical line carries exactly
	// one universe.
	class CPPDMX_API OpenDmxDriver : public IDmxDriver
	{
	public:
		using ErrorCallback = std::function<void(const std::error_code&)>;

		OpenDmxDriver(std::string portName, std::uint32_t universe, int refreshRateHz = 30);
		~OpenDmxDriver();

		std::error_code Initialize() override; // opens + configures the serial port
		std::error_code Shutdown() override;   // Stop()s first, then closes

		void Start(const DmxEngine& engine) override;
		void Stop() override;
		void Flush() override;

		std::string GetDriverName() const override { return "Open DMX (raw RS-485)"; }

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

	/** Builds a raw DMX-512 frame payload: [start code][512 slots]. This is the
		exact byte sequence clocked onto the wire after the BREAK / mark-after-break
		(which is a line-level signal, not part of the byte stream). Exposed as a
		pure, testable seam, mirroring encodeUsbProSendDmx. */
	CPPDMX_API std::vector<std::uint8_t> encodeOpenDmxFrame(std::uint8_t startCode, const std::array<std::uint8_t, 512>& data);
}
