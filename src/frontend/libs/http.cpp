#ifndef LIB_HTTP_CPP
#define LIB_HTTP_CPP

#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <regex>
#include <string>
#include <vector>

// ============================================================================
// ENVIRONMENT DETECTION & ABSTRACTION LAYER (Cross-platform magic)
// ============================================================================
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#define FETCH_IMPLEMENTATION_EMSCRIPTEN
#else
#include <curl/curl.h>
#define FETCH_IMPLEMENTATION_LIBCURL
#endif

namespace AutoLang {
class ACompiler;

namespace Libs {
namespace http {

// ============================================================================
// SECURITY LAYER: ZERO-TRUST DOMAIN VERIFICATION
// ============================================================================
inline bool checkUrlSecurity(const char *url, ANotifier &notifier) {
	std::string urlStr(url);

	// 1. Chặn ReDoS, tràn bộ đệm và mã độc bằng giới hạn độ dài
	if (urlStr.length() > 512)
		return false;

	// Nếu không có giấy phép (Null) -> Mặc định mọi cái đều vào được
	if (notifier.vm->allowedDomainsRegex == nullptr)
		return true;

	// 3. Bóc tách domain thô an toàn
	static const std::regex domain_regex(
	    R"(^(?:https?:\/\/)?(?:[^@\n]+@)?(?:www\.)?([^:\/\n\?]+))");
	std::smatch match;
	if (std::regex_search(urlStr, match, domain_regex) && match.size() > 1) {
		std::string targetDomain = match[1].str();
		// 4. Khớp với Regex đã compile sẵn của Host
		return std::regex_match(targetDomain,
		                        *(notifier.vm->allowedDomainsRegex));
	}

	return false; // URL dị dạng, không parse được -> Cấm
}

#ifdef FETCH_IMPLEMENTATION_LIBCURL
/**
 * Standard libcurl write callback (Desktop Only).
 * Đã thêm bảo vệ RAM: Chặn tải file > 5MB để tránh OOM.
 */
static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
	size_t totalSize = size * nmemb;
	std::string *str = static_cast<std::string *>(userp);

	// AutoLang Limits: Không cho phép string phình to vô hạn
	const size_t MAX_SIZE = 5 * 1024 * 1024; // 5MB
	if (str->size() + totalSize > MAX_SIZE) {
		return 0; // Trả về 0 sẽ ép CURL ngắt request ngay lập tức
		          // (CURLE_WRITE_ERROR)
	}

	str->append(static_cast<char *>(contents), totalSize);
	return totalSize;
}
#endif

// ============================================================================
// 1. Fetch a single URL (Synchronously blocks VM thread, returns String)
// ============================================================================
inline AObject *get(NativeFuncInData) {
	const char *url = args[0]->str->data;
	long timeout_ms = static_cast<long>(args[1]->i);

	// [SECURITY GATE]
	if (!checkUrlSecurity(url, notifier)) {
		notifier.throwException(
		    "SecurityError: Domain not allowed or URL is malformed/too long.");
		return nullptr;
	}

#ifdef FETCH_IMPLEMENTATION_LIBCURL
	CURL *curl = curl_easy_init();
	if (!curl) {
		notifier.throwException("Failed to initialize libcurl");
		return nullptr;
	}

	std::string response_string;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
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
		notifier.throwException("HTTP Error status code: " +
		                        std::to_string(response_code));
		return nullptr;
	}

	return notifier.createString(response_string);

#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "GET");
	attr.timeoutMSecs = timeout_ms;
	attr.attributes =
	    EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;

	emscripten_fetch_t *fetch = emscripten_fetch(&attr, url);

	if (fetch->status == 200) {
		std::string result(fetch->data, fetch->numBytes);
		emscripten_fetch_close(fetch);
		return notifier.createString(result);
	} else {
		std::string err_msg =
		    "HTTP Request failed with status: " + std::to_string(fetch->status);
		emscripten_fetch_close(fetch);
		notifier.throwException(err_msg);
		return nullptr;
	}
#endif
}

inline AObject *post(NativeFuncInData) {
	const char *url = args[0]->str->data;
	const std::string &body = args[1]->str->data;
	long timeout_ms = static_cast<long>(args[2]->i);

	// [SECURITY GATE]
	if (!checkUrlSecurity(url, notifier)) {
		notifier.throwException(
		    "SecurityError: Domain not allowed or URL is malformed/too long.");
		return nullptr;
	}

#ifdef FETCH_IMPLEMENTATION_LIBCURL
	CURL *curl = curl_easy_init();
	if (!curl) {
		notifier.throwException("Failed to initialize libcurl");
		return nullptr;
	}

	std::string response_string;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
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
		notifier.throwException("HTTP Error status code: " +
		                        std::to_string(response_code));
		return nullptr;
	}

	return notifier.createString(response_string);

#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "POST");
	attr.timeoutMSecs = timeout_ms;
	attr.attributes =
	    EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
	
	attr.requestData = body.c_str();
	attr.requestDataSize = body.size();

	emscripten_fetch_t *fetch = emscripten_fetch(&attr, url);

	if (fetch->status >= 200 && fetch->status < 300) {
		std::string result(fetch->data, fetch->numBytes);
		emscripten_fetch_close(fetch);
		return notifier.createString(result);
	} else {
		std::string err_msg =
		    "HTTP Request failed with status: " + std::to_string(fetch->status);
		emscripten_fetch_close(fetch);
		notifier.throwException(err_msg);
		return nullptr;
	}
#endif
}

inline AObject *http_delete(NativeFuncInData) {
	const char *url = args[0]->str->data;
	long timeout_ms = static_cast<long>(args[1]->i);

	// [SECURITY GATE]
	if (!checkUrlSecurity(url, notifier)) {
		notifier.throwException(
		    "SecurityError: Domain not allowed or URL is malformed/too long.");
		return nullptr;
	}

#ifdef FETCH_IMPLEMENTATION_LIBCURL
	CURL *curl = curl_easy_init();
	if (!curl) {
		notifier.throwException("Failed to initialize libcurl");
		return nullptr;
	}

	std::string response_string;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
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
		notifier.throwException("HTTP Error status code: " +
		                        std::to_string(response_code));
		return nullptr;
	}

	return notifier.createString(response_string);

#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "DELETE");
	attr.timeoutMSecs = timeout_ms;
	attr.attributes =
	    EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;

	emscripten_fetch_t *fetch = emscripten_fetch(&attr, url);

	if (fetch->status >= 200 && fetch->status < 300) {
		std::string result(fetch->data, fetch->numBytes);
		emscripten_fetch_close(fetch);
		return notifier.createString(result);
	} else {
		std::string err_msg =
		    "HTTP Request failed with status: " + std::to_string(fetch->status);
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
	long timeout_ms = static_cast<long>(args[2]->i);

	size_t url_count = notifier.getArraySize(arrayObj);
	if (url_count == 0) {
		return notifier.createArray(arrayClassId);
	}

	// [SECURITY GATE] Kiểm tra toàn bộ mảng trước khi khởi tạo Request
	for (size_t i = 0; i < url_count; ++i) {
		const char *url = arrayObj->member->data[i]->str->data;
		if (!checkUrlSecurity(url, notifier)) {
			notifier.throwException(
			    std::string("SecurityError: Blocked URL in array: ") + url);
			return nullptr;
		}
	}

#ifdef FETCH_IMPLEMENTATION_LIBCURL
	CURLM *multi_handle = curl_multi_init();
	std::vector<CURL *> easy_handles(url_count);
	std::vector<std::string> response_strings(url_count);

	for (size_t i = 0; i < url_count; ++i) {
		const char *url = arrayObj->member->data[i]->str->data;
		easy_handles[i] = curl_easy_init();

		curl_easy_setopt(easy_handles[i], CURLOPT_URL, url);
		curl_easy_setopt(easy_handles[i], CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(easy_handles[i], CURLOPT_WRITEDATA,
		                 &response_strings[i]);
		curl_easy_setopt(easy_handles[i], CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(easy_handles[i], CURLOPT_CONNECTTIMEOUT_MS,
		                 timeout_ms);
		curl_easy_setopt(easy_handles[i], CURLOPT_TIMEOUT_MS, timeout_ms);

		curl_multi_add_handle(multi_handle, easy_handles[i]);
	}

	int still_running = 0;
	do {
		CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
		if (mc == CURLM_OK && still_running) {
			curl_multi_poll(multi_handle, nullptr, 0, 1000, nullptr);
		}
	} while (still_running);

	auto newArr = notifier.createArray(arrayClassId);

	for (size_t i = 0; i < url_count; ++i) {
		long response_code = 0;
		curl_easy_getinfo(easy_handles[i], CURLINFO_RESPONSE_CODE,
		                  &response_code);
		curl_multi_remove_handle(multi_handle, easy_handles[i]);
		curl_easy_cleanup(easy_handles[i]);

		if (response_code == 200) {
			notifier.arrayAdd(newArr,
			                  notifier.createString(response_strings[i]));
		} else {
			notifier.arrayAdd(newArr, notifier.createString(""));
		}
	}

	curl_multi_cleanup(multi_handle);
	return newArr;

#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
	std::vector<emscripten_fetch_t *> fetches(url_count);

	for (size_t i = 0; i < url_count; ++i) {
		const char *url = arrayObj->member->data[i]->str->data;

		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "GET");
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
		attr.timeoutMSecs = timeout_ms;

		fetches[i] = emscripten_fetch(&attr, url);
	}

	bool all_done = false;
	while (!all_done) {
		all_done = true;
		for (size_t i = 0; i < url_count; ++i) {
			if (fetches[i]->numBytes == 0 && fetches[i]->status == 0) {
				all_done = false;
				break;
			}
		}
		if (!all_done)
			emscripten_sleep(10);
	}

	auto newArr = notifier.createArray(arrayClassId);
	for (size_t i = 0; i < url_count; ++i) {
		if (fetches[i]->status == 200) {
			std::string res(fetches[i]->data, fetches[i]->numBytes);
			notifier.arrayAdd(newArr, notifier.createString(res));
		} else {
			notifier.arrayAdd(newArr, notifier.createString(""));
		}
		emscripten_fetch_close(fetches[i]);
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
    @native("http_get")
    static func get(url: String, timeoutMs: Int = 10000): String

    @native("http_post")
    static func post(url: String, body: String, timeoutMs: Int = 10000): String

    @native("http_delete")
    static func delete(url: String, timeoutMs: Int = 10000): String

    @native("http_get_all")
    private static func _getAll(urls: Array<String>, arrayClassId: Int, timeoutMs: Int): Array<String>
    
    static func getAll(urls: Array<String>, timeoutMs: Int = 10000): Array<String> = _getAll(urls, getClassId(Array<String>), timeoutMs)
}
        )###",
	                                LibraryConfig(false, true, true),
	                                ANativeMap({
	                                    {"http_get", &http::get},
	                                    {"http_post", &http::post},
	                                    {"http_delete", &http::http_delete},
	                                    {"http_get_all", &http::get_all},
	                                }));
}

} // namespace http
} // namespace Libs
} // namespace AutoLang
#endif