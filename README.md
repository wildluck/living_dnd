# Living World Engine

a persistent D&D world simulator written in C++23. Generates a living hex-grid world with procedural terrain, dynamic weather, kingdoms, trade routes, and mechanical effects - designed as a DM tool that runs a breathing world between sessions.

---

## What It Does

Drop a seed number and the engine builds a complete fantasy world:
- **Procedural terrain** - continents, mountains, rivers, biomes, vis simplex noise and FBM
- **Settlements** - cities, towns, villages placed by geographic desirability (rivers, coast, fertile land)
- **Road network** - minimum spanning tree + A* pathfindind through terrain
- **Trade routes** - resource production and comsumption between settlemets
- **Kingdoms** - Voronoi territories from capital cities with diplomatic relationships
- **Dungeons & POIs** - caves, ruins, dragon lairs scattered in remote wilderness
- **Climate zones** — latitude + elevation + coast proximity
- **Dynamic weather** — temperature, wind, precipitation, storms that change every tick
- **D&D 5e effects** — weather and terrain automatically resolve into mechanical modifiers (skill checks, saving throws, difficult terrain, damage)
- **Live simulation** — tick-by-tick world updates at 1 in-game day per second

Everything is deterministic - same seed procedures the same world, same weather, same events. Save the seed to reproduce anything.

---

## Building

Requires C++23 and CMake 3.25+

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j${nproc}
```

## Running Tests
```bash
ctest --test-dir build --output-on-failure
```

## Live (Weather) Simulation

```bash
./build/live_sim                         # random seed, radius 20, 1 sec/tick
./build/live_sim --radius 30 --speed 500 # bigger world, faster ticks
./build/live_sim                         # reproducible world
```

Watch weather evolve in real time - stomrs form and spread, seasons shift, temperature rises and falls across the map.

---

## World Generation Stages

| Stage            | What it does                                    |
| ---------------- | ----------------------------------------------- |
| 1.  Elevation    | FBM noise + radial island falloff               |
| 2.  Moisture     | Independent FBM noise layer                     |
| 3.  Biomes       | 5×3 lookup table (elevation × moisture)         |
| 4.  Rivers       | Peak-to-sea flow accumulation                   |
| 5.  Settlements  | Score by river/coast/biome + spacing constraint |
| 6.  Roads        | MST between settlements, A* for actual paths    |
| 7.  Trade routes | Producer->consumer matching along roads         |
| 8.  POIs         | Remoteness scoring, biome-themed dungeons       |
| 9.  Kingdoms     | Voronoi partition from capital cities           |
| 10. Climate      | Latitude + elevation + coast -> climate zone    |

## Sample output

- **Weather** 
- **World map & roads**
- **Kingdom map**

---

## Roadmap

### Done
- [x] Hex grid library — cube coordinates, neighbors, rings, radius, rotation, line drawing, pixel conversion
- [x] Grid container — templated sparse hex grid with get/set/neighbors/filter/BFS search
- [x] Pathfinding — A*, BFS, flood fill, distance map, weighted flood fill
- [x] Graph algorithms — Voronoi partition, connected components, minimum spanning tree
- [x] Simplex noise — seeded 2D coherent noise with permutation table
- [x] Fractal Brownian motion — multi-octave noise layering
- [x] Domain warp — organic coastline distortion
- [x] Dice/RNG — deterministic seeded random: rolls, weighted picks, shuffles, distributions
- [x] World generation — 10-stage pipeline (elevation, moisture, biomes, rivers, settlements, roads, trade, POIs, kingdoms, climate)
- [x] World clock — tick system with day/season/year tracking and callbacks
- [x] Weather engine — per-hex temperature, wind, precipitation, storms with cellular automata spread
- [x] Effect resolver — weather + terrain → D&D 5e mechanical modifiers
- [x] Live simulation — real-time terminal visualization with weather overlay
- [x] CMake build system — clean include paths, 8 test suites, all passing
- [x] Directional rivers — downstream cheap, upstream expensive for pathfinding

### Next
- [ ] State machine — towns grow/shrink, kingdoms expand/contract, roads decay
- [ ] Cascade — butterfly effect propagation between hexes
- [ ] Encounter table — dynamic encounters based on location + weather + faction
- [ ] Narrative engine — template-based prose generation for world events
- [ ] History ledger — event sourcing write-ahead log
- [ ] Gateway TUI — interactive DM dashboard (ftxui)

### Future
- [ ] ZeroMQ messaging — decouple services for parallel simulation
- [ ] Proper river segments with branching, deltas, and lakes
- [ ] Orographic moisture (rain shadow behind mountains)
- [ ] Hydraulic erosion for realistic terrain carving
- [ ] SIMD batch noise sampling
- [ ] Multi-hex settlements (cities occupy 3-7 hexes)
- [ ] Parallel stage execution with std::execution

---

## Improvement Ideas

See [docs/TODO.md](docs/TODO.md) for the full detailed backlog. Highlights:

**World Gen**
- Proper river segments — branching, deltas, lakes, harbor mouths
- Orographic moisture — rain shadow, coastal humidity, river moisture boost
- Hydraulic erosion — water carving channels, sediment deposits
- Multi-hex settlements — cities cover 3-7 hexes with crop fields
- Coastal biomes — beaches, cliffs, tidal flats, coral reefs
- Road decay — weather degrades roads, trade traffic maintains them
- Dynamic kingdoms — wars shift borders, rebellions split territories
**Weather**
- Pressure system cellular automata — moving fronts, rain shadows
- Markov chain extreme events — multi-day storms, magical weather
- Weather history — drought/flood detection from running averages
**Effects**
- Compound conditions — cold + wet = hypothermia, fog + swamp = hag encounters
- Equipment degradation from weather exposure
- Encounter modifiers — storm + night = ambush advantage
**Hex Library**
- SIMD batch noise (AVX2)
- Hex-based field of view with elevation-aware LOS
- Named regions auto-generated from biome clusters
**General**
- Binary serialization for save/load
- YAML config for tuning without recompilation
- JSON export for external tools

---

## License
 
MIT
