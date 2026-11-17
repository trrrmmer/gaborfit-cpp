cmake_minimum_required(VERSION 3.18)
project(gaborfit VERSION 0.2.0 LANGUAGES C CXX)

option(GABORFIT_BUILD_SHARED "Build shared C ABI library" ON)
option(GABORFIT_BUILD_TESTS "Build native tests" ON)
option(GABORFIT_ENABLE_CERES "Prefer Ceres when an installation is available" ON)
option(GABORFIT_USE_SYSTEM_EIGEN "Use an installed Eigen 3.4.0 instead of the pinned archive" OFF)
option(GABORFIT_BUILD_EEG_CLEANER "Build the complete EEG muscle-artifact cleaner" ON)
option(GABORFIT_BUILD_EXAMPLES "Build command-line examples" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(GABORFIT_USE_SYSTEM_EIGEN)
  find_package(Eigen3 3.4.0 EXACT REQUIRED NO_MODULE)
  set(GABORFIT_EIGEN_TARGET Eigen3::Eigen)
  message(STATUS "Using explicitly requested system Eigen ${Eigen3_VERSION}")
else()
  include(FetchContent)
  FetchContent_Declare(gaborfit_eigen
    URL
      https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
      https://huggingface.co/csukuangfj/sherpa-onnx-cmake-deps/resolve/27996728725ac6303bf3525099cb36d19dd09677/eigen-3.4.0.tar.gz
    URL_HASH SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72)
  FetchContent_GetProperties(gaborfit_eigen)
  if(NOT gaborfit_eigen_POPULATED)
    FetchContent_Populate(gaborfit_eigen)
  endif()
  add_library(gaborfit_eigen_headers INTERFACE)
  target_include_directories(gaborfit_eigen_headers INTERFACE ${gaborfit_eigen_SOURCE_DIR})
  set(GABORFIT_EIGEN_TARGET gaborfit_eigen_headers)
  message(STATUS "Using pinned Eigen 3.4.0 archive (${gaborfit_eigen_SOURCE_DIR})")
endif()

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
target_link_libraries(gaborfit PRIVATE ${GABORFIT_EIGEN_TARGET})
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

if(GABORFIT_BUILD_EEG_CLEANER)
  add_library(gaborfit_eeg SHARED src/eeg_clean.cpp src/eeg_clean_c.cpp)
  add_library(GaborFit::eeg_clean ALIAS gaborfit_eeg)
  set_target_properties(gaborfit_eeg PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME eeg_clean
    WINDOWS_EXPORT_ALL_SYMBOLS ON)
  target_include_directories(gaborfit_eeg PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
  target_link_libraries(gaborfit_eeg PUBLIC GaborFit::gaborfit)
  target_compile_definitions(gaborfit_eeg PRIVATE GABORFIT_EEG_BUILDING_LIBRARY)
  if(MSVC)
    target_compile_options(gaborfit_eeg PRIVATE /W4 /permissive-)
  else()
    target_compile_options(gaborfit_eeg PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endif()

if(GABORFIT_BUILD_EXAMPLES AND GABORFIT_BUILD_EEG_CLEANER)
  add_executable(gaborfit_clean_example examples/clean_synthetic.cpp)
  target_link_libraries(gaborfit_clean_example PRIVATE GaborFit::eeg_clean)
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
  add_executable(gaborfit_eigen_version_test tests/test_eigen_version.cpp)
  target_link_libraries(gaborfit_eigen_version_test PRIVATE ${GABORFIT_EIGEN_TARGET})
  add_test(NAME gaborfit_eigen_version_test COMMAND gaborfit_eigen_version_test)
  if(GABORFIT_BUILD_EEG_CLEANER)
    add_executable(gaborfit_eeg_tests tests/test_eeg_clean.cpp)
    target_link_libraries(gaborfit_eeg_tests PRIVATE GaborFit::eeg_clean)
    add_test(NAME gaborfit_eeg_tests COMMAND gaborfit_eeg_tests)
  endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/GaborFitConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/GaborFitConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GaborFit)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/GaborFitConfigVersion.cmake
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
set(GABORFIT_INSTALL_TARGETS gaborfit)
if(GABORFIT_BUILD_EEG_CLEANER)
  list(APPEND GABORFIT_INSTALL_TARGETS gaborfit_eeg)
endif()
install(TARGETS ${GABORFIT_INSTALL_TARGETS} EXPORT GaborFitTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT GaborFitTargets NAMESPACE GaborFit:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GaborFit)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/GaborFitConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/GaborFitConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GaborFit)


