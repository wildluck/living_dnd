#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <ostream>
#include <vector>

namespace lwe::hex {

/* ---------------- Hex grid (cubic) coordinates (pointy-top) ---------------- */
struct Coord {
    int q, r;

    constexpr Coord() = default;
    constexpr Coord(int q, int r) : q(q), r(r) {}

    [[nodiscard]] constexpr int s() const noexcept { return -q - r; }

    /* Comparison operators */
    constexpr bool operator==(const Coord&) const = default;
    constexpr auto operator<=>(const Coord&) const = default;

    /* Arithmetic operators */
    constexpr Coord operator+(const Coord& o) const noexcept {
        return { q + o.q, r + o.r };
    }
    constexpr Coord operator-(const Coord& o) const noexcept {
        return { q - o.q, r - o.r };
    }
    constexpr Coord operator*(int scalar) const noexcept {
        return { q * scalar, r * scalar };
    }
    constexpr Coord operator-() const noexcept {
        return { -q, -r };
    }
    constexpr Coord& operator+=(const Coord& o) noexcept {
        q += o.q;
        r += o.r;
        return *this;
    }
    constexpr Coord& operator-=(const Coord& o) noexcept {
        q -= o.q;
        r -= o.r;
        return *this;
    }

    /* ---------------- Distance ---------------- */
    [[nodiscard]] constexpr int distance_to(const Coord& o) const {
        const auto d = *this - o;
        return (std::abs(d.q) + std::abs(d.r) + std::abs(d.s())) / 2;
    }
    [[nodiscard]] constexpr int length() const { /* Distance from origin */
        return (std::abs(q) + std::abs(r) + std::abs(s())) / 2;
    }

    /* ---------------- Neighbors ---------------- */
    enum class Direction : uint8_t {
        NorthEast = 0,
        East,
        SouthEast,
        SouthWest,
        West,
        NorthWest,

        DirectionCount = 6,
    };

    template <typename F>
    static constexpr void for_each_direction(F&& fn) {
        for (uint8_t i = 0; i < static_cast<uint8_t>(Direction::DirectionCount); ++i)
            fn(static_cast<Direction>(i));
    }

    [[nodiscard]] static constexpr Coord direction(Direction dir) noexcept {
        switch(dir) {
        case Direction::NorthEast: return { +1, -1 };
        case Direction::East: return { 1, 0 };
        case Direction::SouthEast: return { 0, 1 };
        case Direction::SouthWest: return { -1, 1 };
        case Direction::West: return { -1, 0 };
        case Direction::NorthWest: return { 0, -1 };
        default: return { 0, 0 }; /* Unreachable */
        }
    }

    [[nodiscard]] constexpr Coord neighbor(Direction dir) const noexcept {
        return *this + direction(dir);
    }

    [[nodiscard]] constexpr std::array<Coord, 6> neighbors() const noexcept {
        return {
            neighbor(Direction::NorthEast),
            neighbor(Direction::East),
            neighbor(Direction::SouthEast),
            neighbor(Direction::SouthWest),
            neighbor(Direction::West),
            neighbor(Direction::NorthWest),
        };
    }

    [[nodiscard]] Direction direction_between(const Coord& o) const noexcept {
        const auto d = o - *this;
        if (d.q == 0 && d.r == 0) return Direction::DirectionCount;

        int best = 0;
        int best_dist = std::numeric_limits<int>::max();
        for (int i = 0; i < 6; ++i) {
            auto dir = direction(static_cast<Direction>(i));
            int len = d.length();
            auto scaled = dir * len;
            int dist = (scaled - d).length();
            if (dist < best_dist) {
                best_dist = dist;
                best = i;
            }
        }
        return static_cast<Direction>(best);
    }

    /* ---------------- Rings and ranges ---------------- */
    [[nodiscard]] std::vector<Coord> ring(int radius) const noexcept {
        if (radius == 0) return {*this};

        std::vector<Coord> result;
        result.reserve(radius * 6);

        /* We go west and then walk around the ring */
        Coord curr = *this + direction(Direction::West) * radius;

        for_each_direction([&](Direction side) {
            for (int step = 0; step < radius; ++step) {
                result.push_back(curr);
                curr = curr.neighbor(side);
            }
        });

        return result;
    }

    [[nodiscard]] static constexpr int hex_count(int radius) noexcept {
        return 1 + 3 * radius * (radius + 1);
    }

    [[nodiscard]] std::vector<Coord> within_radius(int radius) const noexcept {
        if (radius == 0) return {*this};

        std::vector<Coord> result;
        result.reserve(hex_count(radius));

        Coord curr = *this;
        result.push_back(curr);
        curr = curr.neighbor(Direction::West);

        for (int r = 1; r <= radius; ++r) {
            for_each_direction([&](Direction side) {
                for (int step = 0; step < r; ++step) {
                    curr = curr.neighbor(side);
                    result.push_back(curr);
                }
            });
            curr = curr.neighbor(Direction::West);
        }

        return result;
    }

    /* ---------------- Rotation ---------------- */
    /* Rotate clockwise 60 degrees - (q, r, s) -> (-r, -s, -q) */
    [[nodiscard]] constexpr Coord rotate_cw() const noexcept {
        return { -r, -s() };
    }

    /* Rotate counterclockwise 60 degrees - (q, r, s) -> (-s, -q, -r) */
    [[nodiscard]] constexpr Coord rotate_ccw() const noexcept {
        return { -s(), -q };
    }

    [[nodiscard]] constexpr Coord rotate_cw_around(const Coord& center) const noexcept {
        return (*this - center).rotate_cw() + center;
    }

    [[nodiscard]] constexpr Coord rotate_ccw_around(const Coord& center) const noexcept {
        return (*this - center).rotate_ccw() + center;
    }

    /* ---------------- Line and shape ---------------- */
    [[nodiscard]] std::vector<Coord> line_to(const Coord& target) const noexcept {
        const int dist = distance_to(target);
        if (dist == 0) return {*this};

        std::vector<Coord> result;
        result.reserve(dist + 1);

        for (int i = 0; i <= dist; ++i) {
            const double t = static_cast<double>(i) / dist;
            const double fq = q + (target.q - q) * t;
            const double fr = r + (target.r - r) * t;
            result.emplace_back(cube_round(fq, fr));
        }

        return result;
    }

    [[nodiscard]] std::vector<Coord> cone(Direction dir, int radius, int spread) const noexcept {
        std::vector<Coord> result;
        for (int i = -spread; i <= spread; ++i) {
            Direction d = static_cast<Direction>(((static_cast<int>(dir) + i) % 6 + 6) % 6);
            auto w = wedge(d, radius);
            result.insert(result.end(), w.begin(), w.end());
        }

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());

        return result;
    }

    [[nodiscard]] std::vector<Coord> wedge(Direction dir, int radius) const noexcept {
        std::vector<Coord> result;

        Direction walk = static_cast<Direction>((static_cast<int>(dir) + 2) % 6);

        for (int dist = 1; dist <= radius; ++dist) {
            Coord curr = *this + direction(dir) * dist;
            for (int step = 0; step <= dist; ++step) {
                result.push_back(curr);
                curr = curr.neighbor(walk);
            }
        }

        return result;
    }

    /* ---------------- Pixel conversion ---------------- */
    struct PixelPos { double x, y; };

    [[nodiscard]] PixelPos to_pixel(double hex_size) const noexcept {
        double x = (std::sqrt(3) * q + std::sqrt(3) / 2 * r);
        double y = 3.0 / 2 * r;
        x = x * hex_size;
        y = y * hex_size;
        return PixelPos{ x, y };
    }

    [[nodiscard]] static Coord from_pixel(PixelPos px, double hex_size) noexcept {
        double x = px.x / hex_size;
        double y = px.y / hex_size;
        double q = (std::sqrt(3) / 3 * x - 1.0 / 3 * y);
        double r = (2.0 / 3 * y);
        return cube_round(q, r);
    }

    /* ---------------- Utility ---------------- */
    friend std::ostream& operator<<(std::ostream& os, const Coord& c) {
        return os << "(" << c.q << ", " << c.r << ")";
    }

    /* Rounding a cube coordinate to the nearest integer */
    [[nodiscard]] static constexpr Coord cube_round(double fq, double fr) noexcept {
        int rq = static_cast<int>(std::round(fq));
        int rr = static_cast<int>(std::round(fr));
        int rs = static_cast<int>(std::round(-fq - fr));

        const double dq = std::abs(rq - fq);
        const double dr = std::abs(rr - fr);
        const double ds = std::abs(rs - (-fq - fr));

        if (dq > dr && dq > ds) {
            rq = -rr - rs;
        } else if (dr > ds) {
            rr = -rq - rs;
        }

        return { rq, rr };
    }
};

} /* namespace lwe::hex */

template<>
struct std::hash<lwe::hex::Coord> {
    std::size_t operator()(const lwe::hex::Coord& c) const noexcept {
        auto h = static_cast<std::size_t>(c.q);
        h ^= static_cast<std::size_t>(c.r) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }
};
