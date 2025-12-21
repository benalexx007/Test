tasklist | findstr /I "mummymaze.exe" >nul
if %errorlevel%==0 (
    exit /b
)

powershell -NoProfile -Command ^
"g++ -std=c++23 -O2 -Wall -I libs/include -L libs/lib ^
(Get-ChildItem src -Recurse -Filter *.cpp).FullName ^
-lSDL3 -lSDL3_image -lSDL3_ttf -o build/mummymaze.exe; ^
./build/mummymaze.exe"