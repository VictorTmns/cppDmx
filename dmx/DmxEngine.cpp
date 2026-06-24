#include "DmxEngine.h"

#include <algorithm>
#include <chrono>
#include <cstring>

DmxEngine::DmxEngine() = default;

DmxEngine::~DmxEngine()
{
    stopSending();

    const std::lock_guard<std::mutex> sl (lock);
    if (output != nullptr)
        output->close();
}

void DmxEngine::setOutput (std::unique_ptr<IDmxOutput> newOutput)
{
    if (newOutput != nullptr)
        newOutput->open();

    const std::lock_guard<std::mutex> sl (lock);
    if (output != nullptr)
        output->close();
    output = std::move (newOutput);
}

void DmxEngine::setChannel (int universe, int channel, std::uint8_t value)
{
    if (channel < 1 || channel > 512)
        return;

    const std::lock_guard<std::mutex> sl (lock);
    universes[universe].data[(size_t) (channel - 1)] = value;
}

void DmxEngine::setUniverse (int universe, const std::uint8_t* data, int numChannels)
{
    const std::lock_guard<std::mutex> sl (lock);
    auto& buf = universes[universe].data;
    buf.fill (0);
    std::memcpy (buf.data(), data, (size_t) std::clamp (numChannels, 0, 512));
}

void DmxEngine::clear()
{
    const std::lock_guard<std::mutex> sl (lock);
    for (auto& u : universes)
        u.second.data.fill (0);
}

void DmxEngine::start (double hz)
{
    {
        const std::lock_guard<std::mutex> sl (lock);
        refreshHz = std::clamp (hz, 1.0, 60.0);
        if (running)
            return;            // already sending; the rate change is picked up next frame
        running = true;
    }

    sendThread = std::thread (&DmxEngine::run, this);
}

void DmxEngine::stopSending()
{
    {
        const std::lock_guard<std::mutex> sl (lock);
        if (! running)
            return;
        running = false;
    }

    wakeUp.notify_all();       // cut the inter-frame wait short instead of sleeping it out

    if (sendThread.joinable())
        sendThread.join();
}

void DmxEngine::flushOnce()
{
    const std::lock_guard<std::mutex> sl (lock);
    flushLocked();
}

void DmxEngine::flushLocked()
{
    if (output == nullptr)
        return;

    for (auto& u : universes)
        output->sendUniverse (u.first, u.second.data.data(), 512);
}

void DmxEngine::run()
{
    std::unique_lock<std::mutex> sl (lock);

    while (running)
    {
        // For a localhost test loop, sending under the lock is fine (it takes
        // microseconds). For production, snapshot each universe under the lock
        // and perform the socket write outside it.
        flushLocked();

        const auto period = std::chrono::duration<double> (1.0 / refreshHz);

        // wait_for releases the lock while waiting and re-acquires on wake; the
        // predicate makes stopSending() return immediately rather than sleeping
        // out the remainder of the frame.
        wakeUp.wait_for (sl, std::chrono::duration_cast<std::chrono::steady_clock::duration> (period),
                         [this] { return ! running; });
    }
}
