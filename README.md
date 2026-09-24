# cppDmx

A small, cross-platform C++17 DMX engine with a clean driver abstraction:
write channel values into one thread-safe `DmxEngine`, and send them out over
Art-Net, an Enttec USB Pro widget, a raw RS-485 line, or any transport you
implement yourself.

This project was made as a proof of concept for my graduation work. As so I was unable to verify these drivers with real hardware. If this projects intrests you don't hesitate to contact me. I can provide you with my paper and I would love to discuss this. 

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![CMake](https://img.shields.io/badge/CMake-%E2%89%A5%203.22-blue)
![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![License: MIT](https://img.shields.io/badge/License-MIT-green)

---

## Features

- **One data store, many outputs.** A controller only ever talks to `DmxEngine`
  and the `IDmxDriver` interface swapping transport is a new driver, not a
  rewrite.
- **Thread-safe engine.** `DmxEngine` is a pure, lock-guarded multi-universe
  data store with no timing or driver awareness of its own.
- **Included Drivers.** Art-Net (with ArtPoll node discovery), Enttec USB Pro,
  and raw Open DMX drivers ship in the box.
- **Truly cross-platform.** Builds on Windows (MSVC), Linux (GCC/Clang), and
  macOS (Apple Clang).  It is only the C++ standard library
  plus [standalone Asio](https://think-async.com/Asio/) (header-only, used
  privately; it never appears in the public headers).
- **Easy to consume.** FetchContent or `find_package`; link the single target
  `cppDmx::cppDmx`. Honours `BUILD_SHARED_LIBS` (static or shared).
- **Extensible.** Implement `IDmxDriver` for your own protocol, that is the
  whole point of the design.

## Drivers

| Driver | Transport | Header |
|--------|-----------|--------|
| **Art-Net** | UDP / Ethernet (ArtDmx, default `127.0.0.1:6454`; ArtPoll discovery) | `<cppDmx/Drivers/Art-Net/ArtNetDriver.h>` |
| **Enttec USB Pro** | USB serial widget (framed protocol) | `<cppDmx/Drivers/UsbPro/UsbProDriver.h>` |
| **Open DMX** | Raw RS-485 / FTDI serial (DMX-512 wire protocol: break + 250 kb slots) | `<cppDmx/Drivers/OpenDmx/OpenDmxDriver.h>` |
| **Your own** | Anything | implement `IDmxDriver` |

Every driver exposes the same five-method lifecycle: `Initialize`,
`Start(engine)`, `Stop()`, `Flush()`, `Shutdown` and decides internally how it
dispatches (a fixed-rate pump, a hardware-paced loop, ...). Callers never need to
know which strategy a given driver uses.

## Requirements

- CMake ≥ 3.22
- A C++17 compiler (MSVC, GCC, or Clang)

## Quick start

The CMake target is `cppDmx::cppDmx`. Public headers are included as
`<cppDmx/...>`.

```cpp
#include <cppDmx/DmxEngine.h>
#include <cppDmx/Drivers/Art-Net/ArtNetDriver.h>
#include <memory>

int main()
{
    cppDmx::DmxEngine engine;

    auto driver = std::make_unique<cppDmx::ArtNetDriver>("127.0.0.1", 40); // 40 Hz
    driver->Initialize();
    driver->Start(engine);

    engine.setChannel(0, 1, 255);       // universe 0, channel 1 -> full
    // ... update channels while it streams ...

    driver->Stop();
    driver->Shutdown();
}
```

Channels are `1..512` following the dmx conventions. Universe indices are flat and 0-based.


## Project layout

```
cppDmx/          the library (include/cppDmx public headers, src/ private)
app/             JUCE GUI controller (not part of the library)
demo/            command-line demos + fake nodes for manual bring-up
tests/           framework-free unit tests (run via ctest)
```

## License

Released under the [MIT License](LICENSE).
