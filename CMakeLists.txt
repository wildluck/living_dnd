cmake_minimum_required(VERSION 3.20)
project(living_dnd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------- Header-only libraries ---------------- #

add_library(hex INTERFACE)
target_include_directories(hex INTERFACE ${CMAKE_SOURCE_DIR}/libs/hex)

add_library(noise INTERFACE)
target_include_directories(noise INTERFACE ${CMAKE_SOURCE_DIR}/libs/noise)

add_library(dice INTERFACE)
target_include_directories(dice INTERFACE ${CMAKE_SOURCE_DIR}/libs/dice)

add_library(common INTERFACE)
target_include_directories(common INTERFACE ${CMAKE_SOURCE_DIR}/libs/common)
target_link_libraries(common INTERFACE hex)

add_library(display INTERFACE)
target_include_directories(display INTERFACE ${CMAKE_SOURCE_DIR}/libs/display)
target_link_libraries(display INTERFACE common weather_engine effect_resolver)

# ---------------- Service libraries ---------------- #

add_library(world_gen STATIC services/world-gen/world_gen.cpp)
target_include_directories(world_gen PUBLIC ${CMAKE_SOURCE_DIR}/services/world-gen)
target_link_libraries(world_gen PUBLIC common dice)
target_link_libraries(world_gen PRIVATE noise)

add_library(world_clock INTERFACE)
target_include_directories(world_clock INTERFACE ${CMAKE_SOURCE_DIR}/services/world-clock)

add_library(weather_engine STATIC services/weather-engine/weather_engine.cpp)
target_include_directories(weather_engine PUBLIC ${CMAKE_SOURCE_DIR}/services/weather-engine)
target_link_libraries(weather_engine PUBLIC common world_clock dice)

add_library(effect_resolver STATIC services/effect-resolver/effect_resolver.cpp)
target_include_directories(effect_resolver PUBLIC ${CMAKE_SOURCE_DIR}/services/effect-resolver)
target_link_libraries(effect_resolver PUBLIC common weather_engine world_clock)

add_library(state_machine STATIC services/state-machine/state_machine.cpp)
target_include_directories(state_machine PUBLIC ${CMAKE_SOURCE_DIR}/services/state-machine)
target_link_libraries(state_machine PUBLIC common weather_engine)

add_library(cascade STATIC services/cascade/cascade.cpp)
target_include_directories(cascade PUBLIC ${CMAKE_SOURCE_DIR}/services/cascade)
target_link_libraries(cascade PUBLIC common weather_engine state_machine)

# ---------------- Test support ---------------- #

add_library(test_support INTERFACE)
target_include_directories(test_support INTERFACE ${CMAKE_SOURCE_DIR}/tests)

# ---------------- Tests ---------------- #

enable_testing()

add_executable(test_coord tests/test_coord.cpp)
target_link_libraries(test_coord PRIVATE hex test_support)
add_test(NAME test_coord COMMAND test_coord)

add_executable(test_grid tests/test_grid.cpp)
target_link_libraries(test_grid PRIVATE hex test_support)
add_test(NAME test_grid COMMAND test_grid)

add_executable(test_algo tests/test_algo.cpp)
target_link_libraries(test_algo PRIVATE hex test_support)
add_test(NAME test_algo COMMAND test_algo)

add_executable(test_noise tests/test_noise.cpp)
target_link_libraries(test_noise PRIVATE noise test_support)
add_test(NAME test_noise COMMAND test_noise)

add_executable(test_world_gen tests/test_world_gen.cpp)
target_link_libraries(test_world_gen PRIVATE world_gen display test_support)
add_test(NAME test_world_gen COMMAND test_world_gen)

add_executable(test_world_clock tests/test_world_clock.cpp)
target_link_libraries(test_world_clock PRIVATE world_clock common test_support)
add_test(NAME test_world_clock COMMAND test_world_clock)

add_executable(test_weather_engine tests/test_weather_engine.cpp)
target_link_libraries(test_weather_engine PRIVATE weather_engine world_gen display test_support)
add_test(NAME test_weather_engine COMMAND test_weather_engine)

add_executable(test_effect_resolver tests/test_effect_resolver.cpp)
target_link_libraries(test_effect_resolver PRIVATE effect_resolver weather_engine world_gen display test_support)
add_test(NAME test_effect_resolver COMMAND test_effect_resolver)

add_executable(test_state_machine tests/test_state_machine.cpp)
target_link_libraries(test_state_machine PRIVATE state_machine weather_engine world_gen display test_support)
add_test(NAME test_state_machine COMMAND test_state_machine)

add_executable(test_cascade tests/test_cascade.cpp)
target_link_libraries(test_cascade PRIVATE cascade weather_engine world_gen display test_support)
add_test(NAME test_cascade COMMAND test_cascade)

# ---------------- Live simulation --------------- #

add_executable(live_sim src/live_sim.cpp)
target_link_libraries(live_sim PRIVATE world_gen weather_engine display)

# ---------------- Main ---------------- #

add_executable(living_dnd src/main.cpp)
target_link_libraries(living_dnd PRIVATE world_gen weather_engine effect_resolver state_machine)
