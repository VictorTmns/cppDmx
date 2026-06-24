#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "IDmxOutput.h"

/** Owns the DMX value buffers for every universe and pushes them to the active
    IDmxOutput at a fixed refresh rate on its own thread.

    Your controller only ever calls setChannel() / setUniverse(). It has no idea
    whether the bytes end up in BlenderDMX, a hardware node, a USB dongle or an
    in-memory test buffer — that is decided entirely by which IDmxOutput you hand
    to setOutput().
*/
class DmxEngine
{
public:
    DmxEngine();
    ~DmxEngine();

    /** Swap the output backend. Opens the new output, then closes the old one.
        May be called before or while the engine is running. */
    void setOutput (std::unique_ptr<IDmxOutput> newOutput);

    /** Set a single channel. @p channel is 1..512 (DMX convention). */
    void setChannel (int universe, int channel, std::uint8_t value);

    /** Overwrite a whole universe at once (remaining channels are zeroed). */
    void setUniverse (int universe, const std::uint8_t* data, int numChannels);

    /** Zero every channel of every known universe. */
    void clear();

    /** Begin sending at @p refreshHz frames per second (DMX maxes out near 44). */
    void start (double refreshHz = 40.0);

    /** Stop the send thread. Does not clear values. */
    void stopSending();

    /** Send the current state exactly once, synchronously, on the calling thread.
        Used by the unit tests so they need no timing and no threads. Not normally
        called while the engine is running. */
    void flushOnce();

    DmxEngine (const DmxEngine&) = delete;
    DmxEngine& operator= (const DmxEngine&) = delete;

private:
    void run();
    void flushLocked();   // assumes 'lock' is already held

    struct UniverseBuffer { std::array<std::uint8_t, 512> data { {} }; };

    std::mutex                    lock;
    std::condition_variable       wakeUp;     // wakes the send thread to stop early
    std::map<int, UniverseBuffer> universes;  // guarded by lock
    std::unique_ptr<IDmxOutput>   output;     // guarded by lock
    double                        refreshHz = 40.0;   // guarded by lock
    bool                          running = false;    // guarded by lock
    std::thread                   sendThread;
};
