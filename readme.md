cmake --build . --config Release    # compile
.\Release\triarb.exe                # run the executable

# run tests
ctest -C Release -V                 # run the tests

# Full clean rebuild (if you change dependencies in cmakelist.txt)
cd ..
Remove-Item -Recurse -Force C:\Users\katka\source\single_exchange_triangular_arbitrage_bot\build
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/Users/katka/source/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release