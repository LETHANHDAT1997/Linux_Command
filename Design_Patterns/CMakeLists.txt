cmake_minimum_required(VERSION 3.16)
project(design_patterns_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(add_pattern target source)
  add_executable(${target} ${source})
endfunction()

add_pattern(factory_method
  Creational/factory_method/factory_method.cpp)
add_pattern(abstract_factory
  Creational/abstract_factory/abstract_factory.cpp)
add_pattern(builder
  Creational/builder/builder.cpp)
add_pattern(adapter
  Structural/adapter/adapter.cpp)
add_pattern(proxy
  Structural/proxy/proxy.cpp)
add_pattern(observer
  Behavioral/observer/observer.cpp)
add_pattern(state
  Behavioral/state/state.cpp)
add_pattern(strategy
  Behavioral/strategy/strategy.cpp)
