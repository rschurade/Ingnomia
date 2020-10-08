REM run those commands from Visual Studio's Developer Command Prompt if you installed CMake with Visual Studio
cd /d "%~dp0"
mkdir content\tilesheet
copy /y "G:\C\Documents\Fichiers\Jeux\Steam\steamapps\common\Ingnomia\content\tilesheet" content\tilesheet
mkdir build
cd build
cmake .. -DQt5_DIR="C:\Qt\5.14.1\msvc2017_64\lib\cmake\Qt5" -DSTEAM_SDK_ROOT="..\3rdparty\steamworks_sdk_150" -DSTEAM="..\3rdparty\steamworks_sdk_150\redistributable_bin\win64\steam_api64.lib" -DNOESIS_ROOT="..\3rdparty\NoesisGUI-NativeSDK-win-3.0.6" -DNOESIS_APP="..\3rdparty\NoesisGUI-NativeSDK-win-3.0.6\Lib\windows_x86_64\NoesisApp.lib" -DNOESIS="..\3rdparty\NoesisGUI-NativeSDK-win-3.0.6\Lib\windows_x86_64\Noesis.lib" -DNOESIS_LICENSE_NAME="your name" -DNOESIS_LICENSE_KEY="qo+OZibHjma1aKPXaWZEyBGITPabJxhpqsIdSPRp61Z7gWS0"
Ingnomia.sln
