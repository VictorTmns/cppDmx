#pragma once
#include <cppDmx/cppDmx_export.h>
#include <cppDmx/IDmxDriver.h>

#include <cppDmx/Drivers/Art-Net/ArtNetDiscovery.h>

#include <array>
#include <memory>

namespace cppDmx
{
	class PeriodicDriverPump; // defined in the private PeriodicDriverPump.h/.cpp; pimpl'd here

	class CPPDMX_API ArtNetDriver : public IDmxDriver
	{
	public:
		using ErrorCallback = std::function<void(const std::error_code&)>;

		ArtNetDriver(const DiscoveredArtNetNode& node, int refreshRateHz = 40);
		ArtNetDriver(std::string host, int refreshRateHz = 40);

		// Out-of-line so the std::unique_ptr<AsioImpl>/std::unique_ptr<PeriodicDriverPump>
		// deleters see their complete types (both defined in the .cpp). Required
		// for the pimpl, and in particular for shared builds where CPPDMX_API
		// forces the compiler to instantiate this destructor at the consumer's
		// call site.
		~ArtNetDriver();

		std::error_code Initialize() override;
		std::error_code Shutdown() override; // also Stop()s defensively first

		void Start(const DmxEngine& engine) override;
		void Stop() override;
		void Flush() override;

		std::string GetDriverName() const override { return "Art-Net"; };

		bool IsRunning() const override;

		void SetErrorCallback(ErrorCallback callback) override { errorCallback = callback; }

	private:
		void sendOneUniverse(std::uint32_t universe, const std::array<std::uint8_t, 512>& data);

		std::string  targetHost;
		int targetPort = 6454; // default Art-Net port
		int refreshRateHz;
		std::uint8_t sequence = 1;   // 1..255; 0 would disable resequencing

		struct AsioImpl;
		std::unique_ptr<AsioImpl> impl;  // owns the Asio io_context + UDP socket
		std::unique_ptr<PeriodicDriverPump> pump; // declared after impl: destructs first

		ErrorCallback errorCallback;
	};
}
