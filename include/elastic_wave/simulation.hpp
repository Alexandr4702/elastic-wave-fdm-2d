#pragma once

#include <cstddef>
#include <functional>
#include <vector>

namespace elastic_wave {

struct Configuration {
    double width = 0.04;
    double height = 0.04;
    double duration = 2.0;
    double young_modulus = 20.0;
    double poisson_ratio = 0.33;
    double density = 7640.0;
    double dx = 0.0005;
    double dy = 0.0005;
    double dt = 0.0002;
    double initial_displacement = 0.02;
};

class Field {
public:
    Field(std::size_t width, std::size_t height);
    double& operator()(std::size_t x, std::size_t y);
    double operator()(std::size_t x, std::size_t y) const;
    std::size_t width() const noexcept;
    std::size_t height() const noexcept;
    std::vector<double>& values() noexcept;
    const std::vector<double>& values() const noexcept;

private:
    std::size_t width_;
    std::size_t height_;
    std::vector<double> values_;
};

struct StateView {
    std::size_t step;
    double time;
    const Field& horizontal;
    const Field& vertical;
};

using Observer = std::function<void(const StateView&)>;

class Simulation {
public:
    explicit Simulation(Configuration configuration);
    void run(const Observer& observer = {});
    const Configuration& configuration() const noexcept;
    std::size_t columns() const noexcept;
    std::size_t rows() const noexcept;
    std::size_t steps() const noexcept;

private:
    Configuration configuration_;
    std::size_t columns_;
    std::size_t rows_;
    std::size_t steps_;
};

} // namespace elastic_wave

