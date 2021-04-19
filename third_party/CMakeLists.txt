# Third party dependencies (manually vendored to improve fetch speeds)

# https://github.com/gulrak/filesystem (MIT)
add_library(ghc_filesystem INTERFACE)
target_include_directories(ghc_filesystem INTERFACE filesystem-1.5.2/include)

# https://github.com/Tessil/ordered-map (MIT)
add_library(ordered_map INTERFACE)
target_include_directories(ordered_map INTERFACE ordered-map-1.0.0/include)
target_compile_definitions(ordered_map INTERFACE NOMINMAX)

# https://github.com/ToruNiina/toml11 (MIT)
add_library(toml11 INTERFACE)
target_include_directories(toml11 INTERFACE toml11-3.6.0)

# https://github.com/mpark/variant (BSL-1.0)
add_library(mpark_variant INTERFACE)
target_include_directories(mpark_variant INTERFACE variant-1.4.0/include)
