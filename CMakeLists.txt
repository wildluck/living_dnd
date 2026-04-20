cmake_minimum_required(VERSION 3.20)
project(living_dnd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(
  ${CMAKE_SOURCE_DIR}/libs
  ${CMAKE_SOURCE_DIR}/libs/display
  ${CMAKE_SOURCE_DIR}/services/world-gen
  ${CMAKE_SOURCE_DIR}/services/world-clock
  ${CMAKE_SOURCE_DIR}/services/weather-engine
  ${CMAKE_SOURCE_DIR}/services/effect-resolver
  ${CMAKE_SOURCE_DIR}/services/state-machine
)

# ---------------- Libraries (header-only) ---------------- #
add_library(hex INTERFACE)
target_include_directories(hex INTERFACE ${CMAKE_SOURCE_DIR}/libs)

add_library(noise INTERFACE)
target_include_directories(noise INTERFACE ${CMAKE_SOURCE_DIR}/libs)

add_library(dice INTERFACE)
target_include_directories(dice INTERFACE ${CMAKE_SOURCE_DIR}/libs)

add_library(common INTERFACE)
target_include_directories(common INTERFACE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(common INTERFACE hex)

add_library(display INTERFACE)
target_include_directories(display INTERFACE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(display INTERFACE common weather_engine effect_resolver)

# ---------------- Services ---------------- #
add_library(world_gen
  services/world-gen/world_gen.cpp
)
target_link_libraries(world_gen PUBLIC common noise dice)

add_library(world_clock INTERFACE)
target_include_directories(world_clock INTERFACE ${CMAKE_SOURCE_DIR}/services/world-clock)

add_library(weather_engine
  services/weather-engine/weather_engine.cpp
)
target_link_libraries(weather_engine PUBLIC common world_clock dice)

add_library(effect_resolver
  services/effect-resolver/effect_resolver.cpp
)
target_link_libraries(effect_resolver PUBLIC common weather_engine world_clock)

add_library(state_machine services/state-machine/state_machine.cpp)
target_link_libraries(state_machine PUBLIC common weather_engine dice)

# ---------------- Tests ---------------- #
enable_testing()

add_executable(test_coord tests/test_coord.cpp)
target_link_libraries(test_coord hex)
add_test(NAME test_coord COMMAND test_coord)

add_executable(test_grid tests/test_grid.cpp)
target_link_libraries(test_grid hex)
add_test(NAME test_grid COMMAND test_grid)

add_executable(test_algo tests/test_algo.cpp)
target_link_libraries(test_algo hex)
add_test(NAME test_algo COMMAND test_algo)

add_executable(test_noise tests/test_noise.cpp)
target_link_libraries(test_noise noise)
add_test(NAME test_noise COMMAND test_noise)

add_executable(test_world_gen tests/test_world_gen.cpp)
target_link_libraries(test_world_gen world_gen)
add_test(NAME test_world_gen COMMAND test_world_gen)

add_executable(test_world_clock tests/test_world_clock.cpp)
target_link_libraries(test_world_clock world_clock common)
add_test(NAME test_world_clock COMMAND test_world_clock)

add_executable(test_weather_engine tests/test_weather_engine.cpp)
target_link_libraries(test_weather_engine weather_engine world_gen)
add_test(NAME test_weather_engine COMMAND test_weather_engine)

add_executable(test_effect_resolver tests/test_effect_resolver.cpp)
target_link_libraries(test_effect_resolver effect_resolver weather_engine world_gen)
add_test(NAME test_effect_resolver COMMAND test_effect_resolver)

add_executable(test_state_machine tests/test_state_machine.cpp)
target_link_libraries(test_state_machine state_machine weather_engine world_gen)
add_test(NAME test_state_machine COMMAND test_state_machine)

# ---------------- Live simulation --------------- #
add_executable(live_sim src/live_sim.cpp)
target_link_libraries(live_sim world_gen weather_engine state_machine)

# ---------------- Main ---------------- #
add_executable(living_dnd src/main.cpp)
target_link_libraries(living_dnd world_gen weather_engine effect_resolver state_machine)
