#include "elastic_wave/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

#if defined(_MSC_VER)
#define ELASTIC_WAVE_RESTRICT __restrict
#define ELASTIC_WAVE_VECTORIZE __pragma(loop(ivdep))
#elif defined(__GNUC__) || defined(__clang__)
#define ELASTIC_WAVE_RESTRICT __restrict__
#define ELASTIC_WAVE_PRAGMA_IMPL(value) _Pragma(#value)
#define ELASTIC_WAVE_PRAGMA(value) ELASTIC_WAVE_PRAGMA_IMPL(value)
#define ELASTIC_WAVE_VECTORIZE ELASTIC_WAVE_PRAGMA(GCC ivdep)
#else
#define ELASTIC_WAVE_RESTRICT
#define ELASTIC_WAVE_VECTORIZE
#endif

namespace elastic_wave {
namespace {

std::size_t intervals(double extent, double spacing, const char* name) {
    if (!std::isfinite(extent) || !std::isfinite(spacing) || extent <= 0.0 || spacing <= 0.0) {
        throw std::invalid_argument(std::string{name} + " and its spacing must be finite and positive");
    }
    const auto count = static_cast<std::size_t>(std::llround(extent / spacing));
    if (count < 2) throw std::invalid_argument(std::string{name} + " requires at least two intervals");
    return count;
}

void clear_boundaries(Field& field) {
    const auto last_x = field.width() - 1;
    const auto last_y = field.height() - 1;
    for (std::size_t x = 0; x < field.width(); ++x) {
        field(x, 0) = 0.0;
        field(x, last_y) = 0.0;
    }
    for (std::size_t y = 0; y < field.height(); ++y) {
        field(0, y) = 0.0;
        field(last_x, y) = 0.0;
    }
}

} // namespace

Field::Field(std::size_t width, std::size_t height)
    : width_(width), height_(height), values_(width * height, 0.0) {
    if (width == 0 || height == 0) throw std::invalid_argument("field dimensions must be positive");
}

double& Field::operator()(std::size_t x, std::size_t y) { return values_.at(y * width_ + x); }
double Field::operator()(std::size_t x, std::size_t y) const { return values_.at(y * width_ + x); }
std::size_t Field::width() const noexcept { return width_; }
std::size_t Field::height() const noexcept { return height_; }
std::vector<double>& Field::values() noexcept { return values_; }
const std::vector<double>& Field::values() const noexcept { return values_; }

Simulation::Simulation(Configuration configuration)
    : configuration_(configuration),
      columns_(intervals(configuration.width, configuration.dx, "width") + 1),
      rows_(intervals(configuration.height, configuration.dy, "height") + 1),
      steps_(intervals(configuration.duration, configuration.dt, "duration")) {
    if (!std::isfinite(configuration_.density) || configuration_.density <= 0.0 ||
        !std::isfinite(configuration_.young_modulus) || configuration_.young_modulus <= 0.0) {
        throw std::invalid_argument("density and Young's modulus must be finite and positive");
    }
    if (!std::isfinite(configuration_.poisson_ratio) || configuration_.poisson_ratio <= -1.0 ||
        configuration_.poisson_ratio >= 0.5) {
        throw std::invalid_argument("Poisson ratio must be between -1 and 0.5");
    }
    if (!std::isfinite(configuration_.damping) || configuration_.damping < 0.0) {
        throw std::invalid_argument("damping must be finite and non-negative");
    }
}

void Simulation::run(const Observer& observer, const Interactor& interactor,
                     const RunCondition& continue_running) {
    Field previous_u(columns_, rows_), current_u(columns_, rows_), next_u(columns_, rows_);
    Field previous_v(columns_, rows_), current_v(columns_, rows_), next_v(columns_, rows_);
    current_u(columns_ / 2, rows_ / 2) = configuration_.initial_displacement;
    previous_u = current_u; // zero initial velocity

    const double e = configuration_.young_modulus;
    const double nu = configuration_.poisson_ratio;
    const double lambda = nu * e / ((1.0 + nu) * (1.0 - 2.0 * nu));
    const double mu = e / (2.0 * (1.0 + nu));
    const double factor = configuration_.dt * configuration_.dt / configuration_.density;
    const double au = (lambda + 2.0 * mu) * factor / (configuration_.dx * configuration_.dx);
    const double bu = mu * factor / (configuration_.dy * configuration_.dy);
    const double av = (lambda + 2.0 * mu) * factor / (configuration_.dy * configuration_.dy);
    const double bv = mu * factor / (configuration_.dx * configuration_.dx);
    const double coupling = (lambda + mu) * factor /
                            (4.0 * configuration_.dx * configuration_.dy);
    const double damping_half_step = 0.5 * configuration_.damping * configuration_.dt;
    const double damping_denominator = 1.0 / (1.0 + damping_half_step);
    const double previous_weight = 1.0 - damping_half_step;
    const double pressure_speed = std::sqrt((lambda + 2.0 * mu) / configuration_.density);
    const double courant = pressure_speed * configuration_.dt *
                           std::sqrt(1.0 / (configuration_.dx * configuration_.dx) +
                                     1.0 / (configuration_.dy * configuration_.dy));
    if (courant > 1.0) {
        throw std::invalid_argument("unstable discretization: reduce dt or increase grid spacing");
    }

    if (observer) observer({0, 0.0, current_u, current_v});
    for (std::size_t step = 1;
         continue_running ? continue_running() : step <= steps_;
         ++step) {
        if (interactor) interactor(current_u, previous_u, current_v, previous_v);
        std::fill(next_u.values().begin(), next_u.values().end(), 0.0);
        std::fill(next_v.values().begin(), next_v.values().end(), 0.0);

        const double* ELASTIC_WAVE_RESTRICT previous_u_data = previous_u.values().data();
        const double* ELASTIC_WAVE_RESTRICT current_u_data = current_u.values().data();
        double* ELASTIC_WAVE_RESTRICT next_u_data = next_u.values().data();
        const double* ELASTIC_WAVE_RESTRICT previous_v_data = previous_v.values().data();
        const double* ELASTIC_WAVE_RESTRICT current_v_data = current_v.values().data();
        double* ELASTIC_WAVE_RESTRICT next_v_data = next_v.values().data();

        for (std::size_t y = 1; y + 1 < rows_; ++y) {
            const std::size_t row = y * columns_;
            ELASTIC_WAVE_VECTORIZE
            for (std::size_t x = 1; x + 1 < columns_; ++x) {
                const std::size_t i = row + x;
                next_u_data[i] = damping_denominator * (
                    au * (current_u_data[i + 1] + current_u_data[i - 1]) +
                    bu * (current_u_data[i + columns_] + current_u_data[i - columns_]) +
                    coupling * (current_v_data[i + columns_ + 1] -
                                current_v_data[i - columns_ + 1] -
                                current_v_data[i + columns_ - 1] +
                                current_v_data[i - columns_ - 1]) +
                    2.0 * (1.0 - au - bu) * current_u_data[i] -
                    previous_weight * previous_u_data[i]);
                next_v_data[i] = damping_denominator * (
                    av * (current_v_data[i + columns_] + current_v_data[i - columns_]) +
                    bv * (current_v_data[i + 1] + current_v_data[i - 1]) +
                    coupling * (current_u_data[i + columns_ + 1] -
                                current_u_data[i - columns_ + 1] -
                                current_u_data[i + columns_ - 1] +
                                current_u_data[i - columns_ - 1]) +
                    2.0 * (1.0 - av - bv) * current_v_data[i] -
                    previous_weight * previous_v_data[i]);
            }
        }
        clear_boundaries(next_u);
        clear_boundaries(next_v);
        std::swap(previous_u, current_u);
        std::swap(current_u, next_u);
        std::swap(previous_v, current_v);
        std::swap(current_v, next_v);
        if (observer) observer({step, step * configuration_.dt, current_u, current_v});
    }
}

const Configuration& Simulation::configuration() const noexcept { return configuration_; }
std::size_t Simulation::columns() const noexcept { return columns_; }
std::size_t Simulation::rows() const noexcept { return rows_; }
std::size_t Simulation::steps() const noexcept { return steps_; }

} // namespace elastic_wave
