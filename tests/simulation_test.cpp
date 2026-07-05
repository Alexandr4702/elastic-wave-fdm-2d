#include "elastic_wave/simulation.hpp"

#include <cmath>
#include <stdexcept>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
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
    simulation.run([&](const elastic_wave::StateView& state) {
        ++observations;
        for (double value : state.horizontal.values()) require(std::isfinite(value), "non-finite u");
        for (double value : state.vertical.values()) require(std::isfinite(value), "non-finite v");
        for (std::size_t x = 0; x < state.horizontal.width(); ++x) {
            require(state.horizontal(x, 0) == 0.0, "lower boundary changed");
            require(state.horizontal(x, state.horizontal.height() - 1) == 0.0,
                    "upper boundary changed");
        }
    });
    require(observations == simulation.steps() + 1, "unexpected observation count");

    config.poisson_ratio = 0.5;
    try {
        elastic_wave::Simulation invalid(config);
        require(false, "invalid Poisson ratio accepted");
    } catch (const std::invalid_argument&) {
    }
}
