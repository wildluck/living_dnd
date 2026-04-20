#pragma once

#include "hex/coord.hpp"

#include <algorithm>
#include <climits>
#include <concepts>
#include <functional>
#include <optional>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lwe::hex {

template <typename CellT>
class HexGrid {
public:
HexGrid() {}
HexGrid(int expected_size) { cells_.reserve(expected_size); }

/* ---------------- Access ---------------- */
[[nodiscard]] std::optional<std::reference_wrapper<CellT>> get(const Coord& coord) noexcept {
    auto it = cells_.find(coord);
    if (it == cells_.end()) return std::nullopt;
    return std::ref(it->second);
}

[[nodiscard]] std::optional<std::reference_wrapper<const CellT>> get(const Coord& coord) const noexcept {
    auto it = cells_.find(coord);
    if (it == cells_.end()) return std::nullopt;
    return std::cref(it->second);
}

[[nodiscard]] CellT& at(const Coord& coord) {
    auto it = cells_.find(coord);
    if (it == cells_.end()) throw std::out_of_range("HexGrid::at: coord not found");
    return it->second;
}

[[nodiscard]] const CellT& at(const Coord& coord) const {
    auto it = cells_.find(coord);
    if (it == cells_.end()) throw std::out_of_range("HexGrid::at: coord not found");
    return it->second;
}

[[nodiscard]] CellT& operator[](const Coord& coord) { /* insert default if missing */
    return cells_[coord];
}

/* ---------------- Modification ---------------- */
void set(const Coord& coord, CellT value) {
    cells_.insert_or_assign(coord, std::move(value));
}

template <typename... Args>
[[nodiscard]] CellT& emplace(const Coord& coord, Args&&... args) { /* Construct in-place if missing */
    return cells_.try_emplace(coord, std::forward<Args>(args)...).first->second;
}

[[nodiscard]] bool erase(const Coord& coord) {
    auto it = cells_.find(coord);
    if (it == cells_.end()) return false;
    cells_.erase(it);
    return true;
}

void clear() {
    if (cells_.empty()) return;
    cells_.clear();
}

/* ---------------- Query ---------------- */
[[nodiscard]] bool contains(const Coord& coord) const {
    return cells_.find(coord) != cells_.end();
}

[[nodiscard]] std::size_t size() const {
    return cells_.size();
}

[[nodiscard]] bool empty() const {
    return cells_.empty();
}

/* ---------------- Neighbor access ---------------- */
[[nodiscard]] std::vector<std::pair<Coord, std::reference_wrapper<CellT>>> neighbors_of(Coord coord) {
    std::vector<std::pair<Coord, std::reference_wrapper<CellT>>> result;
    for (const auto& neighbor : coord.neighbors()) {
        auto it = cells_.find(neighbor);
        if (it != cells_.end()) {
            result.emplace_back(neighbor, std::ref(it->second));
        }
    }
    return result;
}

[[nodiscard]] const std::vector<std::pair<Coord, std::reference_wrapper<const CellT>>> neighbors_of(Coord coord) const {
    std::vector<std::pair<Coord, std::reference_wrapper<const CellT>>> result;
    for (const auto& neighbor : coord.neighbors()) {
        auto it = cells_.find(neighbor);
        if (it != cells_.end()) {
            result.emplace_back(neighbor, std::cref(it->second));
        }
    }
    return result;
}

/* ---------------- Filtering and search ---------------- */
template <typename F> requires std::invocable<F, const Coord&, const CellT&>
[[nodiscard]] std::size_t count_if(F&& fn) const {
    std::size_t count = 0;
    for (const auto& [coord, cell] : cells_) {
        if (fn(coord, cell)) ++count;
    }
    return count;
}

template <typename F> requires std::invocable<F, const Coord&, const CellT&>
[[nodiscard]] std::optional<std::pair<Coord, std::reference_wrapper<const CellT>>> find_if(F&& fn) const {
    for (const auto& [coord, cell] : cells_) {
        if (fn(coord, cell)) return std::make_optional(std::make_pair(coord, std::cref(cell)));
    }
    return std::nullopt;
}

template <typename F> requires std::invocable<F, const Coord&, const CellT&>
[[nodiscard]] std::vector<std::pair<Coord, std::reference_wrapper<const CellT>>> filter(F&& fn) const {
    std::vector<std::pair<Coord, std::reference_wrapper<const CellT>>> result;
    for (const auto& [coord, cell] : cells_) {
        if (fn(coord, cell)) result.emplace_back(std::make_pair(coord, std::cref(cell)));
    }
    return result;
}

/* BFS outward from target coord, returns first match */
template <typename F> requires std::invocable<F, const Coord&, const CellT&>
[[nodiscard]] std::optional<Coord> nearest_to(Coord target, F&& fn) const {
    std::queue<Coord> queue;
    std::unordered_set<Coord> visited;
    queue.push(target);
    visited.insert(target);
    while (!queue.empty()) {
        Coord coord = queue.front();
        queue.pop();
        CellT cell = at(coord);
        if (fn(coord, cell)) return std::make_optional(coord);
        for (const auto& [neighbor, _] : neighbors_of(coord)) {
            if (visited.find(neighbor) == visited.end()) {
                queue.push(neighbor);
                visited.insert(neighbor);
            }
        }
    }
    return std::nullopt;
}

/* ---------------- Spatial ---------------- */
[[nodiscard]] std::pair<Coord, Coord> bounding_box() const {
    Coord min = {INT_MAX, INT_MAX};
    Coord max = {INT_MIN, INT_MIN};
    for (const auto& [coord, _] : cells_) {
        min = {std::min(min.q, coord.q), std::min(min.r, coord.r)};
        max = {std::max(max.q, coord.q), std::max(max.r, coord.r)};
    }
    return {min, max};
}

[[nodiscard]] std::vector<Coord> adjacent_region(const std::unordered_set<Coord>& region) const {
    std::unordered_set<Coord> result;
    for (const auto& coord : region) {
        for (const auto& n : coord.neighbors()) {
            if (region.find(n) == region.end() && contains(n)) {
                result.insert(n);
            }
        }
    }
    return {result.begin(), result.end()};
}

/* ---------------- Transformation ---------------- */
template <typename F> requires std::invocable<F, const Coord&, const CellT&>
[[nodiscard]] auto transform(F&& fn) const {
    using U = std::invoke_result_t<F, Coord, const CellT&>;
    HexGrid<U> result;
    for (const auto& [coord, cell] : cells_) {
        result.set(coord, fn(coord, cell));
    }
    return result;
}

/* ---------------- Iteration ---------------- */
[[nodiscard]] auto begin() noexcept { return cells_.begin(); }
[[nodiscard]] auto end() noexcept { return cells_.end(); }
[[nodiscard]] auto cbegin() const noexcept { return cells_.begin(); }
[[nodiscard]] auto cend() const noexcept { return cells_.end(); }

template <typename F> requires std::invocable<F, const Coord&, CellT&>
void for_each(F&& fn) {
    for (auto& [coord, cell] : cells_) {
        fn(coord, cell);
    }
}

template <typename F> requires std::invocable<F, const Coord&, CellT&>
void for_each(F&& fn) const {
    for (const auto& [coord, cell] : cells_) {
        fn(coord, cell);
    }
}

[[nodiscard]] std::vector<Coord> coords() const {
    std::vector<Coord> result;
    for (const auto& [coord, _] : cells_) {
        result.push_back(coord);
    }
    return result;
}

/* ---------------- Bulk generation ---------------- */
template <typename F> requires std::invocable<F, const Coord&>
static HexGrid<CellT> generate(int radius, F&& fn) {
    HexGrid<CellT> grid(Coord::hex_count(radius));
    Coord origin(0, 0);
    for (const auto& c : origin.within_radius(radius)) {
        grid.set(c, fn(c));
    }
    return grid;
}

private:
std::unordered_map<Coord, CellT> cells_;
};

} /* namespace lwe::hex */
