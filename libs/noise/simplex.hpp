#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <utility>

namespace lwe::noise {

class SimplexNoise {
public:
    explicit SimplexNoise(uint64_t seed) noexcept : seed_(seed) {
        std::iota(perm_.begin(), perm_.begin() + 256, 0);
        uint64_t state = seed;
        for (int i = 255; i > 0; --i) {
            state = splitmix64(state);
            int j = static_cast<int>(state % static_cast<uint64_t>(i + 1));
            std::swap(perm_[i], perm_[j]);
        }

        for (int i = 0; i < 256; ++i) {
            perm_[256 + i] = perm_[i];
        }
    }

    [[nodiscard]] double sample(double x, double y) const noexcept {
        double F2 = 0.5 * (std::sqrt(3.0) - 1.0);
        double G2 = (3.0 - std::sqrt(3.0)) / 6.0;

        const double s = (x + y) * F2;
        const int i = fast_floor(x + s);
        const int j = fast_floor(y + s);

        const double t = (i + j) * G2;
        const double X0 = i - t;
        const double Y0 = j - t;
        const double x0 = x - X0;
        const double y0 = y - Y0;

        int i1, j1;
        if (x0 > y0) { i1 = 1; j1 = 0; }
        else { i1 = 0; j1 = 1; }

        const double x1 = x0 - i1 + G2;
        const double y1 = y0 - j1 + G2;
        const double x2 = x0 - 1.0 + 2.0 * G2;
        const double y2 = y0 - 1.0 + 2.0 * G2;

        const int ii = i & 255;
        const int jj = j & 255;
        const int gi0 = perm_[ii + perm_[jj]] % 12;
        const int gi1 = perm_[ii + i1 + perm_[jj + j1]] % 12;
        const int gi2 = perm_[ii + 1 + perm_[jj + 1]] % 12;

        double n0 = corner(x0, y0, gi0);
        double n1 = corner(x1, y1, gi1);
        double n2 = corner(x2, y2, gi2);

        return 70.0 * (n0 + n1 + n2);
    }

    [[nodiscard]] uint64_t seed() const noexcept { return seed_; }

private:
    uint64_t seed_;
    std::array<int, 512> perm_;

    static constexpr std::array<std::array<double, 2>, 12> grad2_ {{
        {{ 1, 1}}, {{-1, 1}}, {{ 1,-1}}, {{-1,-1}},
        {{ 1, 0}}, {{-1, 0}}, {{ 0, 1}}, {{ 0,-1}},
        {{ 1, 1}}, {{-1, 1}}, {{ 1,-1}}, {{-1,-1}},
    }};

    [[nodiscard]] static double corner(double x, double y, int gi) noexcept {
        double t = 0.5 - x * x - y * y;
        if (t < 0.0) return 0.0;
        t *= t;
        return t * t * (grad2_[gi][0] * x + grad2_[gi][1] * y);
    }

    [[nodiscard]] static int fast_floor(double x) noexcept {
        int xi = static_cast<int>(x);
        return (x < xi) ? xi - 1: xi;
    }

    [[nodiscard]] static uint64_t splitmix64(uint64_t x) noexcept {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }
};

} /* namespace lwe::noise */
