---
trigger: always_on
description: Quy chuẩn kiến trúc, sửa lỗi và tối ưu hóa mã nguồn AutoLangC
---

# Quy chuẩn kiến trúc và tối ưu hóa mã nguồn AutoLangC

Tài liệu này định nghĩa các nguyên tắc bắt buộc khi bảo trì, sửa lỗi và mở rộng mã nguồn dự án AutoLangC.

---

## 1. Phân tích cú pháp và xử lý mã thông báo (Lexer & Parser)
1. **Không so sánh chuỗi trực tiếp**: Tuyệt đối không dùng toán tử so sánh chuỗi (ví dụ `context.lexerString[...] == "..."`) trong bộ phân tích cú pháp để kiểm tra từ khóa hoặc chú thích.
2. **Khai báo mã thông báo chuẩn tắc**: 
   - Mọi từ khóa hoặc cú pháp chú thích mới bắt buộc phải được khai báo trong `TokenType` ([Lexer.hpp](file:///d:/code/AutoLangC/src/frontend/lexer/Lexer.hpp)), ánh xạ tại bảng `KEYWORDS` ([Lexer.cpp](file:///d:/code/AutoLangC/src/frontend/lexer/Lexer.cpp)) và bảng `CAST` ([Token.cpp](file:///d:/code/AutoLangC/src/frontend/lexer/Token.cpp)).
   - Tại bộ phân tích cú pháp, chỉ phân nhánh dựa trên `token->type` (ví dụ `case Lexer::TokenType::DESCRIPTION:`).
3. **Từ khóa theo ngữ cảnh (Contextual Keywords)**:
   - Đối với các từ khóa có thể trùng tên với biến hoặc thuộc tính (ví dụ `description`, `to`), bộ tách từ phải kiểm tra ngữ cảnh tại hàm `pushIdentifier` ([Lexer.cpp](file:///d:/code/AutoLangC/src/frontend/lexer/Lexer.cpp)). Chỉ tạo mã thông báo từ khóa khi thỏa mãn ngữ cảnh (ví dụ đứng ngay sau `@`), tránh biến từ đó thành từ khóa bảo lưu làm gãy cú pháp khai báo thông thường.
4. **Định danh cố định**: Nếu cần so khớp tên định danh nội bộ không phải từ khóa, dùng các hằng số chỉ số định danh (`lexerId...`) trong [ParserContext.hpp](file:///d:/code/AutoLangC/src/frontend/parser/ParserContext.hpp) thay vì kiểm tra chuỗi ký tự.

---

## 2. Quản lý chuỗi và tối ưu hóa bộ nhớ
1. **Lưu trữ chuỗi bất biến**:
   - Tận dụng `StringArena` và `std::string_view` để lưu trữ và tham chiếu các chuỗi bất biến, tránh việc tạo bản sao chuỗi mới liên tục gây phân mảnh bộ nhớ.
   - Khi lưu trữ chuỗi vào cấu trúc hàm hoặc lớp, ưu tiên lưu chỉ số độ lệch vùng nhớ chuỗi (`StringArenaOffset`) thay vì lưu trữ cả chuỗi ký tự.
2. **Tạo lập đối tượng thời gian chạy**:
   - Luôn sử dụng `ANotifier` để tạo các đối tượng (`createInt`, `createString`, `createMemberObject`), bảo đảm quản lý đúng vòng đời tham chiếu và cơ chế thu gom rác.
3. **Cấu trúc dữ liệu hiệu năng cao**:
   - Ưu tiên sử dụng bảng băm tối ưu `ankerl::unordered_dense` (`HashMap`) và mảng kích thước nhỏ `SmallVector` đã được tích hợp sẵn thay vì các cấu trúc thư viện chuẩn phân bổ bộ nhớ liên tục.
   - Nút cú pháp và các cấu trúc trung gian của trình biên dịch phải được cấp phát thông qua các vùng nhớ khối chuyên dụng (`ChunkArena`, `NonReallocatePool`).

---

## 3. Cơ chế chẩn đoán và thông báo lỗi
1. **Không làm loãng ngữ cảnh thông báo**:
   - Khi phát sinh lỗi không tìm thấy thuộc tính hoặc hàm, tuyệt đối không xuất toàn bộ danh sách thành viên hay hàm của lớp.
   - Chỉ sử dụng thuật toán đo độ tương đồng tên gọi (`rapidfuzz`) với ngưỡng phù hợp để đưa ra gợi ý tên gọi khả dĩ nhất (`Did you mean`).
2. **Tích hợp thông tin mô tả tính năng**:
   - Đối với các hàm có chú thích `@description`, luôn trích xuất và hiển thị mô tả ngắn gọn đi kèm trong gợi ý sửa lỗi hoặc danh sách các hàm nạp chồng phù hợp để hỗ trợ ngữ cảnh làm việc.

---

## 4. An toàn kiểu dữ liệu và tính toàn vẹn
1. **Kiểm tra khả năng nhận giá trị rỗng**:
   - Luôn kiểm tra cờ khả năng nhận giá trị rỗng (`allGenericTypeNullable`, `allMemberNullable`) trước khi trích xuất con trỏ đối tượng.
2. **Bảo tồn tài liệu và chú thích**:
   - Tuyệt đối không tự ý xóa bỏ các dòng chú thích giải thích logic hiện có trong mã nguồn khi chỉnh sửa.

---

## 5. Quy trình xác thực và kiểm thử
1. **Quy trình biên dịch bắt buộc**:
   - Sau khi hoàn thành bất kỳ thay đổi nào, phải thực hiện biên dịch lại với cấu hình Release: `cmake --build build --config Release`.
2. **Kiểm thử tự động**:
   - Bắt buộc chạy toàn bộ bộ kiểm thử của dự án: `.\build\autolang.exe`. Toàn bộ các trường hợp kiểm thử phải vượt qua thành công trước khi kết thúc công việc.
