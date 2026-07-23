# Standard Libraries (`src/frontend/libs`)

This directory implements the core standard library functions and APIs exposed to the Autolang programming language.

## Key Modules

- **`bytes.cpp` / `bytes.hpp`**: Operations on raw byte arrays.
- **`date.cpp` / `date.hpp`**: Calendar dates and date formatting utilities.
- **`file.cpp` / `file.hpp`**: File system read, write, and manipulation operations (sandboxed by VM permissions).
- **`http.cpp` / `http.hpp`**: HTTP client integration allowing external network requests.
- **`json.cpp` / `json.hpp`**: JSON parsing and serialization support.
- **`math.cpp` / `math.hpp`**: Standard mathematical functions.
- **`regex.cpp` / `regex.hpp`**: Regular expression matching and searching.
- **`stdlib.cpp` / `stdlib.hpp`**: Core language global utilities and standard functions.
- **`time.cpp` / `time.hpp`**: Execution timing, delays, and timestamp utilities.
- **`vm.cpp` / `vm.hpp`**: Standard VM-related meta-programming functions or controls.
- **`Debugger.cpp` / `Debugger.hpp`**: Debug logging utilities.
