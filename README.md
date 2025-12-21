# MÊ CUNG TÂY DU – HÀNH TRÌNH TÔN NGỘ KHÔNG

## Giới thiệu

Đây là trò chơi mê cung 2D lấy cảm hứng từ *Tây Du Ký*, được phát triển bằng **C++23** và thư viện **SDL3**.
Người chơi điều khiển **Tôn Ngộ Không** di chuyển trong mê cung và tìm đường đến cổng thoát, đồng thời né tránh sự truy đuổi của các **quái vật** do AI điều khiển.

## Cách chơi

* Sử dụng **phím mũi tên** để di chuyển Tôn Ngộ Không.
* Mỗi lượt di chuyển của người chơi, quái vật có thể di chuyển tối đa **2 bước**.
* Mục tiêu là đến được **ô thoát** mà không bị quái vật bắt.

## Yêu cầu

* Compiler: GCC hỗ trợ tiêu chuẩn **C++23**
* Thư viện:

  * SDL3
  * SDL3_image
  * SDL3_ttf
* Hệ điều hành: Windows
  (các file DLL đã được cung cấp trong thư mục `libs/bin`)

## Hướng dẫn biên dịch và chạy

Mở **PowerShell** tại thư mục gốc của dự án và chạy:

```powershell
g++ -std=c++23 -O2 -Wall -Ilibs/include -Llibs/lib (Get-ChildItem src -Recurse -Filter *.cpp).FullName -lSDL3 -lSDL3_image -lSDL3_ttf -o build\mummymaze.exe; ./build\mummymaze.exe
```

Lưu ý: Đảm bảo thư mục `build/` đã tồn tại và chứa đầy đủ các file `.dll` cần thiết.

## Cấu trúc thư mục

* `src/`: Mã nguồn chương trình

  * `ai/`: Thuật toán điều khiển quái vật
  * `entities/`: Nhân vật Tôn Ngộ Không và quái vật
  * `screens/`: Các màn hình giao diện
  * `ingame/`: Logic gameplay
* `assets/`: Hình ảnh, âm thanh và bản đồ
* `libs/`: Thư viện SDL và các file liên quan
