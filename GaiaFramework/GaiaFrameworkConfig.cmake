cmake_minimum_required(3.10)

find_path(GaiaFramework_INCLUDE_DIRS "GaiaFramework")
find_library(GaiaFramework_LIBS "GaiaFramework")

if (GaiaFramework_INCLUDE_DIRS AND GaiaFramework_LIBS)
    set(GaiaFramework_FOUND TRUE)
endif()