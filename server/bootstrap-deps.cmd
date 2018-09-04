cd deps
md build 2> NUL
cd build
cmake -A x64 -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .