ECHO OFF

powershell -Command "clear"

ECHO Downloading GraalVM 20.1.0 ...
SET GRAALVM_URL=https://github.com/graalvm/graalvm-ce-builds/releases/download/vm-20.1.0/graalvm-ce-java11-windows-amd64-20.1.0.zip
powershell -Command "$ProgressPreference = 'SilentlyContinue'; Invoke-WebRequest %GRAALVM_URL% -OutFile GraalVM.zip"

ECHO Installing GraalVM 20.1.0 to C:\GraalVM\ ...
powershell -Command "$ProgressPreference = 'SilentlyContinue'; Expand-Archive -Path 'GraalVM.zip' -DestinationPath 'C:\GraalVM\'"
del GraalVM.zip
ECHO Installed GraalVM to C:\GraalVM\graalvm-ce-java11-20.1.0 !

ECHO Checking Java Installation ...
SET JAVA_HOME=C:\GraalVM\graalvm-ce-java11-20.1.0
%JAVA_HOME%\bin\java.exe -version

ECHO Installing GraalVM-20.1.0 Native-Image ...
%JAVA_HOME%\lib\installer\bin\gu.exe install native-image

rem SET OWL_VER=release-20.06.00
ECHO Installing OWL/%OWL_VER% ...
RMDIR /S /Q owl
rem git clone --depth=1 https://gitlab.lrz.de/i7/owl --branch %OWL_VER%
MKDIR owl
CD owl
tar -xf ..\owl-release-20.06.00.zip
gradlew.bat clean
gradlew.bat distZip -Pdisable-default
