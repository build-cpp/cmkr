# Third party dependencies (manually vendored to improve fetch speeds)
 
# https://github.com/ToruNiina/toml11 (MIT)
add_library(toml11 INTERFACE)
target_include_directories(toml11 INTERFACE toml11-3.8.1) 
