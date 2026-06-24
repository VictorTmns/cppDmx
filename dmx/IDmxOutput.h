#pragma once

#include <cstdint>

/** Abstract DMX output — "the swap seam".

    The controller never talks to a socket or a USB device directly. It talks to
    this interface. Swapping software-test output (Art-Net to a visualiser on the
    same PC) for real hardware later is just a matter of handing the DmxEngine a
    different implementation of this interface. Nothing in the controller changes.
*/
class IDmxOutput
{
public:
    virtual ~IDmxOutput() = default;

    /** Acquire any resources (open a socket, open a serial port, ...).
        @returns true on success. */
    virtual bool open() = 0;

    /** Release resources. Safe to call even if open() failed or was never called. */
    virtual void close() = 0;

    /** Push one universe of DMX data to the output.
        @param universe     flat universe index (0-based, Art-Net convention)
        @param data         channel values; channel 1 == data[0]
        @param numChannels  number of valid channels in @p data (1..512) */
    virtual void sendUniverse (int universe, const std::uint8_t* data, int numChannels) = 0;

    /** Human-readable name, handy for logging or a UI dropdown. */
    virtual const char* name() const = 0;

protected:
    // C.67: a polymorphic base suppresses public copy/move to prevent slicing,
    // while still allowing derived classes to define their own copy semantics.
    IDmxOutput() = default;
    IDmxOutput (const IDmxOutput&) = default;
    IDmxOutput (IDmxOutput&&) = default;
    IDmxOutput& operator= (const IDmxOutput&) = default;
    IDmxOutput& operator= (IDmxOutput&&) = default;
};
