cmake_minimum_required(VERSION 3.18)
project(gaborfit VERSION 0.1.0 LANGUAGES C CXX)

option(GABORFIT_BUILD_SHARED "Build shared C ABI library" ON)
option(GABORFIT_BUILD_TESTS "Build native tests" ON)
option(GABORFIT_ENABLE_CERES "Prefer Ceres when an installation is available" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
if(GABORFIT_ENABLE_CERES)
  find_package(Ceres QUIET)
endif()

set(GABORFIT_SOURCES src/gabor.cpp src/c_api.cpp)
if(GABORFIT_BUILD_SHARED)
  add_library(gaborfit SHARED ${GABORFIT_SOURCES})
else()
  add_library(gaborfit STATIC ${GABORFIT_SOURCES})
endif()
add_library(GaborFit::gaborfit ALIAS gaborfit)
set_target_properties(gaborfit PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  WINDOWS_EXPORT_ALL_SYMBOLS ON)
target_include_directories(gaborfit PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(gaborfit PUBLIC Eigen3::Eigen)
target_compile_definitions(gaborfit PRIVATE GABORFIT_BUILDING_LIBRARY)
if(Ceres_FOUND)
  target_compile_definitions(gaborfit PUBLIC GABORFIT_CERES_AVAILABLE=1)
  message(STATUS "Ceres ${Ceres_VERSION} found; API records availability (Eigen backend remains reproducibility default in v0.1)")
else()
  message(STATUS "Ceres not found; building tested Eigen bounded-LM backend")
endif()
if(MSVC)
  target_compile_options(gaborfit PRIVATE /W4 /permissive-)
else()
  target_compile_options(gaborfit PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(GABORFIT_BUILD_TESTS)
  enable_testing()
  add_executable(gaborfit_tests tests/test_gabor.cpp)
  target_link_libraries(gaborfit_tests PRIVATE GaborFit::gaborfit)
  add_test(NAME gaborfit_tests COMMAND gaborfit_tests)
  add_executable(gaborfit_c_api_tests tests/test_c_api.c)
  set_target_properties(gaborfit_c_api_tests PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON LINKER_LANGUAGE CXX)
  target_link_libraries(gaborfit_c_api_tests PRIVATE GaborFit::gaborfit)
  add_test(NAME gaborfit_c_api_tests COMMAND gaborfit_c_api_tests)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/GaborFitConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/GaborFitConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GaborFit)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/GaborFitConfigVersion.cmake
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(TARGETS gaborfit EXPORT GaborFitTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT GaborFitTargets NAMESPACE GaborFit:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GaborFit)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/GaborFitConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/GaborFitConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GaborFit)


