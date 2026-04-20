#pragma once

#include <cstdint>

#define RUN(name) do { std::print("  " #name "... "); name(); std::print("OK\n"); } while (0)

const uint64_t WORLD_SEED = 500;
const int WORLD_RADIUS = 40;
