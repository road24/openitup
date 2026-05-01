#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <openitup/input/input_driver.h>

namespace openitup {

// Forward declaration for platform-specific USB handle
struct PiuioHandle;

enum class PiuioVersion {
    UNKNOWN,
    V1,  // 4-sensor per panel
    V2   // 5-sensor per panel (standard PIU configuration)
};

class PiuioDriver : public InputDriver {
public:
    PiuioDriver();
    ~PiuioDriver() override;

    PiuioDriver(const PiuioDriver&) = delete;
    PiuioDriver& operator=(const PiuioDriver&) = delete;

    uint32_t poll_held() override;
    std::string device_name() const override;

    // Version detection
    PiuioVersion version() const { return version_; }

    // Lamp control (for US-INP-051)
    void set_lamp(int column, bool state);
    void update_lamps();

    // Hardware detection
    static bool is_available();

private:
    bool initialize();
    void cleanup();
    uint32_t read_sensors();
    void write_lamp_state();

    std::unique_ptr<PiuioHandle> handle_;
    PiuioVersion version_;
    bool connected_;

    // Lamp state (10 lamps: 5 per player)
    uint32_t lamp_state_;
};

} // namespace openitup
