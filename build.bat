ECHO OFF

rem CMake settings for using Visual Studio (you may only chnage the 
rem VS version with one from the list in 'cmake --help')
set BUILDTYPE=Release
set KERNEL_NAME=omega
set VSVERSION="Visual Studio 16 2019"
set BUILD_DEF=-DCMAKE_BUILD_TYPE=%BUILDTYPE%
set VCPKG_PATH=C:/Users/engmk/Workspace/Gitlab/vcpkg
set VCPKG_TRIPLET=-DVCPKG_TARGET_TRIPLET=x64-windows

rem Remove any old build
IF NOT EXIST .\build GOTO BUILDING
  rmdir /S/Q .\build

rem Building ....
:BUILDING
set vcpkg=-DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake
mkdir build
cd build
cmake .. -Wno-dev -Wno-deprecated %BUILD_DEF% %vcpkg% %VCPKG_TRIPLET% -G %VSVERSION%
cmake --build . --config %BUILDTYPE%
cd ..
