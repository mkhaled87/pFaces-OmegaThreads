@ECHO OFF
setlocal enabledelayedexpansion

pushd "%~dp0"
set "SCRIPT_DIR=%CD%"

set "NIK_ARCH=amd64"
if /I "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
    echo Windows ARM64 detected; falling back to Windows x64 Liberica NIK archive ^(requires x64 emulation/MSVC x64 tools^).
)

set "NIK_FILE=bellsoft-liberica-vm-openjdk11.0.22+12-22.3.5+1-windows-%NIK_ARCH%.zip"
set "NIK_URL=https://github.com/bell-sw/LibericaNIK/releases/download/22.3.5%%2B1-11.0.22%%2B12/bellsoft-liberica-vm-openjdk11.0.22%%2B12-22.3.5%%2B1-windows-%NIK_ARCH%.zip"
set "INSTALL_PATH=%SCRIPT_DIR%\GraalVM"
set "GVM_SETUP_FILE=%SCRIPT_DIR%\gvm_name.txt"

if not exist "%INSTALL_PATH%" mkdir "%INSTALL_PATH%"

set "JAVA_HOME_DIR="
for /f "delims=" %%F in ('dir /b /s "%INSTALL_PATH%\java.exe" 2^>nul') do (
    for %%D in ("%%~dpF..") do set "JAVA_HOME_DIR=%%~fD"
    goto :found_java
)

:found_java
if not defined JAVA_HOME_DIR (
    echo Downloading BellSoft Liberica NIK 22.3.5 ^(OpenJDK 11^) for Windows...
    if exist "%INSTALL_PATH%\*" rmdir /S /Q "%INSTALL_PATH%"
    mkdir "%INSTALL_PATH%"

    where curl.exe >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        curl.exe -fsSL -o "%SCRIPT_DIR%\GraalVM.zip" "%NIK_URL%"
    ) else (
        powershell -NoProfile -Command "$ProgressPreference = 'SilentlyContinue'; Invoke-WebRequest -Uri '%NIK_URL%' -OutFile '%SCRIPT_DIR%\GraalVM.zip'"
    )
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to download Liberica NIK archive.
        popd
        exit /b 1
    )

    echo Extracting Liberica NIK to %INSTALL_PATH%...
    powershell -NoProfile -Command "$ProgressPreference = 'SilentlyContinue'; Expand-Archive -Path '%SCRIPT_DIR%\GraalVM.zip' -DestinationPath '%INSTALL_PATH%' -Force"
    if !ERRORLEVEL! NEQ 0 (
        del /f /q "%SCRIPT_DIR%\GraalVM.zip" 2>nul
        echo Failed to extract Liberica NIK archive.
        popd
        exit /b 1
    )
    del /f /q "%SCRIPT_DIR%\GraalVM.zip" 2>nul

    for /f "delims=" %%F in ('dir /b /s "%INSTALL_PATH%\java.exe" 2^>nul') do (
        for %%D in ("%%~dpF..") do set "JAVA_HOME_DIR=%%~fD"
        goto :found_java_after
    )
)

:found_java_after
if not exist "%JAVA_HOME_DIR%\bin\java.exe" (
    echo Could not locate java.exe under %INSTALL_PATH%.
    popd
    exit /b 1
)

for %%D in ("%JAVA_HOME_DIR%") do set "NIK_REL=%%~nxD"
> "%GVM_SETUP_FILE%" echo !NIK_REL!

set "JAVA_HOME=%JAVA_HOME_DIR%"
set "GRAAL_HOME=%JAVA_HOME_DIR%"
set "PATH=%JAVA_HOME%\bin;%PATH%"

"%JAVA_HOME%\bin\java.exe" -version
if %ERRORLEVEL% NEQ 0 (
    popd
    exit /b 1
)

if not exist "%JAVA_HOME%\bin\native-image.cmd" if not exist "%JAVA_HOME%\bin\native-image.exe" if not exist "%JAVA_HOME%\lib\svm\bin\native-image.exe" (
    echo Installing native-image component via gu...
    call "%JAVA_HOME%\bin\gu.cmd" install native-image
    if !ERRORLEVEL! NEQ 0 (
        popd
        exit /b 1
    )
)

if not defined VCToolsInstallDir (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" (
        echo Could not find vswhere.exe and MSVC developer environment is not initialized.
        popd
        exit /b 1
    )
    set "VS_INSTALL_DIR="
    for /f "usebackq delims=" %%I in (`"!VSWHERE!" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_INSTALL_DIR=%%I"
    )
    if not defined VS_INSTALL_DIR (
        echo Visual Studio with C++ x64 build tools was not found via vswhere.
        popd
        exit /b 1
    )
    call "!VS_INSTALL_DIR!\VC\Auxiliary\Build\vcvars64.bat"
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to initialize MSVC x64 environment.
        popd
        exit /b 1
    )
)

echo Extracting and building bundled Owl ^(release-20.06.00^)...
if exist "%SCRIPT_DIR%\owl" rmdir /S /Q "%SCRIPT_DIR%\owl"
mkdir "%SCRIPT_DIR%\owl"
cd /d "%SCRIPT_DIR%\owl"
tar -xf ..\owl-release-20.06.00.zip
if %ERRORLEVEL% NEQ 0 (
    powershell -NoProfile -Command "$ProgressPreference = 'SilentlyContinue'; Expand-Archive -Path '..\owl-release-20.06.00.zip' -DestinationPath '.' -Force"
    if !ERRORLEVEL! NEQ 0 (
        echo Failed to extract owl-release-20.06.00.zip.
        popd
        exit /b 1
    )
)

call gradlew.bat clean buildNativeLibrary -Pdisable-pandoc -Pdisable-default --no-daemon
if %ERRORLEVEL% NEQ 0 (
    echo Owl Gradle native library build failed.
    popd
    exit /b 1
)

if not exist "%SCRIPT_DIR%\owl\build\native-library\libowl.dll" (
    echo Expected libowl.dll was not produced.
    popd
    exit /b 1
)
if not exist "%SCRIPT_DIR%\owl\build\native-library\libowl.h" (
    echo Expected libowl.h was not produced.
    popd
    exit /b 1
)

if not exist "%SCRIPT_DIR%\..\..\..\..\kernel-pack" mkdir "%SCRIPT_DIR%\..\..\..\..\kernel-pack"
copy /Y "%SCRIPT_DIR%\owl\build\native-library\libowl.dll" "%SCRIPT_DIR%\..\..\..\..\kernel-pack\libowl.dll" >nul
if %ERRORLEVEL% NEQ 0 (
    echo Failed to copy libowl.dll into kernel-pack.
    popd
    exit /b 1
)

echo Owl native library build completed successfully.
popd
exit /b 0
