g++ -std=c++23 -O2 -Wall -Ilibs/include -Llibs/lib ` src\*.cpp src\entities\*.cpp src\ingame\*.cpp ` -lSDL3 -lSDL3_image -lSDL3_ttf -o build\mummymaze.exe
build\mummymaze.exe
