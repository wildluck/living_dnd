# TODO List

Deferred improvements to circle back to after core system is working.

## world-gen

### Rivers - track segments properly
Current implementation only marks hexes as "has_river" and pushes a self-loop segment `{coord, coord, flow}`. Better version should:
- Track actual flow direction per hex: `{from_coord, to_coord, flow}` (already done)
- Merge branches when multiple streams converge
- Distinguish headwaters, mainstem, delta
- Handle lakes (local minima) - create lake hexes, route outflow to next downhill path if one exists
- Handle river mouths (where river meets ocean) - mark for harbor/delta biome modification

### Moisture - orographic effects
Current moisture is pure noise. Real moisture is affected by:
- Prevailing wind direction
- Rain shadow behind mountains (dry side)
- Proximity to coastline (wet)
- Rivers (nearby hexes get moisture boost)

### Elevation - hydraulic erosion
Current terrain is raw noise + radial falloff. Real terrain erodes:
- Simulate water droplets carving channels
- Sediment deposits in valleys
- Smoother slopes near rivers

### Settlements - several hexes (not one)
Current Settlement (no matter size) is a single hex. Real settlements:
- Bigger settlements cover more hexes
- Resources are based on covered lands
- Settlements have different edges and entrances
- Max trade routes are based on size
- Crop fields around towns and cities

### Biomes - coastal refinement
Currently no Beach/Cliff biomes assigned. Should:
- Mark land hexes adjacent to ocean as Beach (low elevation) or Cliff (high elevation)
- Add tidal flats, salt marshes in swampy coasts
- Coral reef biome for tropical shallow ocean

### Roads - maintenance and decay
Currently roads are permanent once built. Should:
- Roads have a condition value (0.0–1.0) that degrades over time
- Weather accelerates decay (storms, freezing)
- Trade traffic maintains roads (high-traffic routes stay good)
- Unmaintained roads revert to terrain
- Bridges at river crossings (separate from fords)

### Trade routes - dynamic supply and demand
Current trade is static producer→consumer matching. Should:
- Settlements consume resources based on population
- Surplus/deficit drives price changes
- New routes form when demand exceeds supply
- Routes break when roads decay or war blocks them
- Caravans as entities that travel along routes over multiple ticks

### Kingdoms - dynamic borders
Current kingdoms are static Voronoi. Should:
- Territory expands toward unclaimed or weak neighbors
- Wars cause border shifts
- Rebellions split kingdoms
- Alliances merge territories
- Border fortresses placed at contested borders

### POIs - dungeon ecology
Current POIs are static. Should:
- Monster populations grow over time if unchecked
- Raids from dungeons affect nearby settlements
- Clearing a dungeon reduces threat temporarily
- New POIs can spawn (bandits form camps, abandoned mines reopen)
- Loot tables based on dungeon type and CR

## weather-engine

### Full cellular automata
Plan specifies pressure/humidity/wind propagation between neighbors. Initial implementation may use simpler approach:
- Pressure systems that move across the map
- Humidity carried by wind direction
- Rain shadow effect when wind hits mountains
- Temperature fronts that create weather boundaries
- Multi-day storm systems that track across the world

### Markov chain extreme events
Per-climate-zone transition tables for blizzards, sandstorms, magical weather. Triggered by state-machine events (army marching → dust clouds):
- Transition matrices per climate zone per season
- Event duration tracking (storms last 1-3 days)
- Magical weather events (wild magic zones, planar bleeding)
- Seasonal mega-events (monsoon season, dry season, aurora)

### Weather history
- Track weather over past N ticks per hex
- Running averages for temperature, precipitation
- Drought detection (prolonged low precipitation)
- Flood detection (prolonged high precipitation + river)

## effect-resolver

### Compound effects
Current effects stack independently. Should:
- Detect compound conditions (cold + wet = hypothermia risk)
- Fatigue accumulation over multiple ticks of harsh weather
- Acclimatization (characters in cold region for many days get bonus)
- Equipment degradation from weather exposure

### Encounter modifiers
Effects should influence encounter difficulty:
- Storm + night = ambush advantage for monsters
- Heavy rain = reduced patrol range for town guards
- Extreme cold = undead are unaffected, living creatures weakened
- Fog + swamp = hag encounter probability increases

## hex library

### Performance
- SIMD batch noise sampling - with AVX2 we could sample 4-8 hexes in parallel
- `std::execution::par_unseq` for stage loops - each hex's elevation/moisture/biome is independent
- Spatial hashing for large grids (bucket-based neighbor lookup)
- Memory pool allocator for HexGrid cells

### Field of view
- Hex-based raycasting for line of sight
- Elevation-aware visibility (can see farther from hilltops)
- Forest/jungle blocks LOS
- Fog of war per-faction

### Hex regions
- Named regions (The Whispering Woods, The Iron Coast)
- Region-based event targeting
- Region-based encounter tables
- Auto-naming based on biome clusters

## narrative engine (future)

### Template system
- Sentence templates with slot filling: "{settlement} faces {threat} from {direction}"
- Variation pools to avoid repetition
- Tone adjustment based on severity
- Multi-sentence event descriptions

### Event compression
- Summarize multiple small events into one narrative
- "The northern border saw three skirmishes this season"
- Weekly/monthly/seasonal summaries for DM briefing

## general

### Serialization
- Save/load WorldData to binary format
- Save weather state for pause/resume
- Export world to JSON for external tools
- Import custom maps

### Configuration
- YAML/TOML config file for world generation parameters
- Per-biome tuning without recompilation
- Weather intensity scaling
- Difficulty scaling for effect resolver
