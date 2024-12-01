@ECHO OFF

rem CMake settings for using Visual Studio (you may need to change the 
rem VS version with one from the list in 'cmake --help'. You may also need
rem point the VCPKG_PATH to a correct one where you have all needed libs
set BUILD_TYPE=Release
set KERNEL_NAME=gb_fp
set VS_VERSION="Visual Studio 17 2022"
set BUILD_DEF=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%
set VCPKG_PATH=C:/src/vcpkg
set VCPKG_TRIPLET=-DVCPKG_TARGET_TRIPLET=x64-windows

rem In case you need to use a custom OpenCL implementation, update this (OFF, or ON and fill values)
set CUSTOM_OPENCL=OFF
set CUSTOM_OPENCL_INC_DIR=%OCL_ROOT%\include
set CUSTOM_OPENCL_LIB_PATH=%OCL_ROOT%\lib\x86_64\OpenCL.lib

rem Prepare JAVA paths
SET JAVA_HOME=%cd:\=/%/kernel-driver/lib/ltl2dpa/GraalVM/graalvm-ce-java11-20.1.0

rem Remove any old build
IF NOT EXIST .\build GOTO BUILDING
  rmdir /S/Q .\build

rem Building ....
:BUILDING
set vcpkg=-DCMAKE_TOOLCHAIN_FILE=%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake
mkdir build
cd build
cmake .. -Wno-dev -Wno-deprecated %BUILD_DEF% %vcpkg% %VCPKG_TRIPLET% -G %VS_VERSION% -DKERNEL=%KERNEL_NAME% -DCUSTOM_OPENCL:BOOL=%CUSTOM_OPENCL% -DCUSTOM_OPENCL_INC_DIR="%CUSTOM_OPENCL_INC_DIR%" -DCUSTOM_OPENCL_LIB_PATH="%CUSTOM_OPENCL_LIB_PATH%" -DJAVA_HOME="%JAVA_HOME%"
cmake --build . --config %BUILD_TYPE%
cd ..
