@echo off
if not exist build mkdir build
pushd build
cmake -G "Unix Makefiles" ..
popd
cmake --build build