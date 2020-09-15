# cmkr

cmkr, pronounced "cmaker", is A CMakeLists.txt generator from TOML. Still WIP.

## Building
cmkr requires a C++17 compiler, cmake >= 3.14 and git. It depends on toml11 by ToruNiina, which is added as a git submodule.
```
git clone https://github.com/moalyousef/cmkr
cd cmkr
cmake -Bbin
cmake --build bin
```

## Usage
cmkr parses cmake.toml files (using toml11) at the project directory. A basic hello world format with the minimum required fields:
```toml
[cmake]
minimum = "3.14"

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
minimum = "3.14"

[project]
name = "cmkr"
version = "0.1.0"

[fetch-content]
toml11 = { GIT_REPOSITORY = "https://github.com/ToruNiina/toml11" }

[[bin]]
name = "cmkrlib"
type = "static"
sources = ["src/cmake.cpp", "src/gen.cpp", "src/help.cpp", "src/build.cpp", "src/error.cpp"]
include-dirs = ["include"]
features = ["cxx_std_17"]
link-libs = ["toml11::toml11"]

[[bin]]
name = "cmkr"
type = "exe"
sources = ["src/main.cpp", "src/args.cpp"]
link-libs = ["cmkrlib"]
```

Currently supported fields:
```toml
[cmake] # required for top-level project
minimum = "3.14" # required
subdirs = [] # optional
bin-dir = "bin" # optional
cpp-flags = [] # optional
c-flags = [] # optional
link-flags = [] # optional
generator = "Ninja" # optional
arguments = ["CMAKE_TOOLCHAIN_FILE=/path/to/toolchain"] # optional

[project] # required per project
name = "app" # required
version = "0.1.0" # required

[find-package] # optional, runs find_package, use "*" to ignore version
Boost = { version = "1.74.0", required = false, components = ["system"] } # optional
spdlog = "*"

[fetch-content] # optional, runs fetchContent
toml11 = { GIT_REPOSITORY = "https://github.com/ToruNiina/toml11", GIT_TAG = "v3.5.0" } # optional

[options] # optional
APP_BUILD_STUFF = false # optional
APP_OTHER_STUFF = { comment = "does other stuff", value = false } # optional

[[bin]] # required, can define several binaries
name = "app" # required
type = "exe" # required (exe || shared || static)
sources = ["src/*.cpp"] # required, supports globbing
include-dirs = ["include"] # optional
alias = "" # optional
features = [] # optional
defines = [] # optional
link-libs = [] # optional 
```

The cmkr executable can be run from the command-line:
```
Usage: cmkr [arguments]
arguments:
    init     [exe|lib|shared|static|interface]    Starts a new project in the same directory.
    gen                                           Generates CMakeLists.txt file.
    build    <extra cmake args>                   Run cmake and build.
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
Header only library.

## Roadmap
- Support more fields.
- Support conditional cmake args somehow!
