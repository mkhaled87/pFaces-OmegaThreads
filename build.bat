@ECHO OFF
setlocal enabledelayedexpansion

pushd "%~dp0"
set "SCRIPT_DIR=%CD%"

set "BUILD_TYPE=Release"
set "CLEAN_BUILD=True"
set "KERNEL_NAME=omega"
set "VS_VERSION=Visual Studio 17 2022"
set "BUILD_DEF=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%"

if not defined PFACES_SDK_ROOT (
    echo Error: PFACES_SDK_ROOT is not set.
    popd
    exit /b 1
)

rem Install Owl native library if missing
set "OWL_LIB_DIR=%SCRIPT_DIR%\kernel-driver\lib\ltl2dpa\owl\build\native-library"
if not exist "%OWL_LIB_DIR%\libowl.h" (
    call "%SCRIPT_DIR%\kernel-driver\lib\ltl2dpa\install-owl.bat"
    if !ERRORLEVEL! NEQ 0 (
        popd
        exit /b 1
    )
) else if not exist "%OWL_LIB_DIR%\libowl.dll" (
    call "%SCRIPT_DIR%\kernel-driver\lib\ltl2dpa\install-owl.bat"
    if !ERRORLEVEL! NEQ 0 (
        popd
        exit /b 1
    )
)

rem Remove any old build
if /I "%CLEAN_BUILD%"=="True" (
    if exist "%SCRIPT_DIR%\kernel-pack\%KERNEL_NAME%.driver" del /f /q "%SCRIPT_DIR%\kernel-pack\%KERNEL_NAME%.driver"
    if exist "%SCRIPT_DIR%\build" rmdir /S /Q "%SCRIPT_DIR%\build"
)

rem Building ....
mkdir "%SCRIPT_DIR%\build"
cd /d "%SCRIPT_DIR%\build"
cmake .. -Wno-dev -Wno-deprecated %BUILD_DEF% -G "%VS_VERSION%" -A x64 -DKERNEL=%KERNEL_NAME%
if %ERRORLEVEL% NEQ 0 (
    popd
    exit /b 1
)
cmake --build . --config %BUILD_TYPE%
if %ERRORLEVEL% NEQ 0 (
    popd
    exit /b 1
)

if not exist "%SCRIPT_DIR%\kernel-pack\%KERNEL_NAME%.driver" (
    echo Error: kernel-pack\%KERNEL_NAME%.driver was not produced.
    popd
    exit /b 1
)
if not exist "%SCRIPT_DIR%\kernel-pack\libowl.dll" (
    echo Error: kernel-pack\libowl.dll was not produced.
    popd
    exit /b 1
)

echo Build complete: kernel-pack\%KERNEL_NAME%.driver and kernel-pack\libowl.dll
popd
exit /b 0
