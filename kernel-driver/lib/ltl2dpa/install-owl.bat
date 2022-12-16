@ECHO OFF

powershell -Command "clear"

ECHO Downloading GraalVM 20.1.0 ...
SET GRAALVM_URL=https://github.com/graalvm/graalvm-ce-builds/releases/download/vm-20.1.0/graalvm-ce-java11-windows-amd64-20.1.0.zip
powershell -Command "$ProgressPreference = 'SilentlyContinue'; Invoke-WebRequest %GRAALVM_URL% -OutFile GraalVM.zip"

ECHO Installing GraalVM 20.1.0 to .\GraalVM\ ...
powershell -Command "$ProgressPreference = 'SilentlyContinue'; Expand-Archive -Path 'GraalVM.zip' -DestinationPath '%cd%\GraalVM\'"
del GraalVM.zip
ECHO Installed GraalVM to %cd%\graalvm-ce-java11-20.1.0 !

ECHO Checking Java Installation ...
SET JAVA_HOME=%cd%\GraalVM\graalvm-ce-java11-20.1.0
%JAVA_HOME%\bin\java.exe -version

ECHO Installing GraalVM-20.1.0 Native-Image ...
%JAVA_HOME%\lib\installer\bin\gu.exe install native-image

ECHO Installing OWL from a local copy (owl-release-20.06.00.zip) ...
RMDIR /S /Q owl
MKDIR owl
CD owl
tar -xf ..\owl-release-20.06.00.zip
SET GRAAL_HOME=%JAVA_HOME%
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
call gradlew.bat clean
call gradlew.bat distZip -x javadoc -Pdisable-default -Pdisable-pandoc
cp .\build\native-library\libowl.dll ..\..\..\..\kernel-pack\libowl.dll