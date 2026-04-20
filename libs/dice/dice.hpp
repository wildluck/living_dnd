#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

class RNG {
public:
RNG(uint64_t seed) : seed_(seed), gen_(seed) {}

/* ---------------- RNG ---------------- */
[[nodiscard]] int rand_int(int min, int max) noexcept {
    std::uniform_int_distribution<int> distrib(min, max);
    return distrib(gen_);
}

[[nodiscard]] double rand_float(double min, double max) noexcept {
    std::uniform_real_distribution<double> distrib(min, max);
    return distrib(gen_);
}

[[nodiscard]] double rand_float(float min, float max) noexcept {
    return rand_float(static_cast<double>(min), static_cast<double>(max));
}

[[nodiscard]] bool rand_bool(double probability = 0.5) noexcept {
    return rand_float(0.0, 1.0) < probability;
}

[[nodiscard]] double rand_normal(double mean = 0.0, double stddev = 1.0) noexcept {
    std::normal_distribution<double> distrib(mean, stddev);
    return distrib(gen_);
}

[[nodiscard]] inline constexpr uint64_t seed() noexcept { return seed_; }

/* ---------------- D&D dice ---------------- */
[[nodiscard]] int roll(int count, int sides) noexcept {
    int result = 0;
    for (int i = 0; i < count; i++) {
        result += rand_int(1, sides);
    }
    return result;
}

/* ---------------- Weighted selection ---------------- */
[[nodiscard]] int pick_weighted(const std::vector<int>& weights) {
    std::discrete_distribution<int> distrib(weights.begin(), weights.end());
    return distrib(gen_);
}

/* ---------------- Shuffle ---------------- */
template <typename T>
void shuffle(std::vector<T>& vec) {
    std::shuffle(vec.begin(), vec.end(), gen_);
}

/* ---------------- Pick random element ---------------- */
template <typename T>
[[nodiscard]] T& pick(std::vector<T>& vec) {
    return vec[rand_int(0, vec.size() - 1)];
}

template <typename T>
[[nodiscard]] const T& pick(std::vector<T>& vec) {
    return vec[rand_int(0, vec.size() - 1)];
}

private:
uint64_t seed_;
std::mt19937_64 gen_;
};
