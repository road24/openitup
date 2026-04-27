#pragma once

#include <nlohmann/json.hpp>

namespace openitup {

class JsonSerializable {
public:
    virtual ~JsonSerializable() = default;
    virtual void from_json(const nlohmann::json& j) = 0;
    virtual nlohmann::json to_json() const = 0;
};

} // namespace openitup
