#include <openitup/input/piuio_driver.h>

#include <spdlog/spdlog.h>

#include <openitup/input/pad_input.h>

namespace openitup {

// Platform-specific USB handle (stub implementation)
struct PiuioHandle {
    // In a real implementation, this would hold libusb context and device handle
    // For now, this is a placeholder for the stub
    bool is_mock = true;
};

// PIUIO USB VID/PID
constexpr uint16_t PIUIO_VENDOR_ID = 0x0547;
constexpr uint16_t PIUIO_PRODUCT_ID = 0x1002;

PiuioDriver::PiuioDriver()
    : version_(PiuioVersion::UNKNOWN),
      connected_(false),
      lamp_state_(0) {
    connected_ = initialize();
}

PiuioDriver::~PiuioDriver() {
    cleanup();
}

bool PiuioDriver::initialize() {
    // Stub implementation: attempt to detect PIUIO hardware
    // In a real implementation, this would:
    // 1. Initialize libusb
    // 2. Enumerate USB devices
    // 3. Find device with VID:PID = 0x0547:0x1002
    // 4. Open device and claim interface
    // 5. Perform version detection handshake

    handle_ = std::make_unique<PiuioHandle>();

    // For now, log that we're running in stub mode
    if (!is_available()) {
        spdlog::info("PIUIO hardware not detected, driver running in stub mode");
        version_ = PiuioVersion::UNKNOWN;
        return false;
    }

    // If we had real hardware, we would detect version here
    // For stub mode, assume V2 (standard 5-sensor configuration)
    version_ = PiuioVersion::V2;
    spdlog::info("PIUIO driver initialized: version={}",
                 version_ == PiuioVersion::V2 ? "v2" : "v1");

    return true;
}

void PiuioDriver::cleanup() {
    if (handle_) {
        // In a real implementation, this would:
        // 1. Release USB interface
        // 2. Close device
        // 3. Cleanup libusb context
        handle_.reset();
    }
}

uint32_t PiuioDriver::poll_held() {
    if (!connected_) {
        return 0;
    }

    return read_sensors();
}

std::string PiuioDriver::device_name() const {
    if (!connected_) {
        return "PIUIO (not connected)";
    }

    switch (version_) {
        case PiuioVersion::V1:
            return "PIUIO v1";
        case PiuioVersion::V2:
            return "PIUIO v2";
        default:
            return "PIUIO (unknown version)";
    }
}

uint32_t PiuioDriver::read_sensors() {
    // Stub implementation: return no sensors pressed
    // In a real implementation, this would:
    // 1. Perform USB bulk transfer to read sensor state
    // 2. Parse the response packet based on version (v1 vs v2)
    // 3. Map sensor bits to PadInput bitmask
    //
    // PIUIO sensor mapping (v2):
    // Sensors 0-4: P1 (down-left, up-left, center, up-right, down-right)
    // Sensors 5-9: P2 (down-left, up-left, center, up-right, down-right)

    // For stub mode, always return 0 (no sensors pressed)
    return 0;
}

void PiuioDriver::set_lamp(int column, bool state) {
    if (column < 0 || column >= 10) {
        return;
    }

    if (state) {
        lamp_state_ |= (1u << column);
    } else {
        lamp_state_ &= ~(1u << column);
    }
}

void PiuioDriver::update_lamps() {
    if (!connected_) {
        return;
    }

    write_lamp_state();
}

void PiuioDriver::write_lamp_state() {
    // Stub implementation: lamp state is tracked but not sent to hardware
    // In a real implementation, this would:
    // 1. Format lamp state into USB packet
    // 2. Perform USB bulk transfer to write lamp state
    // 3. Handle version-specific packet formats
    //
    // Note: On real hardware, lamp state is typically written in the same
    // USB packet as input reads for performance reasons
}

bool PiuioDriver::is_available() {
    // Stub implementation: check if PIUIO hardware is available
    // In a real implementation, this would:
    // 1. Initialize libusb
    // 2. Enumerate USB devices
    // 3. Check for VID:PID = 0x0547:0x1002
    // 4. Return true if found

    // For stub mode, always return false (hardware not available)
    return false;
}

} // namespace openitup
