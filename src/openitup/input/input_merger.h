#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <openitup/input/input_driver.h>

namespace openitup {

enum class MergeStrategy {
    OR_MERGE,      // Combine all inputs (default)
    FIRST_WINS,    // US-INP-082: Priority-based, first driver with input wins
};

class InputMerger : public InputDriver {
public:
    InputMerger() = default;
    ~InputMerger() override = default;

    InputMerger(const InputMerger&) = delete;
    InputMerger& operator=(const InputMerger&) = delete;

    uint32_t poll_held() override;
    std::string device_name() const override;

    void add_driver(InputDriver* driver);
    void remove_driver(InputDriver* driver);

    // US-INP-082: Configure merge strategy and priority
    void set_merge_strategy(MergeStrategy strategy);
    MergeStrategy merge_strategy() const { return strategy_; }

    const std::vector<InputDriver*>& drivers() const { return drivers_; }

private:
    std::vector<InputDriver*> drivers_;
    MergeStrategy strategy_ = MergeStrategy::OR_MERGE;

    void sort_by_priority();
    int get_driver_priority(InputDriver* driver) const;
};

} // namespace openitup
