#pragma once

#include "hex/coord.hpp"

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace lwe::hex {

struct PathResult {
    std::vector<Coord> path;
    double total_cost = 0;
    [[nodiscard]] bool found() const { return !path.empty(); }
};

/* Pathfinding */
template<typename F> requires std::invocable<F, const Coord&, const Coord&>
std::optional<PathResult> a_star(const Coord& start, const Coord& goal, F&& cost_fn, int max_radius) {
    if (start == goal) return PathResult{{start}, 0.0};

    using PQEntry = std::pair<double, Coord>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pri_queue;
    pri_queue.emplace(start.distance_to(goal), start);

    std::unordered_map<Coord, Coord> came_from;
    std::unordered_map<Coord, double> cost_so_far;

    came_from[start] = start;
    cost_so_far.emplace(start, 0.0);

    while (!pri_queue.empty()) {
        auto [priority, current] = pri_queue.top();
        pri_queue.pop();

        double current_cost = cost_so_far.at(current);

        if (priority > current_cost + current.distance_to(goal) + 1e-6) continue;

        if (current == goal) {
            PathResult result;
            result.total_cost = current_cost;
            for (Coord curr = goal; !(curr == start); curr = came_from.at(curr)) {
                result.path.push_back(curr);
            }
            result.path.push_back(start);
            std::reverse(result.path.begin(), result.path.end());

            return result;
        }

        for (const auto& n : current.neighbors()) {
            if (max_radius > 0 && n.distance_to(start) > max_radius) continue;

            double edge_cost = cost_fn(current, n);
            if (edge_cost < 0) continue; /* impassable */
            double new_cost = cost_so_far.at(current) + edge_cost;
            if (cost_so_far.find(n) == cost_so_far.end() || new_cost < cost_so_far.at(n)) {
                cost_so_far[n] = new_cost;
                came_from[n] = current;
                pri_queue.emplace(new_cost + n.distance_to(goal), n);
            }
        }
    }

    return std::nullopt;
}

template<typename F> requires std::invocable<F, const Coord&>
std::optional<PathResult> bfs(const Coord& start, const Coord& goal, F&& passable_fn) {
    if (!passable_fn(start)) return std::nullopt;
    if (start == goal) return PathResult{{start}, 0.0};

    std::queue<Coord> queue;
    queue.push(start);

    std::unordered_map<Coord, Coord> came_from;
    came_from[start] = start;

    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();

        if (current == goal) {
            PathResult result;
            for (Coord curr = goal; !(curr == start); curr = came_from.at(curr)) {
                result.path.push_back(curr);
            }
            result.path.push_back(start);
            std::reverse(result.path.begin(), result.path.end());

            result.total_cost = static_cast<double>(result.path.size() - 1);
            return result;
        }

        for (const auto& n : current.neighbors()) {
            if (passable_fn(n) && came_from.find(n) == came_from.end()) {
                came_from.emplace(n, current);
                queue.push(n);
            }
        }
    }

    return std::nullopt;
}

/* Flood and distance */
template<typename F> requires std::invocable<F, const Coord&>
std::unordered_set<Coord> flood_fill(const Coord& start, F&& passable_fn, int max_radius) {
    if (!passable_fn(start)) return {};

    std::unordered_set<Coord> result;
    result.emplace(start);

    std::queue<Coord> queue;
    queue.push(start);

    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();

        for (const auto& n : current.neighbors()) {
            if (result.contains(n)) continue; /* already visited */
            if (max_radius > 0 && n.distance_to(start) > max_radius) continue;

            if (passable_fn(n)) {
                result.emplace(n);
                queue.push(n);
            }
        }
    }

    return result;
}

template<typename F> requires std::invocable<F, const Coord&>
std::unordered_map<Coord, int> distance_map(const Coord& source, F&& passable_fn, int max_radius) {
    if (!passable_fn(source)) return {};
    std::unordered_map<Coord, int> result;
    result[source] = 0;

    std::queue<Coord> queue;
    queue.push(source);

    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();

        for (const auto& n : current.neighbors()) {
            if (result.contains(n)) continue;
            if (max_radius > 0 && n.distance_to(source) > max_radius) continue;
            if (passable_fn(n)) {
                result[n] = result[current] + 1;
                queue.push(n);
            }
        }
    }

    return result;
}

template<typename F> requires std::invocable<F, const Coord&, const Coord&>
std::unordered_map<Coord, double> weighted_flood_fill(const Coord& start, F&& cost_fn, double max_cost) {
    using PQEntry = std::pair<double, Coord>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pri_queue;
    pri_queue.emplace(0.0, start);

    std::unordered_map<Coord, double> result;

    result.emplace(start, 0.0);

    while (!pri_queue.empty()) {
        auto [current_cost, current] = pri_queue.top();
        pri_queue.pop();
        if (current_cost > result[current]) continue;
        for (const auto& n : current.neighbors()) {
            double edge_cost = cost_fn(current, n);
            if (edge_cost < 0) continue; /* impassable */
            double new_cost = result.at(current) + edge_cost;
            if (new_cost > max_cost) continue;
            if (result.find(n) == result.end() || new_cost < result.at(n)) {
                result[n] = new_cost;
                pri_queue.emplace(new_cost, n);
            }
        }
    }

    return result;
}

/* Graph algorithms */
template<typename F> requires std::invocable<F, const Coord&>
std::unordered_map<Coord, int> voronoi_partition(const std::vector<Coord>& seeds, F&& passable_fn) {
    std::unordered_map<Coord, int> result;

    std::queue<Coord> queue;
    for (int i = 0; i < static_cast<int>(seeds.size()); ++i) {
        result[seeds[i]] = i;
        queue.push(seeds[i]);
    }

    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();

        for (const auto& n : current.neighbors()) {
            if (result.contains(n)) continue;
            if (passable_fn(n)) {
                result[n] = result[current];
                queue.push(n);
            }
        }
    }

    return result;
}

template<typename F> requires std::invocable<F, const Coord&>
std::vector<std::unordered_set<Coord>> connected_components(F&& passable_fn, const std::vector<Coord>& coords) {
    std::vector<std::unordered_set<Coord>> result;
    std::unordered_set<Coord> visited;

    for (const auto& c : coords) {
        if (visited.contains(c)) continue;
        if (!passable_fn(c)) continue;

        auto component = flood_fill(c, passable_fn, 0);
        for (const auto& h : component) visited.insert(h);
        result.push_back(component);
    }

    return result;
}

template<typename F> requires std::invocable<F, const Coord&, const Coord&>
std::vector<std::pair<Coord, Coord>> minimum_spanning_tree(const std::vector<Coord>& nodes, F&& cost_fn) {
    if (nodes.size() <= 1) return {};

    std::vector<std::pair<Coord, Coord>> result;
    std::unordered_set<Coord> connected;
    /* edge = (cost, from, to) */
    using Edge = std::tuple<double, Coord, Coord>;
    std::priority_queue<Edge, std::vector<Edge>, std::greater<Edge>> pri_queue;

    connected.insert(nodes[0]);

    for (size_t i = 1; i < nodes.size(); i++) {
        pri_queue.emplace(cost_fn(nodes[0], nodes[i]), nodes[0], nodes[i]);
    }

    while (!pri_queue.empty() && connected.size() < nodes.size()) {
        auto [cost, from, to] = pri_queue.top();
        pri_queue.pop();

        if (connected.contains(to)) continue;

        connected.insert(to);
        result.emplace_back(from, to);

        for (const auto& n : nodes) {
            if (!connected.contains(n)) {
                pri_queue.emplace(cost_fn(to, n), to, n);
            }
        }
    }

    return result;
}

} /* namespace lwe::hex */
