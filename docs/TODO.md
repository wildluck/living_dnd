# TODO List

Deferred improvements to circle back to after core system is working.

## world-gen

### Rivers - track segments properly
Current implementation only marks hexes as "has_river" and pushes a self-loop segment `{coord, coord, flow}`. Better version should:
- Track actual flow direction per hex: `{from_coord, to_coord, flow}`
- Merge branches when multiple streams converge
- Distinguish headwaters, mainstem, delta
- Handle lakes (local minima) - create lake hexes, route outflow to next downhill path if one exists
- Handle river mouths (where river meets ocean) - mark for harbor/delta biome modification

### Moisture - orographic effects
Current moisture is pure noise. Real moisture is affected by:
- Prevailing wind directon
- Rain shadow behind mountains (dry side)
- Proximity to coastline (wet)
- Rivers (nearby hexes get moisture boost)

### Elevation - hydraulic erosion
Current terrain is raw noise + radical falloff. Real terrain erodes:
- Simulate water droplets carving channels
- Sediment deposits in valleys
- Smoother slopes near rivers

### Settlements - several hexes (not one)
Current Settlement (no matter size) is a single Hex.
Real Settlement:
- Bigger settlements cover more hexes
- Resources are based on covered lands
- Settlements have different egdes and enters
- Max trade routes are based on size

## weather-engine (future)

### Full cellular automata

### Full cellular automata
Plan specifies pressure/humidity/wind propagation between neighbors. Initial implementation may use simpler approach.

### Markov chain extreme events
Per-climate-zone transition tables for blizzards, sandstorms, magical weather. Triggered by state-machines events (army marching -> dust clouds).

## Performance

### SIMD batch noise sampling
Currently sample one hex at a time. With AVX2 we could sample 4-8 hexes in parallel.

### std::execution::par_unseq for stage loops
Each hex's elevation/moisture/biome is independent - parallelize.
