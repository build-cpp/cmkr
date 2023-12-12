curl https://raw.githubusercontent.com/build-cpp/cmkr/main/cmake/cmkr.cmake -o cmkr.cmake
cmake -P cmkr.cmake
mkdir "libs"
mkdir "include"