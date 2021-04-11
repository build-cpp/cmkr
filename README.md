# cmkr

cmkr, pronounced "cmaker", is A CMakeLists.txt generator from TOML.


## Building
cmkr requires a C++11 compiler, cmake >= 3.15.
```
git clone https://github.com/moalyousef/cmkr
cd cmkr
cmake -Bbin
cmake --build bin --parallel
```

## Usage
cmkr parses cmake.toml files (using toml11 by Toru Niina) at the project directory. A basic hello world format with the minimum required fields:
```toml
[cmake]
minimum = "3.15"

[project]
name = "app"
version = "0.1.0"

[[bin]]
name = "app"
type = "exe"
sources = ["src/main.cpp"]
```

This project's cmake.toml:
```toml
[cmake]
minimum = "3.15"

[project]
name = "cmkr"
version = "0.1.4"
description = "CMakeLists generator from TOML"

[fetch-content]
toml11 = { git = "https://github.com/ToruNiina/toml11" }
filesystem = { git = "https://github.com/gulrak/filesystem" }
mpark_variant = { url = "https://github.com/mpark/variant/archive/v1.4.0.tar.gz" }

[[bin]]
name = "cmkrlib"
type = "static"
sources = ["src/cmake.cpp", "src/gen.cpp", "src/help.cpp", "src/build.cpp", "src/error.cpp"]
include-dirs = ["include"]
features = ["cxx_std_11"]
link-libs = ["toml11::toml11", "ghc_filesystem"]

[[bin]]
name = "cmkr"
type = "exe"
sources = ["src/main.cpp", "src/args.cpp"]
link-libs = ["cmkrlib"]

[[install]]
targets = ["cmkr"]
destination = "${CMAKE_INSTALL_PREFIX}/bin"
```

Currently supported fields:
```toml
[cmake] # required for top-level project
minimum = "3.15" # required
description = "" # optional
subdirs = [] # optional
bin-dir = "bin" # optional
cpp-flags = [] # optional
c-flags = [] # optional
link-flags = [] # optional
generator = "Ninja" # optional, only valid when run using: cmkr build
config = "Release" # optional, only valid when run using: cmkr build
arguments = ["CMAKE_TOOLCHAIN_FILE=/path/to/toolchain"] # optional, valid when run using: cmkr build

[settings] # optional
CMAKE_BUILD_TYPE = "Release"
TOML_BUILD_TESTS = false # optional
TOML_BUILD_DOCS = { value = false, comment = "builds dependency docs", cache = true, force = true } # optional
OLD_VERSION = "0.1.1" # optional

[project] # required per project
name = "app" # required
version = "0.1.0" # required

[find-package] # optional, runs find_package, use "*" to ignore version
Boost = { version = "1.74.0", required = false, components = ["system"] } # optional
spdlog = "*"

[fetch-content] # optional, runs fetchContent
toml11 = { git = "https://github.com/ToruNiina/toml11", tag = "v3.5.0" } # optional

[options] # optional
APP_BUILD_STUFF = false # optional
APP_OTHER_STUFF = { comment = "does other stuff", value = false } # optional

[[bin]] # required, can define several binaries
name = "app" # required
type = "exe" # required (exe || lib || shared || static || interface)
sources = ["src/*.cpp"] # required, supports globbing
include-dirs = ["include"] # optional
alias = "" # optional
features = [] # optional
definitions = [] # optional
link-libs = [] # optional 
properties = { PROPERTY1 = "property1", ... } # optional

[[test]] # optional, can define several
name = "test1" # required
command = "app" # required
arguments = ["arg1", "arg2"] # optional

[[install]] # optional, can define several
targets = ["app"] # optional
files = ["include/*.h"] # optional
dirs = [] # optional
configs = [] # optional (Release|Debug...etc)
destination = "${CMAKE_INSTALL_PREFIX}/bin" # required
```

The cmkr executable can be run from the command-line:
```
Usage: cmkr [arguments]
arguments:
    init     [exe|lib|shared|static|interface]    Starts a new project in the same directory.
    gen                                           Generates CMakeLists.txt file.
    build    <extra cmake args>                   Run cmake and build.
    install                                       Run cmake --install. Needs admin privileges.
    clean                                         Clean the build directory.
    help                                          Show help.
    version                                       Current cmkr version.
```
The build command invokes cmake and the default build-system on your platform (unless a generator is specified), it also accepts extra cmake build arguments:
```
cmkr build --config Release 
```

## Binary types

### exe
Executable binary.

### lib
Library, can be static or shared depending on the BUILD_SHARED_LIBS variable.

### static
Static library/archive.

### shared
Shared/dynamic library.

### interface
Header-only library.

## Roadmap
- Support more cmake fields.
- Support conditional cmake args somehow!
