@echo off
echo 🔧 Building Mummy Maze...

g++ src\main.cpp src\game.cpp src\map\map.cpp ^
src\entities\character.cpp src\entities\explorer.cpp src\entities\mummy.cpp ^
 -Isrc -Isrc\map -Isrc\entities -Ilibs\include ^
 -Llibs\lib -lSDL3 -lSDL3_image ^
 -o build\mummymaze.exe

if %errorlevel%==0 (
    echo ✅ Build success! Output: build\mummymaze.exe
) else (
    echo ❌ Build failed!
)
pause
