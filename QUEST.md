# 🗺️ QUEST.MD: LỘ TRÌNH NÂNG CẤP TOÀN DIỆN NGÔN NGỮ SONLANG (v1.0 -> v2.0)

> **Mục tiêu:** Chuyển hóa SonLang từ một ngôn ngữ dạng nguyên mẫu (prototype/concept) thành một **Ngôn ngữ Lập trình Hệ thống Độc lập, Hiệu năng cao, AI-Native và Đa dụng** có khả năng xây dựng các hệ thống quy mô lớn: Game Servers (như Pumpkin), Game Engines, Web Infrastructure, Databases, Network Proxies và Embedded AI Runtime.

---

## 🏛️ TỔNG QUAN KIẾN TRÚC MỤC TIÊU

```
+-----------------------------------------------------------------------------------+
|                        TẦNG 5: CÁC DỰ ÁN ỨNG DỤNG THỰC TẾ                         |
|  [Minecraft Server SCraft]  [High-Perf Web API]  [Game Engine 2D/3D]  [KV-Database]|
+-----------------------------------------------------------------------------------+
|                        TẦNG 4: THƯ VIỆN CHUẨN MỞ RỘNG (STDLIB)                    |
|   Async Net & Sockets  |  Dynamic Collections  |  Crypto & Zlib  |  File System   |
+-----------------------------------------------------------------------------------+
|                        TẦNG 3: RUNTIME & HỆ ĐIỀU HÀNH (PAL)                       |
|    C-FFI Subsystem     |   Dynamic Allocators  |   OS Threads   |  Hardware Atomics|
+-----------------------------------------------------------------------------------+
|                        TẦNG 2: BỘ BIÊN DỊCH CỐT LÕI (SON COMPILER)                |
|    Full AST Parser     |      Type Checker     | Borrow Checker |   LLVM Codegen  |
+-----------------------------------------------------------------------------------+
|                        TẦNG 1: GIAO TIẾP HẠ TẦNG PHẦN CỨNG                        |
|                     LLVM 18+ Backend  |  x86-64 / AArch64 Machine Code            |
+-----------------------------------------------------------------------------------+
```

---

## 📜 DANH SÁCH CHI TIẾT CÁC NHIỆM VỤ (QUESTS & EPICS)

### ⚔️ EPIC 1: HOÀN THIỆN LÕI BỘ BIÊN DỊCH (`compiler/`)
*Mục tiêu: Đưa SonLang trở thành ngôn ngữ có cú pháp hoàn chỉnh, tự phân tích ngữ pháp, kiểm tra kiểu và sinh mã LLVM IR thật sự cho mọi thuật toán phức tạp.*

- [ ] **Quest 1.1: Xây dựng Cây cú pháp trừu tượng hoàn chỉnh (`compiler/ast/`)**
  - [ ] Thiết kế `ExprAst`: Hỗ trợ biểu thức nhị phân (`BinaryExpr`), gọi hàm (`CallExpr`), truy xuất mảng (`IndexExpr`), truy xuất thành viên struct (`MemberAccessExpr`), toán tử gán và toán tử một ngôi.
  - [ ] Thiết kế `StmtAst`: Hỗ trợ khai báo biến (`VarDecl`, `ValDecl`), rẽ nhánh (`IfElseStmt`), vòng lặp (`WhileStmt`, `ForStmt`), khối lệnh (`BlockStmt`), lệnh thoát (`ReturnStmt`, `Break`, `Continue`).
  - [ ] Cài đặt thuật toán phân tích biểu thức ưu tiên toán tử (**Pratt Parser**) trong `compiler/parser/parser.sonl` thay thế cho bộ đếm token hiện tại.

- [ ] **Quest 1.2: Bổ sung Bộ kiểm tra kiểu dữ liệu tĩnh (`compiler/semantic/type_check.sonl`)**
  - [ ] Xây dựng Bảng ký hiệu theo phạm vi (`Scoped Symbol Table`): Quản lý biến cục bộ, biến toàn cục, tham số hàm.
  - [ ] Kiểm tra tính hợp lệ của kiểu dữ liệu: Ngăn chặn lỗi gán sai kiểu, ép kiểu tường minh với cú pháp `x as i64`, `ptr as usize`.
  - [ ] Suy diễn kiểu (Type Inference) cho các biểu thức khởi tạo `val x = 10;` và `val s = "hello";`.

- [ ] **Quest 1.3: Hiện thực hóa Bộ sinh mã LLVM IR động (`compiler/backend/llvm_emit.sonl`)**
  - [ ] Sinh mã LLVM cho luồng điều khiển: Sử dụng các nhãn nhảy `br label %then`, `br label %else`, phi-node `phi`.
  - [ ] Sinh mã truy xuất cấu trúc dữ liệu: Dùng lệnh `getelementptr` để đọc/ghi các trường của `struct` và phần tử mảng `[T]`.
  - [ ] Sinh mã gọi hàm động và hỗ trợ đệ quy: Sử dụng lệnh `call`.

- [ ] **Quest 1.4: Hỗ trợ Đơn hình hóa kiểu Generic (`Monomorphization`)**
  - [ ] Nhân bản mã nguồn cho các kiểu dữ liệu chung như `Vec[T]`, `Option[T]`, `Result[T, E]`.
  - [ ] Tự động sinh mã LLVM cụ thể cho từng kiểu dữ liệu thực tế: `Vec_i32`, `Vec_u8`, `Vec_Point`.

- [ ] **Quest 1.5: Từ khóa Dọn dẹp tài nguyên tự động (`defer`)**
  - [ ] Cú pháp: `defer <statement>;` (Ví dụ: `defer tcp_close(sock);`, `defer free(ptr);`).
  - [ ] Compiler tự động chèn các câu lệnh dọn dẹp vào trước mọi câu lệnh `return` hoặc trước khi luồng thực thi đi ra khỏi scope hiện tại.

---

### ⚔️ EPIC 2: HỆ THỐNG LIÊN KẾT THƯ VIỆN C (C-FFI SUBSYSTEM)
*Mục tiêu: Mở khóa khả năng tái sử dụng toàn bộ hệ sinh thái C/C++ có sẵn của thế giới (`WinSock`, `OpenSSL`, `zlib`, `mimalloc`, `SQLite`, `Raylib`).*

- [ ] **Quest 2.1: Bổ sung cú pháp khai báo ngoài (`extern "C"`)**
  - [ ] Hỗ trợ cú pháp:
    ```sonl
    extern "C" {
        fn socket(af: i32, type: i32, protocol: i32) -> i32;
        fn bind(fd: i32, addr: *u8, len: u32) -> i32;
        fn uncompress(dst: *mut u8, dst_len: *mut u64, src: *u8, src_len: u64) -> i32;
    }
    ```
- [ ] **Quest 2.2: Sinh mã LLVM Prototype (`declare`)**
  - [ ] Tự động sinh các dòng `declare <ret_type> @<name>(<param_types>)` vào đầu file LLVM IR output.
- [ ] **Quest 2.3: Tích hợp Linker Flags vào CLI `son`**
  - [ ] Hỗ trợ tham số liên kết thư viện khi compile: `son build app.sonl -lws2_32 -lssl -lcrypto -lz`.

---

### ⚔️ EPIC 3: BỘ NHỚ & RUNTIME HỆ THỐNG (`library/std/mem/` & `runtime/`)
*Mục tiêu: Cung cấp các công cụ quản lý bộ nhớ linh hoạt, tốc độ cao, không rò rỉ RAM.*

- [ ] **Quest 3.1: Bộ cấp phát bộ nhớ Heap động (`library/std/mem/heap.sonl`)**
  - [ ] Tích hợp syscall xin trang nhớ hệ thống: `VirtualAlloc`/`VirtualFree` (Windows) và `mmap`/`munmap` (Linux/macOS).
  - [ ] Cài đặt các hàm chuẩn:
    - `fn alloc(size: usize, align: usize) -> *mut u8`
    - `fn free(ptr: *mut u8, size: usize) -> void`
    - `fn realloc(ptr: *mut u8, old_size: usize, new_size: usize) -> *mut u8`
- [ ] **Quest 3.2: Nâng cấp Chained Block Arena Allocator (`library/std/mem/arena.sonl`)**
  - [ ] Chuyển đổi từ Arena mảng tĩnh thành Arena chuỗi khối nhớ (Chained Memory Blocks): tự động xin thêm khối nhớ mới khi đầy.
  - [ ] Hỗ trợ hàm dọn dẹp hàng loạt $O(1)$: `arena_reset()` giải phóng toàn bộ RAM chỉ trong 1 thao tác.
- [ ] **Quest 3.3: Bộ cấp phát kích thước cố định (`library/std/mem/pool.sonl`)**
  - [ ] Cài đặt `FixedPoolAllocator` với độ phức tạp cấp phát và thu hồi $O(1)$.
  - [ ] Tối ưu riêng cho Game Entities, Network Packets và Task Nodes.

---

### ⚔️ EPIC 4: BỘ CẤU TRÚC DỮ LIỆU ĐA DỤNG CHUẨN (`library/std/collections/`)
*Mục tiêu: Thay thế các cấu trúc dữ liệu giả lập bằng các container thực thụ, hiệu năng cao.*

- [ ] **Quest 4.1: Dynamic Vector tự mở rộng dung lượng (`library/std/collections/vec.sonl`)**
  - [ ] Struct chứa: `ptr: *mut T`, `len: usize`, `cap: usize`.
  - [ ] Cơ chế Exponential Growth: Khi `len == cap`, gọi `realloc` tăng gấp đôi dung lượng.
  - [ ] Cài đặt đầy đủ: `push`, `pop`, `insert`, `remove_swap` (xóa $O(1)$ cho game loop), `clear`.
- [ ] **Quest 4.2: Hash Map hiệu năng cao (`library/std/collections/hashmap.sonl`)**
  - [ ] Triển khai giải thuật **Robin Hood Hashing** hoặc **Flat Probing** lưu trữ cặp Key/Value thật sự.
  - [ ] Tích hợp thuật toán băm nhanh `Wyhash` hoặc `xxHash64`.
  - [ ] Cài đặt đầy đủ: `insert(key, value)`, `get(key) -> Option[V]`, `remove(key)`, `contains(key)`.
- [ ] **Quest 4.3: Hàng đợi tuần hoàn & Vòng đệm byte (`library/std/collections/ring.sonl`)**
  - [ ] Cài đặt `ByteBuffer`: Các hàm tiện ích `read_u8`, `read_i32_be`, `read_f64_le`, `write_bytes`.
  - [ ] Cài đặt `RingBuffer`: Phục vụ luồng dữ liệu mạng (Network Stream) và Audio/Packet buffering.

---

### ⚔️ EPIC 5: MẠNG HỆ ĐIỀU HÀNH, ĐA LUỒNG & PHẦN CỨNG (`library/std/os/` & `sync/`)
*Mục tiêu: Đưa SonLang vào thế giới các ứng dụng mạng thời gian thực, xử lý song song đa luồng.*

- [ ] **Quest 5.1: Network Sockets hệ điều hành thật (`library/std/os/net.sonl`)**
  - [ ] Tầng trừu tượng nền tảng (Platform Abstraction Layer - PAL):
    - **Windows:** Khởi tạo `WSAStartup`, gọi `WSASocketW`, `bind`, `listen`, `accept`, `send`, `recv`, `closesocket`.
    - **Linux/POSIX:** Gọi syscall `socket`, `bind`, `listen`, `accept`, `send`, `recv`, `close`.
  - [ ] Hỗ trợ cờ Non-blocking Socket (`O_NONBLOCK` / `FIONBIO`).
- [ ] **Quest 5.2: Bộ giám sát sự kiện I/O đa kênh (`library/std/os/poll.sonl`)**
  - [ ] Hỗ trợ cơ chế `select` hoặc `poll` đa nền tảng.
  - [ ] Xây dựng mô hình Event Loop đơn giản: 1 thread có thể lắng nghe hàng trăm kết nối socket mà không bị chặn.
- [ ] **Quest 5.3: Luồng thực thi OS & Khóa đồng bộ (`library/std/sync/`)**
  - [ ] Tạo luồng hệ điều hành thật: Gọi `CreateThread` (Windows) hoặc `pthread_create` (Linux).
  - [ ] Khóa Mutex và Điều kiện chờ (`Mutex`, `CondVar`).
- [ ] **Quest 5.4: Lệnh nguyên tử phần cứng thật (`library/std/sync/atomic.sonl`)**
  - [ ] Ánh xạ sang các LLVM Intrinsics: `atomicrmw add`, `cmpxchg`, `fence`.
  - [ ] Cài đặt chuẩn cho `AtomicI32`, `AtomicU64`, `AtomicBool`.

---

### ⚔️ EPIC 6: CÔNG CỤ NHỊ PHÂN & GIAO THỨC CHUYÊN SÂU (`library/std/protocol/`)
*Mục tiêu: Cung cấp nền tảng xử lý dữ liệu cho Minecraft, Game Protocols và Web APIs.*

- [ ] **Quest 6.1: Bộ chuyển đổi thứ tự Byte (Endianness)**
  - [ ] Cài đặt `to_be16`, `to_be32`, `to_be64` và `swap_bytes` (bắt buộc cho giao thức Minecraft Big-Endian).
- [ ] **Quest 6.2: Bộ mã hóa/giải mã số nguyên biến độ dài (VarInt & VarLong)**
  - [ ] Đọc/ghi số VarInt (1 - 5 bytes) từ byte buffer với thuật toán bitshift 7-bit.
- [ ] **Quest 6.3: Thư viện Mật mã & Nén dữ liệu qua FFI (`library/std/crypto/` & `compress/`)**
  - [ ] Binding với thư viện Zlib: `deflate` và `inflate` (nén packet, đọc file Chunk Anvil `.mca`).
  - [ ] Binding với thư viện Crypto: AES-128-CFB8, RSA-1024, SHA-1 (Mojang hex digest).
- [ ] **Quest 6.4: Bộ giải mã NBT (Named Binary Tag)**
  - [ ] Đọc và tuần tự hóa các thẻ NBT: Byte, Short, Int, Long, Float, Double, ByteArray, String, List, Compound.

---

## 🏆 CÁC CỘT MỐC THỬ NGHIỆM ĐÁNH GIÁ (MILESTONE SHOWCASES)

Mỗi khi hoàn thành một nhóm nhiệm vụ, các dự án mẫu sau đây sẽ được xây dựng để kiểm chứng thực tế:

| Cột mốc | Dự án Kiểm chứng | Yêu cầu kỹ thuật cần đạt | Trạng thái |
| :--- | :--- | :--- | :---: |
| **M1** | **Echo TCP Server** | Socket thật, đọc/ghi dữ liệu cổng TCP 8080 qua terminal `telnet`/`nc` | ⏳ Chờ Epic 2, 5 |
| **M2** | **SCraft: Minecraft Ping** | Cổng 25565, parse VarInt, trả về JSON Status Ping hiển thị trong game | ⏳ Chờ Epic 5, 6 |
| **M3** | **High-Perf HTTP/1.1 API** | Non-blocking socket, Router HashMap, trả về JSON với tốc độ 50k req/s | ⏳ Chờ Epic 4, 5 |
| **M4** | **Mini 2D Game (Raylib)** | Sử dụng C-FFI liên kết thư viện đồ họa Raylib, mở cửa sổ 60 FPS | ⏳ Chờ Epic 2 |
| **M5** | **SCraft: World & Player** | Đọc Chunk Anvil `.mca` (Zlib), AES encryption, cho phép người chơi vào game | ⏳ Chờ Toàn bộ |

---

## 📌 NGUYÊN TẮC THI CÔNG
1. **Ưu tiên số 1:** C-FFI (Epic 2) và Dynamic Heap Allocator (Epic 3). Hai phần này mở khóa 80% sức mạnh còn lại của SonLang.
2. **Không tái tạo bánh xe:** Sử dụng thư viện C có sẵn thông qua FFI cho những phần phức tạp (Zlib, OpenSSL, WinSock) trước khi tự viết phiên bản thuần SonLang.
3. **Kiểm thử liên tục:** Mỗi tính năng mới phải có file test độc lập trong `tests/` và chạy tự động qua `son.ps1 test`.
