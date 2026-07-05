#include "elastic_wave/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    try {
        const std::string configuration_path = argc > 1 ? argv[1] : "environment.json";
        elastic_wave::Simulation simulation(elastic_wave::load_configuration(configuration_path));
        double final_peak = 0.0;
        simulation.run([&](const elastic_wave::StateView& state) {
            if (state.step != simulation.steps()) return;
            for (std::size_t i = 0; i < state.horizontal.values().size(); ++i) {
                final_peak = std::max(final_peak, std::hypot(state.horizontal.values()[i],
                                                              state.vertical.values()[i]));
            }
        });
        std::cout << "grid: " << simulation.columns() << 'x' << simulation.rows()
                  << ", steps: " << simulation.steps() << '\n'
                  << "final peak displacement: " << final_peak << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "elastic-wave: " << error.what() << '\n';
        return 1;
    }
}
