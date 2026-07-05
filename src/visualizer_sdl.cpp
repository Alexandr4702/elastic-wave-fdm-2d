#include "elastic_wave/simulation.hpp"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Click {
    int x;
    int y;
};

[[noreturn]] void sdl_error(const char* operation) {
    throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}

std::uint32_t heat_colour(double value) {
    value = std::clamp(value, 0.0, 1.0);
    const double scaled = value * 4.0;
    const int segment = std::min(3, static_cast<int>(scaled));
    const double fraction = scaled - segment;

    int red = 0;
    int green = 0;
    int blue = 0;
    switch (segment) {
    case 0:
        blue = static_cast<int>(255.0 * fraction);
        break;
    case 1:
        green = static_cast<int>(255.0 * fraction);
        blue = 255;
        break;
    case 2:
        red = static_cast<int>(255.0 * fraction);
        green = 255;
        blue = static_cast<int>(255.0 * (1.0 - fraction));
        break;
    default:
        red = 255;
        green = static_cast<int>(255.0 * (1.0 - fraction));
        break;
    }
    return 0xff000000u | static_cast<std::uint32_t>(red << 16) |
           static_cast<std::uint32_t>(green << 8) | static_cast<std::uint32_t>(blue);
}

void draw(SDL_Renderer* renderer, SDL_Texture* texture,
          const elastic_wave::StateView& state, double scale,
          std::vector<std::uint32_t>& pixels) {
    const std::size_t width = state.horizontal.width();
    const std::size_t height = state.horizontal.height();
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < width; ++x) {
            const double u = state.horizontal(x, y);
            const double v = state.vertical(x, y);
            pixels[(height - y - 1) * width + x] = heat_colour(std::hypot(u, v) / scale);
        }
    }
    if (SDL_UpdateTexture(texture, nullptr, pixels.data(),
                          static_cast<int>(width * sizeof(std::uint32_t))) != 0) {
        sdl_error("cannot update texture");
    }
    SDL_RenderClear(renderer);
    if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0) {
        sdl_error("cannot render texture");
    }
    SDL_RenderPresent(renderer);
}

} // namespace

int main(int argc, char** argv) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    try {
        SDL_SetMainReady();
        if (SDL_Init(SDL_INIT_VIDEO) != 0) sdl_error("cannot initialize SDL");

        const std::string configuration_path = argc > 1 ? argv[1] : "environment.json";
        elastic_wave::Simulation simulation(elastic_wave::load_configuration(configuration_path));
        window = SDL_CreateWindow("Elastic wave FDM 2D — click to disturb",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  850, 850, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        if (!window) sdl_error("cannot create window");
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer) sdl_error("cannot create renderer");
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    static_cast<int>(simulation.columns()),
                                    static_cast<int>(simulation.rows()));
        if (!texture) sdl_error("cannot create texture");

        bool running = true;
        std::vector<Click> pending_clicks;
        std::vector<std::uint32_t> pixels(simulation.columns() * simulation.rows());
        const double scale = simulation.configuration().initial_displacement;

        simulation.run(
            [&](const elastic_wave::StateView& state) {
                SDL_Event event{};
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) running = false;
                    if (event.type == SDL_MOUSEBUTTONDOWN &&
                        event.button.button == SDL_BUTTON_LEFT) {
                        pending_clicks.push_back({event.button.x, event.button.y});
                    }
                }
                if (!running || state.step % 10 != 0) return;
                draw(renderer, texture, state, scale, pixels);
                const std::string title = "Elastic wave FDM 2D — click to disturb — step " +
                    std::to_string(state.step) + ", t = " + std::to_string(state.time) + " s";
                SDL_SetWindowTitle(window, title.c_str());
                SDL_Delay(10);
            },
            [&](elastic_wave::Field& current_u, elastic_wave::Field& previous_u,
                elastic_wave::Field&, elastic_wave::Field&) {
                int window_width = 0;
                int window_height = 0;
                SDL_GetWindowSize(window, &window_width, &window_height);
                if (window_width <= 0 || window_height <= 0) {
                    pending_clicks.clear();
                    return;
                }
                for (const Click click : std::exchange(pending_clicks, {})) {
                    const double grid_x = static_cast<double>(click.x) *
                        static_cast<double>(current_u.width() - 1) / window_width;
                    const double grid_y = static_cast<double>(window_height - 1 - click.y) *
                        static_cast<double>(current_u.height() - 1) / window_height;
                    const double amplitude = simulation.configuration().initial_displacement;
                    constexpr double radius = 2.5;
                    for (std::size_t y = 1; y + 1 < current_u.height(); ++y) {
                        for (std::size_t x = 1; x + 1 < current_u.width(); ++x) {
                            const double dx = static_cast<double>(x) - grid_x;
                            const double dy = static_cast<double>(y) - grid_y;
                            const double displacement = amplitude *
                                std::exp(-(dx * dx + dy * dy) / (2.0 * radius * radius));
                            current_u(x, y) += displacement;
                            previous_u(x, y) += displacement;
                        }
                    }
                }
            },
            [&] { return running; });

        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "elastic-wave-visualizer: " << error.what() << '\n';
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
}
