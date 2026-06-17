#ifndef LIB_HTTP_CPP
#define LIB_HTTP_CPP

#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <string>
#include <vector>

// ============================================================================
// ENVIRONMENT DETECTION & ABSTRACTION LAYER (Cross-platform magic)
// ============================================================================
#ifdef __EMSCRIPTEN__
    // 1. WebAssembly / Emscripten Environment
    #include <emscripten/fetch.h>
    #include <emscripten/emscripten.h> // Required for emscripten_sleep()
    #define FETCH_IMPLEMENTATION_EMSCRIPTEN
#else
    // 2. Native Desktop Environment (Windows, Linux, macOS)
    #include <curl/curl.h>
    #define FETCH_IMPLEMENTATION_LIBCURL
#endif

namespace AutoLang {
class ACompiler;

namespace Libs {
namespace http {

#ifdef FETCH_IMPLEMENTATION_LIBCURL
/**
 * Standard libcurl write callback (Desktop Only).
 * Appends raw incoming network byte streams dynamically into a C++ std::string container.
 */
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
#endif

// ============================================================================
// 1. Fetch a single URL (Synchronously blocks VM thread, returns String)
// ============================================================================
inline AObject *get(NativeFuncInData) {
    // ZERO-ALLOCATION OPTIMIZATION: Extract direct char* raw pointer from AutoLang VM.
    const char* url = args[0]->str->data;
    
    // Extract dynamic timeout provided by the Autolang user (in milliseconds)
    long timeout_ms = static_cast<long>(args[1]->i);

#ifdef FETCH_IMPLEMENTATION_LIBCURL
    // --- NATIVE DESKTOP IMPLEMENTATION (LIBCURL) ---
    CURL* curl = curl_easy_init();
    if (!curl) {
        notifier.throwException("Failed to initialize libcurl");
        return nullptr;
    }

    std::string response_string;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    // Apply dynamic timeout constraints in MILLISECONDS
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::string error_msg = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        notifier.throwException("HTTP Request failed | Error: " + error_msg);
        return nullptr;
    }

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    if (response_code >= 400) {
        notifier.throwException("HTTP Error status code: " + std::to_string(response_code));
        return nullptr;
    }

    return notifier.createString(response_string);

#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
    // --- WEBASSEMBLY IMPLEMENTATION (EMSCRIPTEN FETCH) ---
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    
    // Apply dynamic timeout constraints directly in milliseconds
    attr.timeoutMSecs = timeout_ms;

    // Block the WebAssembly thread safely until the browser resolves the fetch
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;

    emscripten_fetch_t *fetch = emscripten_fetch(&attr, url);

    if (fetch->status == 200) {
        std::string result(fetch->data, fetch->numBytes);
        emscripten_fetch_close(fetch); // Free unmanaged memory
        return notifier.createString(result);
    } else {
        std::string err_msg = "HTTP Request failed with status: " + std::to_string(fetch->status);
        emscripten_fetch_close(fetch);
        notifier.throwException(err_msg);
        return nullptr;
    }
#endif
}

// ============================================================================
// 2. Fetch multiple URLs concurrently (Returns Array<String>)
// ============================================================================
inline AObject *get_all(NativeFuncInData) {
    AObject *arrayObj = args[0];
    ClassId arrayClassId = args[1]->i;
    
    // Extract dynamic timeout provided by the Autolang user (in milliseconds)
    long timeout_ms = static_cast<long>(args[2]->i);

    size_t url_count = notifier.getArraySize(arrayObj);
    if (url_count == 0) {
        return notifier.createArray(arrayClassId);
    }

#ifdef FETCH_IMPLEMENTATION_LIBCURL
    // --- NATIVE DESKTOP IMPLEMENTATION (MULTI-HANDLE MULTIPLEXING) ---
    CURLM* multi_handle = curl_multi_init();
    std::vector<CURL*> easy_handles(url_count);
    std::vector<std::string> response_strings(url_count);

    for (size_t i = 0; i < url_count; ++i) {
        const char* url = arrayObj->member->data[i]->str->data;
        easy_handles[i] = curl_easy_init();
        
        curl_easy_setopt(easy_handles[i], CURLOPT_URL, url);
        curl_easy_setopt(easy_handles[i], CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(easy_handles[i], CURLOPT_WRITEDATA, &response_strings[i]);
        curl_easy_setopt(easy_handles[i], CURLOPT_FOLLOWLOCATION, 1L);
        
        // Apply dynamic timeout constraints in MILLISECONDS for each concurrent request
        curl_easy_setopt(easy_handles[i], CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
        curl_easy_setopt(easy_handles[i], CURLOPT_TIMEOUT_MS, timeout_ms);
        
        curl_multi_add_handle(multi_handle, easy_handles[i]);
    }

    int still_running = 0;
    do {
        CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
        if (mc == CURLM_OK && still_running) {
            curl_multi_poll(multi_handle, nullptr, 0, 1000, nullptr); // Non-blocking poll
        }
    } while (still_running);

    auto newArr = notifier.createArray(arrayClassId);

    for (size_t i = 0; i < url_count; ++i) {
        long response_code = 0;
        curl_easy_getinfo(easy_handles[i], CURLINFO_RESPONSE_CODE, &response_code);
        curl_multi_remove_handle(multi_handle, easy_handles[i]);
        curl_easy_cleanup(easy_handles[i]);

        if (response_code == 200) {
            notifier.arrayAdd(newArr, notifier.createString(response_strings[i]));
        } else {
            notifier.arrayAdd(newArr, notifier.createString("")); 
        }
    }

    curl_multi_cleanup(multi_handle);
    return newArr;

#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
    // --- WEBASSEMBLY IMPLEMENTATION (ASYNC LOOP + ASYNCIFY SLEEP) ---
    std::vector<emscripten_fetch_t*> fetches(url_count);

    // 1. Dispatch all network requests concurrently in the browser background
    for (size_t i = 0; i < url_count; ++i) {
        const char* url = arrayObj->member->data[i]->str->data;
        
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY; // Asynchronous by default
        
        // Apply dynamic timeout constraints directly in milliseconds
        attr.timeoutMSecs = timeout_ms;

        fetches[i] = emscripten_fetch(&attr, url);
    }

    // 2. Block the Autolang VM while keeping the JS Event Loop alive
    bool all_done = false;
    while (!all_done) {
        all_done = true;
        for (size_t i = 0; i < url_count; ++i) {
            // Emscripten marks completion when status/numBytes are populated
            if (fetches[i]->numBytes == 0 && fetches[i]->status == 0) {
                all_done = false; 
                break;
            }
        }

        if (!all_done) {
            // CRITICAL: Yields the thread back to the browser for 10ms.
            // REQUIRES `-sASYNCIFY` flag during Emscripten compilation.
            emscripten_sleep(10); 
        }
    }

    // 3. Package results and free memory manually (No GC)
    auto newArr = notifier.createArray(arrayClassId);
    
    for (size_t i = 0; i < url_count; ++i) {
        if (fetches[i]->status == 200) {
            std::string res(fetches[i]->data, fetches[i]->numBytes);
            notifier.arrayAdd(newArr, notifier.createString(res));
        } else {
            notifier.arrayAdd(newArr, notifier.createString("")); 
        }
        emscripten_fetch_close(fetches[i]); // Clean up browser-allocated buffers
    }

    return newArr;
#endif
}

// ============================================================================
// 3. Register Built-In Library Bindings with the Compiler Subsystem
// ============================================================================
void init(ACompiler &compiler) {
    compiler.registerBuiltInLibrary("std/http", R"###(
@no_constructor
@no_extends
class Http {
    
    // Fetch a single target URL synchronously (Default timeout: 10000ms)
    @native("http_get")
    static func get(url: String, timeoutMs: Int = 10000): String

    // Internal Native binding for fetching multiple URLs
    @native("http_get_all")
    private static func _getAll(urls: Array<String>, arrayClassId: Int, timeoutMs: Int): Array<String>
    
    // Fetch an array of target URLs concurrently (Default timeout: 10000ms)
    static func getAll(urls: Array<String>, timeoutMs: Int = 10000): Array<String> = _getAll(urls, getClassId(Array<String>), timeoutMs)
}
        )###",
        LibraryConfig(),
        ANativeMap({
            {"http_get", &http::get},
            {"http_get_all", &http::get_all},
        }));
}

} // namespace http
} // namespace Libs
} // namespace AutoLang
#endif