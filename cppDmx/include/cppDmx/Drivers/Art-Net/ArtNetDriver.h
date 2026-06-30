#pragma once
#include <cppDmx/cppDmx_export.h>
#include <cppDmx/IDmxDriver.h>

#include <cppDmx/Drivers/Art-Net/ArtNetDiscovery.h>

#include <memory>

namespace cppDmx
{
	class CPPDMX_API ArtNetDriver : public IDmxDriver
	{
	public:
		using ErrorCallback = std::function<void(const std::error_code&)>;

		ArtNetDriver(const DiscoveredArtNetNode& node);
		ArtNetDriver(std::string host);

		// Out-of-line so the std::unique_ptr<AsioImpl> deleter sees a complete
		// AsioImpl (defined in the .cpp). Required for the pimpl, and in
		// particular for shared builds where CPPDMX_API forces the compiler to
		// instantiate this destructor at the consumer's call site.
		~ArtNetDriver();

		std::optional<std::error_code> Initialize() override;
		std::optional<std::error_code> Shutdown() override;

		void SendDmxData(std::uint32_t universe, const std::array<std::uint8_t, 512>& Data) override;

		std::string GetDriverName() const override { return "Art-Net"; };

		bool IsRunning() const override;

		void SetErrorCallback(ErrorCallback callback) override { errorCallback = callback; }

	private:
		std::string  targetHost;
		int targetPort = 6454; // default Art-Net port
		std::uint8_t sequence = 1;   // 1..255; 0 would disable resequencing

		struct AsioImpl;
		std::unique_ptr<AsioImpl> impl;  // owns the Asio io_context + UDP socket

		ErrorCallback errorCallback;
	};
}
