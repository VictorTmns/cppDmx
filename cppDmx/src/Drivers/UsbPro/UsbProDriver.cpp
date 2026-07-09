#include <cppDmx/Drivers/UsbPro/UsbProDriver.h>

#include "PeriodicDriverPump.h"

#include <asio.hpp>

#include <chrono>

namespace cppDmx
{
	namespace
	{
		// Writes `frame` to `port`, bounded by `timeout`. A stalled peer (e.g. a
		// com0com virtual pair with nothing draining the other end, or in
		// principle an unresponsive real widget) can otherwise block a plain
		// synchronous write indefinitely — which would hang whichever thread
		// calls this: the periodic pump's own thread, or the caller of Flush()
		// (potentially a GUI's message thread). Returns the resulting error;
		// asio::error::operation_aborted if the write was abandoned on timeout.
		asio::error_code writeWithTimeout(asio::io_context& io, asio::serial_port& port,
			const std::vector<std::uint8_t>& frame, std::chrono::milliseconds timeout)
		{
			io.restart();

			asio::error_code result;
			asio::steady_timer timer(io);

			asio::async_write(port, asio::buffer(frame), [&](const asio::error_code& ec, std::size_t)
			{
				result = ec;
				timer.cancel();   // the write finished first: stop waiting on the timer
			});

			timer.expires_after(timeout);
			timer.async_wait([&](const asio::error_code& ec)
			{
				if (!ec)   // fired for real (not cancelled): the write is stuck, abandon it
				{
					asio::error_code ignored;
					port.cancel(ignored);
				}
			});

			io.run();
			return result;
		}
	}

	struct UsbProDriver::AsioImpl
	{
		asio::io_context     io;
		asio::serial_port    port{ io };
		bool                 opened = false;
	};

	UsbProDriver::UsbProDriver(std::string portName, std::uint32_t universe, int refreshRateHz)
		: portName(std::move(portName))
		, universe(universe)
		, refreshRateHz(refreshRateHz)
		, impl(std::make_unique<AsioImpl>())
	{
	}

	UsbProDriver::~UsbProDriver() = default;

	std::error_code UsbProDriver::Initialize()
	{
		asio::error_code ec;

		impl->port.open(portName, ec);
		if (ec) 
			return ec;

		impl->port.set_option(asio::serial_port_base::baud_rate(57600), ec);
		if (ec) 
			return ec;
		impl->port.set_option(asio::serial_port_base::character_size(8), ec);
		if (ec) 
			return ec;
		impl->port.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none), ec);
		if (ec) 
			return ec;
		impl->port.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::two), ec);
		if (ec) 
			return ec;
		impl->port.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none), ec);
		// (no early-returning here: flow control can fail to apply without being a real problem)

		impl->opened = true;
		return {};
	}

	std::error_code UsbProDriver::Shutdown()
	{
		Stop();

		if (!impl->opened)
			return {};

		asio::error_code ec;
		impl->port.close(ec);
		if (ec) 
			return ec;

		impl->opened = false;
		return {};
	}

	void UsbProDriver::Start(const DmxEngine& engine)
	{
		pump = std::make_unique<PeriodicDriverPump>(engine,
			[this](std::uint32_t u, const std::array<std::uint8_t, 512>& data) { sendOneUniverse(u, data); },
			refreshRateHz);
		pump->start();
	}

	void UsbProDriver::Stop() { if (pump) pump->stop(); }

	void UsbProDriver::Flush() { if (pump) pump->flushOnce(); }

	void UsbProDriver::sendOneUniverse(std::uint32_t u, const std::array<std::uint8_t, 512>& data)
	{
		if (u != universe || !impl->opened)
			return; // check universe because only one universe is possible

		const auto frame = encodeUsbProSendDmx(0x00, data); // DMX start code 0x00 (standard "dimmer" data)

		const auto ec = writeWithTimeout(impl->io, impl->port, frame, std::chrono::milliseconds(200));

		if (ec && errorCallback)
			errorCallback(ec);
	}

	bool UsbProDriver::IsRunning() const { return impl->opened; }

	std::vector<std::uint8_t> encodeUsbProSendDmx(std::uint8_t startCode, const std::array<std::uint8_t, 512>& data)
	{
		const std::uint16_t payloadLen = 1 + static_cast<std::uint16_t>(data.size()); // start code + channels

		std::vector<std::uint8_t> frame;
		frame.reserve(4 + payloadLen + 1);

		frame.push_back(0x7E); // start delimiter
		frame.push_back(6); // label 6: Output Only Send DMX Packet
		frame.push_back(static_cast<std::uint8_t>(payloadLen & 0xFF)); // length, low byte
		frame.push_back(static_cast<std::uint8_t>((payloadLen >> 8) & 0xFF)); // length, high byte
		frame.push_back(startCode);
		frame.insert(frame.end(), data.begin(), data.end());
		frame.push_back(0xE7); // end delimiter

		return frame;
	}
}
