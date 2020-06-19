include(ExternalProject)
find_package(Git REQUIRED)
find_package(Threads REQUIRED)

# Get googletest
ExternalProject_Add(
        gtest_src
        PREFIX "extern/gtest"
        GIT_REPOSITORY "https://github.com/google/googletest.git"
        GIT_TAG "release-1.10.0"
        TIMEOUT 10
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/extern/gtest
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        UPDATE_COMMAND ""
)

# Prepare gtest
ExternalProject_Get_Property(gtest_src install_dir)
set(GTEST_INCLUDE_DIR ${install_dir}/include)
set(GTEST_LIBRARY_PATH ${install_dir}/lib/libgtest.a)
set(GTEST_MAIN_LIBRARY_PATH ${install_dir}/lib/libgtest_main.a)
add_library(gtest UNKNOWN IMPORTED)
add_library(gtest_main UNKNOWN IMPORTED)
set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARY_PATH})
set_property(TARGET gtest_main PROPERTY IMPORTED_LOCATION ${GTEST_MAIN_LIBRARY_PATH})

# Dependencies
add_dependencies(gtest gtest_src)
add_dependencies(gtest_main gtest_src)