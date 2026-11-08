@ECHO OFF
setlocal enabledelayedexpansion

pushd "%~dp0"
set "SCRIPT_DIR=%CD%"
if not defined PFACES_DEVICE_ID set "PFACES_DEVICE_ID=1"

where pfaces >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: 'pfaces' is not on PATH.
    popd
    exit /b 1
)

call :run_case "examples\robot2d" "robot.cfg" "robot.mdf" "robot.symmodel"
if %ERRORLEVEL% NEQ 0 (
    popd
    exit /b 1
)

call :run_case "examples\pickupdelivery" "pickupdelivery.cfg" "pickupdelivery.mdf" ""
if %ERRORLEVEL% NEQ 0 (
    popd
    exit /b 1
)

echo All OmegaThreads E2E tests passed.
popd
exit /b 0

:run_case
set "EX_DIR=%~1"
set "CFG_FILE=%~2"
set "OUT1=%~3"
set "OUT2=%~4"
set "FULL_DIR=%SCRIPT_DIR%\%EX_DIR%"
set "LOG_FILE=%TEMP%\pfaces_omega_test_%RANDOM%.log"

echo === Running OmegaThreads test: %EX_DIR%\%CFG_FILE% ^(device %PFACES_DEVICE_ID%^) ===
if not "%OUT1%"=="" if exist "%FULL_DIR%\%OUT1%" del /f /q "%FULL_DIR%\%OUT1%"
if not "%OUT2%"=="" if exist "%FULL_DIR%\%OUT2%" del /f /q "%FULL_DIR%\%OUT2%"

pushd "%FULL_DIR%"
pfaces -CG -d %PFACES_DEVICE_ID% -k omega@..\..\kernel-pack -cfg "%CFG_FILE%" > "%LOG_FILE%" 2>&1
set "PF_ERR=!ERRORLEVEL!"
popd

type "%LOG_FILE%"
if !PF_ERR! NEQ 0 (
    del /f /q "%LOG_FILE%" 2>nul
    echo Error: pfaces failed for %EX_DIR%\%CFG_FILE%.
    exit /b 1
)

findstr /C:"A controller is found !" "%LOG_FILE%" >nul
if !ERRORLEVEL! NEQ 0 (
    del /f /q "%LOG_FILE%" 2>nul
    echo Error: expected 'A controller is found !' in output for %EX_DIR%\%CFG_FILE%.
    exit /b 1
)
del /f /q "%LOG_FILE%" 2>nul

if not "%OUT1%"=="" (
    if not exist "%FULL_DIR%\%OUT1%" (
        echo Error: missing output file %EX_DIR%\%OUT1%.
        exit /b 1
    )
    for %%A in ("%FULL_DIR%\%OUT1%") do if %%~zA LEQ 0 (
        echo Error: empty output file %EX_DIR%\%OUT1%.
        exit /b 1
    )
)
if not "%OUT2%"=="" (
    if not exist "%FULL_DIR%\%OUT2%" (
        echo Error: missing output file %EX_DIR%\%OUT2%.
        exit /b 1
    )
    for %%A in ("%FULL_DIR%\%OUT2%") do if %%~zA LEQ 0 (
        echo Error: empty output file %EX_DIR%\%OUT2%.
        exit /b 1
    )
)
exit /b 0
