# Kế hoạch: Hệ thống Error thân thiện với AI cho AutoLangC

Mục tiêu: biến error của AutoLang thành điểm mạnh cạnh tranh — deterministic, tiết kiệm token, máy đọc được (JSON), và hỗ trợ vòng lặp fix từng lỗi một cho LLM bị giới hạn output (Gemini).

File này dành cho agent khác thực hiện từng bước. Đọc kỹ AGENTS.md trước khi code (C++17, dùng ANotifier, header relative `src/`).

## 1. Hiện trạng đã khảo sát (đừng phá vỡ)

- `Lexer::LexerError { line, message }` trong `src/frontend/lexer/Lexer.hpp:165`. `ParserError : LexerError` trong `src/frontend/parser/Debugger.hpp:20`.
- Khoảng 416 điểm `throw ParserError` (25 file, nhiều nhất `DebuggerFunction.cpp:57`, `Debugger.cpp:44`, `DebuggerDeclaration.cpp:38`) + khoảng 35 điểm `throw LexerError` (xem `Lexer.cpp`).
- Message hiện tại đã tốt: nhiều chỗ có định dạng `"message\nHint: ..."` — đây là nền tảng để tách `message` / `hint`.
- `ParserContext::logError(line, msg)` (`ParserContext.cpp:536`) format thành `"path:line: message"`, lưu vào `lastErrorMessage` (chỉ giữ lỗi cuối), gọi `onError(string_view)` nếu có, ngược lại `cerr`.
- `ACompiler` đã có catch ở 4 chỗ:
  1. `loadSource` (`ACompiler.cpp:138`) — catch `ParserError` từ `loadImport`.
  2. `loadMainSource(LibraryData*)` (`ACompiler.cpp:319`) — tương tự.
  3. `generateBytecodes` (`ACompiler.cpp:468,471,923`) — catch từng dòng `loadLine`, có recovery (skip cùng line + cân bằng `{}`), cho phép thu nhiều lỗi nhưng chỉ phát qua `onError` lặp lại, không có danh sách cấu trúc.
  4. `Lexer::load` (`Lexer.cpp:121`) — catch theo từng dòng, skip tới cuối dòng, set `hasError`.
- Thiếu cho AI: không có error code ổn định, không có column/snippet, không có severity/phase, không có JSON, `getLastError()` chỉ trả 1 string cuối, không có chế độ `maxErrors=1` để fix từng lỗi (quan trọng cho Gemini giới hạn token output).

## 2. Nguyên tắc thiết kế (điểm mạnh để marketing)

1. **Deterministic + ổn định:** mỗi lỗi có `code` (ví dụ `E1203`) không đổi giữa các bản, để AI cache prompt/fix.
2. **Tiết kiệm token:** mặc định chỉ trả lỗi đầu tiên + gợi ý fix ngắn; muốn full thì xin thêm. Có chế độ `maxErrors=1` cho vòng lặp fix.
3. **Máy đọc được:** JSON diagnostics cho `autolang-compiler` (JS/TS) và pybind11, CLI `--format=json`.
4. **Actionable:** tách `message` / `hint` / `fix` (code mẫu sửa), kèm `phase` để AI biết lỗi lexer hay type.
5. **Tương thích ngược:** giữ `getLastError()`, `onError(string_view)`, `compile()` trả bool. Mọi API mới là thêm, không sửa signature cũ.

## 3. Data model đề xuất (Phase 1 — core)

Tạo file mới `src/frontend/parser/ADiagnostic.hpp` (C++17, không phụ thuộc VM):

```cpp
enum class ADiagPhase { LEXER, IMPORT, PARSE, RESOLVE, TYPE, CODEGEN, RUNTIME };
enum class ADiagSeverity { ERROR, WARNING };

struct ADiagnostic {
  std::string code;        // "E1203", "W0101", "L0007"
  ADiagPhase phase;
  ADiagSeverity severity = ADiagSeverity::ERROR;
  std::string path;        // LibraryData::path tại lúc lỗi
  uint32_t line = 0;       // 1-based, giữ như hiện tại
  uint32_t column = 0;     // Phase 4 mới có, trước mắt để 0
  std::string message;     // phần trước "\nHint:"
  std::string hint;        // phần sau "Hint:", có thể rỗng
  std::string snippet;     // Phase 4 mới có, trước mắt rỗng
};
```

- `ParserContext` thêm `std::vector<ADiagnostic> diagnostics; size_t maxDiagnostics = 32;`
- Thêm hàm:
  - `void logDiagnostic(ADiagnostic d)` — push vào vector (check cap + dedup theo code+line), đồng thời gọi `logError()` cũ để giữ behavior.
  - `const std::vector<ADiagnostic>& getDiagnostics() const`
  - `std::string getDiagnosticsJson() const` — build bằng tay (đừng thêm dependency JSON mới, dùng `nlohmann_json` đã có nếu cần, hoặc nối string có escape).
  - `void clearDiagnostics()` — gọi trong `refresh()`.
- `ACompiler` thêm:
  - `getDiagnostics()`, `getDiagnosticsJson()`, `setMaxDiagnostics(n)`, `setOneErrorMode(bool)` (= maxDiagnostics=1).
  - Giữ `getLastError()` như cũ (trả format `"path:line: message"` của lỗi cuối).

Tiêu chí nhận: build cũ vẫn qua, `tests/main.cpp --test` xanh.

## 4. Taxonomy mã lỗi (để chia việc audit 416 điểm)

Đừng đổi 416 chỗ một lúc. Trước hết gán code theo nhóm file:

- `L00xx` — Lexer (`Lexer.cpp`, ~35 điểm): bracket, string/quote, operator, comment, import token.
- `E10xx` — Syntax/parse dòng (`Debugger.cpp`, `DebuggerExpression.cpp`, `DebuggerConditionStatement.cpp`).
- `E12xx` — Declaration (`DebuggerDeclaration.cpp` 38 điểm, `DebuggerParameter.cpp` 19 điểm).
- `E13xx` — Function/closure (`DebuggerFunction.cpp` 57 điểm).
- `E14xx` — Class/enum/typealias/generics (`DebuggerClass.cpp` 33, `DebuggerEnum.cpp` 11, `DebuggerTypealias.cpp` 9, `DebuggerGenericClass.cpp` 6, `ClassInfo.cpp`, `ClassDeclaration.cpp`, `FunctionInfo.cpp`).
- `E15xx` — Statement khác (`DebuggerLoop.cpp` 24, `DebuggerWhen.cpp` 18, `DebuggerTryCatch.cpp` 13, `DebuggerLiteral.cpp` 36, `DebuggerAnnotations.cpp` 20, `DebuggerUtil.cpp` 5, `DebuggerImport.cpp` 5).
- `E16xx` — Resolve/optimize (`node/OptimizeNode.cpp` 22, `node/BinaryNode.cpp` 3, `node/Node.cpp` 1).

Viết script `tools/extract_diagnostics.py` (Python, chạy với `python -I` nếu đọc file không tin cậy — ở đây là source của mình nên chạy thường được):
- Regex `throw (ParserError|LexerError)\(([^,]+),\s*(.*?)\)` qua `src/`.
- Xuất `tools/diagnostics_inventory.json`: mỗi record `{file, line_no, phase_gợi_ý, raw_message, has_hint}`.
- Agent chia batch 20 record/lần để LLM (Gemini) gán `code + tách message/hint + viết fix mẫu`, đúng ý tưởng của user "duyệt từng lỗi vì giới hạn token".

## 5. Lộ trình thực hiện cho agent (5 Phase, mỗi Phase đều build + test được)

### Phase A — Khung diagnostics + tương thích ngược (ưu tiên cao nhất)
- Files: `src/frontend/parser/ADiagnostic.hpp` (mới), `ParserContext.hpp/.cpp` (thêm vector + hàm), `ACompiler.hpp` (thêm getter/setter inline).
- Sửa 5 điểm `catch` hiện tại để ngoài `logError` còn push `ADiagnostic` với code tạm `E0000`/`L0000` + phase đúng (LEXER/IMPORT/PARSE/CODEGEN).
- Tách `message/hint` tại điểm `logError`: nếu msg chứa `"\nHint:"` thì split, không thì hint rỗng.
- Thêm test mới `tests/rule/diagnostics/basic.atl` + assert trong `tests/main.cpp`: compile fail phải có `getDiagnostics().size()>=1` và `getDiagnosticsJson()` parse được.
- Build: `cmake -B build -S .` + `cmake --build build --config Release`, chạy `./build/autolang --test` (Windows: `build\Release\autolang.exe --test`).

### Phase B — Audit + gán mã cho 416 điểm (việc tốn token, chia batch)
- Chạy `tools/extract_diagnostics.py` để ra inventory.
- Mỗi batch 20 lỗi: đọc từng `throw`, gán code theo taxonomy mục 4, tách message/hint, đảm bảo hint cụ thể (có tên biến/type, có ví dụ đúng).
- Cách code để không sửa 416 chỗ tay: tạo helper trong `ADiagnostic.hpp`:
  ```cpp
  [[noreturn]] void throwDiag(uint32_t line, const char* code, ADiagPhase phase, std::string msg, std::string hint = "");
  ```
  `throwDiag` sẽ `throw ParserError(line, msg + (hint.empty()?"":"\nHint: "+hint))` — giữ format cũ nên mọi catch cũ vẫn chạy, đồng thời catch mới ở Phase A có thể parse lại code từ exception (cách đơn giản: mở rộng `ParserError` thêm field `code, phase` với default, constructor cũ giữ nguyên).
- Ưu tiên 3 file nhiều nhất trước: `DebuggerFunction.cpp`, `Debugger.cpp`, `DebuggerDeclaration.cpp` (~139/416). Mỗi PR nhỏ, test xanh mới merge batch tiếp.

### Phase C — Chế độ một-lỗi-một-lần cho Gemini (giá trị demo cao, code ít)
- `ParserContext::maxDiagnostics` + early-exit: khi đủ N lỗi thì `generateBytecodes` dừng thu thêm (vẫn set `CT_ERROR`).
- `ACompiler::setOneErrorMode(true)` để vòng lặp AI: compile → lấy `diagnostics[0]` → fix → `refresh()` → compile lại.
- Document vòng lặp mẫu `examples/ai-fix-loop.js` (cho `autolang-compiler`) và `examples/ai-fix-loop.py` (pybind11 tuân thủ quy tắc `self` đầu tiên trong AGENTS.md).
- Lưu ý binding WASM `release/wasm/main.cpp`: thêm `.function("getDiagnosticsJson", ...)` và `.function("setMaxDiagnostics", ...)`; binding Python tương tự.

### Phase D — Column + snippet (cần đụng Lexer, để sau cùng)
- Hiện `Token { line, indexData, type }` chưa có column; `Lexer::Context` cũng chỉ track `linePos/pos`. Thêm `column` vào `Token`, `LexerError`, `ADiagnostic`.
- Giữ `LibraryData::rawData` hoặc cache `vector<string> lines` để trích snippet 1-2 dòng quanh lỗi (hiện tại `rawData.clear()` sau lex nên mất nguồn).
- Rủi ro cao (đụng mọi parser dùng `token->line`), nên làm sau khi Phase A–C ổn định. Trước mắt `column=0, snippet=""` vẫn chấp nhận được cho AI.

### Phase E — CLI + docs thành điểm mạnh
- CLI `tests/main.cpp` (bản release dùng file này làm entry): thêm cờ `--format=json --max-errors=N --one-error` để CI/AI parse.
- Viết `docs/ERROR_CODES.md` (bảng code → ý nghĩa → fix mẫu) + cập nhật `llms.txt` (file đã có ở root) với section "AI error loop".
- Marketing copy: "AutoLang errors are LLM-first: stable codes, one-error mode, JSON diagnostics" — kèm benchmark token (đếm token trước/sau khi dùng one-error mode).

## 6. Thứ tự agent nên làm + Definition of Done

1. Phase A trước (1 agent, ~1-2 ngày): có vector diagnostics + JSON, test xanh.
2. Phase B song song theo file (chia 2-3 agent, mỗi agent 1 nhóm file, mỗi batch 20 throw): mỗi batch phải `cmake --build` + `--test` xanh.
3. Phase C (1 agent, sau A): one-error mode + 2 ví dụ JS/Python + binding WASM.
4. Phase D (để sau, cần senior review vì đụng Lexer).
5. Phase E (viết docs, không đụng code nguy hiểm).

DoD tổng: `compile()` sai vẫn trả `false` như cũ; `getLastError()` không đổi; `getDiagnosticsJson()` luôn là JSON hợp lệ; `--test` xanh; không tăng quá 5% thời gian compile trên `tests/correctness/main.atl`.

## 7. Rủi ro + cách tránh

- Sửa 416 throw một lúc dễ conflict — bắt buộc đi batch nhỏ, mỗi batch là 1 commit riêng, message kết thúc bằng `Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>` theo quy ước repo.
- Đừng đổi format string `"path:line: msg"` của `logError` — nhiều test/tool parse nó. Chỉ tách hint ở tầng diagnostics mới.
- Đừng thêm dependency mới — dùng `nlohmann_json` đã có nếu cần build JSON trong C++.
- Refresh phải clear diagnostics (không để lỗi cũ rò sang lần compile sau).
