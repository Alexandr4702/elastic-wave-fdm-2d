#include "elastic_wave/simulation.hpp"

#include <cmath>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    const auto parsed = elastic_wave::configuration_from_json(
        R"({"width": 2.5, "density": 10, "dt": 0.01})");
    require(parsed.width == 2.5, "JSON width was not loaded");
    require(parsed.density == 10.0, "JSON density was not loaded");
    require(parsed.dt == 0.01, "JSON time step was not loaded");
    require(parsed.height == elastic_wave::Configuration{}.height,
            "missing JSON field did not keep its default");

    elastic_wave::Configuration config;
    config.width = 1.0;
    config.height = 1.0;
    config.duration = 0.03;
    config.dx = 0.25;
    config.dy = 0.25;
    config.dt = 0.01;
    config.young_modulus = 1.0;
    config.density = 1.0;

    elastic_wave::Simulation simulation(config);
    std::size_t observations = 0;
    std::size_t interactions = 0;
    simulation.run(
        [&](const elastic_wave::StateView& state) {
            ++observations;
            for (double value : state.horizontal.values()) require(std::isfinite(value), "non-finite u");
            for (double value : state.vertical.values()) require(std::isfinite(value), "non-finite v");
            for (std::size_t x = 0; x < state.horizontal.width(); ++x) {
                require(state.horizontal(x, 0) == 0.0, "lower boundary changed");
                require(state.horizontal(x, state.horizontal.height() - 1) == 0.0,
                        "upper boundary changed");
            }
        },
        [&](elastic_wave::Field& current_u, elastic_wave::Field& previous_u,
            elastic_wave::Field&, elastic_wave::Field&) {
            ++interactions;
            if (interactions == 1) {
                current_u(1, 1) += 0.01;
                previous_u(1, 1) += 0.01;
            }
        });
    require(observations == simulation.steps() + 1, "unexpected observation count");
    require(interactions == simulation.steps(), "unexpected interaction count");

    config.poisson_ratio = 0.5;
    try {
        elastic_wave::Simulation invalid(config);
        require(false, "invalid Poisson ratio accepted");
    } catch (const std::invalid_argument&) {
    }

    config.poisson_ratio = 0.33;
    config.damping = -1.0;
    try {
        elastic_wave::Simulation invalid(config);
        require(false, "negative damping accepted");
    } catch (const std::invalid_argument&) {
    }
}
