#include "elastic_wave/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>

int main() {
    try {
        elastic_wave::Simulation simulation({});
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

