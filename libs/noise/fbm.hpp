#pragma once

#include "noise/simplex.hpp"

namespace lwe::noise {

class FBM {
public:
    FBM(const SimplexNoise& source,
        int octaves = 6,
        double lacunarity = 2.0,
        double persistence = 0.5) noexcept
        : source_(source)
        , octaves_(octaves)
        , lacunarity_(lacunarity)
        , persistence_(persistence)
    {}

    [[nodiscard]] double sample(double x, double y) const noexcept {
        double value = 0.0;
        double amplitude = 1.0;
        double frequency = 1.0;
        double max_amp = 0.0;

        for (int i = 0; i < octaves_; ++i) {
            value += source_.sample(x * frequency, y * frequency) * amplitude;
            max_amp += amplitude;
            frequency *= lacunarity_;
            amplitude *= persistence_;
        }

        return value / max_amp;
    }

    [[nodiscard]] int octaves() const noexcept { return octaves_; }
    [[nodiscard]] double lacunarity() const noexcept { return lacunarity_; }
    [[nodiscard]] double persistence() const noexcept { return persistence_; }

private:
    const SimplexNoise& source_;
    int octaves_;
    double lacunarity_;
    double persistence_;
};

class DomainWarp{
public:
    DomainWarp(const SimplexNoise& base,
               const SimplexNoise& warp_x,
               const SimplexNoise& warp_y,
               double strength = 0.5) noexcept
        : base_(base)
        , warp_x_(warp_x)
        , warp_y_(warp_y)
        , strength_(strength)
    {}

    [[nodiscard]] double sample(double x, double y) const noexcept {
        double wx = warp_x_.sample(x, y) * strength_;
        double wy = warp_y_.sample(x, y) * strength_;
        return base_.sample(x + wx, y + wy);
    }

    [[nodiscard]] double strength() const noexcept { return strength_; }

private:
    const SimplexNoise& base_;
    const SimplexNoise& warp_x_;
    const SimplexNoise& warp_y_;
    double strength_;
};

} /* namespace lwe::noise */
