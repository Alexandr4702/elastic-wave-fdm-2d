#include "elastic_wave/simulation.hpp"

#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace elastic_wave {
namespace {

using Json = nlohmann::json;

const std::unordered_set<std::string> configuration_fields = {
    "width", "height", "duration", "young_modulus", "poisson_ratio",
    "density", "dx", "dy", "dt", "initial_displacement", "damping"
};

double numeric_field(const Json& object, const char* name, double fallback) {
    const auto field = object.find(name);
    if (field == object.end()) return fallback;
    if (!field->is_number()) {
        throw std::invalid_argument("configuration field '" + std::string(name) +
                                    "' must be a number");
    }
    const double value = field->get<double>();
    if (!std::isfinite(value)) {
        throw std::invalid_argument("configuration field '" + std::string(name) +
                                    "' must be finite");
    }
    return value;
}

} // namespace

Configuration configuration_from_json(std::string_view source) {
    std::unordered_set<std::string> parsed_keys;
    const auto reject_duplicate_keys = [&](int depth, Json::parse_event_t event,
                                           Json& parsed) {
        if (depth == 1 && event == Json::parse_event_t::key) {
            const std::string key = parsed.get<std::string>();
            if (!parsed_keys.insert(key).second) {
                throw std::invalid_argument("duplicate configuration field '" + key + "'");
            }
        }
        return true;
    };

    Json object;
    try {
        object = Json::parse(source.begin(), source.end(), reject_duplicate_keys);
    } catch (const Json::exception& error) {
        throw std::invalid_argument("invalid configuration JSON: " + std::string(error.what()));
    }
    if (!object.is_object()) {
        throw std::invalid_argument("configuration JSON must contain an object");
    }
    for (const auto& [key, value] : object.items()) {
        (void)value;
        if (configuration_fields.count(key) == 0) {
            throw std::invalid_argument("unknown configuration field '" + key + "'");
        }
    }

    Configuration configuration;
    configuration.width = numeric_field(object, "width", configuration.width);
    configuration.height = numeric_field(object, "height", configuration.height);
    configuration.duration = numeric_field(object, "duration", configuration.duration);
    configuration.young_modulus =
        numeric_field(object, "young_modulus", configuration.young_modulus);
    configuration.poisson_ratio =
        numeric_field(object, "poisson_ratio", configuration.poisson_ratio);
    configuration.density = numeric_field(object, "density", configuration.density);
    configuration.dx = numeric_field(object, "dx", configuration.dx);
    configuration.dy = numeric_field(object, "dy", configuration.dy);
    configuration.dt = numeric_field(object, "dt", configuration.dt);
    configuration.initial_displacement =
        numeric_field(object, "initial_displacement", configuration.initial_displacement);
    configuration.damping = numeric_field(object, "damping", configuration.damping);
    return configuration;
}

Configuration load_configuration(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open configuration file: " + path);
    const std::string contents((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
    return configuration_from_json(contents);
}

} // namespace elastic_wave
