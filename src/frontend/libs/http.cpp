#ifndef LIB_HTTP_CPP
#define LIB_HTTP_CPP

#include "backend/libs/map.hpp"
#include "frontend/ACompiler.hpp"
#include "shared/DefaultOperator.hpp"
#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/fetch.h>
#define FETCH_IMPLEMENTATION_EMSCRIPTEN
#else
#include <curl/curl.h>
#define FETCH_IMPLEMENTATION_LIBCURL
#endif

namespace Autolang {
class ACompiler;

namespace Libs {
namespace http {

inline std::string extractDomainFast(const std::string &url) {
	size_t start = url.find("://");
	start = (start != std::string::npos) ? start + 3 : 0;
	size_t end = url.find('/', start);
	if (end == std::string::npos)
		end = url.length();
	size_t at = url.find('@', start);
	if (at != std::string::npos && at < end)
		start = at + 1;
	std::string domain = url.substr(start, end - start);
	size_t port = domain.find(':');
	if (port != std::string::npos)
		domain = domain.substr(0, port);
	return domain;
}

inline bool checkUrlSecurity(const char *url, ANotifier &notifier) {
	std::string urlStr(url);
	if (urlStr.length() > 512)
		return false;
	if (notifier.vm->allowedDomainsRegex == nullptr)
		return true;
	std::string targetDomain = extractDomainFast(urlStr);
	if (targetDomain.empty())
		return false;
	return std::regex_match(targetDomain, *(notifier.vm->allowedDomainsRegex));
}

inline AObject *createHeadersMap(ANotifier &notifier, ClassId mapClassId,
                                 const std::string &rawHeaders) {
	AObject *newMap = Autolang::Libs::map::constructor(
	    notifier, mapClassId, DefaultClass::stringClassId);
	auto hashMapData =
	    static_cast<Autolang::Libs::map::AHashMap *>(newMap->data->data);
	auto map =
	    static_cast<Autolang::Libs::map::StringHashMap *>(hashMapData->data);

	std::istringstream stream(rawHeaders);
	std::string line;
	while (std::getline(stream, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty() || line.find("HTTP/") == 0)
			continue;

		size_t colon = line.find(':');
		if (colon != std::string::npos) {
			std::string key = line.substr(0, colon);
			size_t valStart = colon + 1;
			while (valStart < line.length() && line[valStart] == ' ')
				valStart++;
			std::string val = line.substr(valStart);

			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			AObject *keyObj = notifier.createString(key);
			AObject *valObj = notifier.createString(val);

			(*map)[keyObj] = valObj;
			keyObj->retain();
			valObj->retain();
		}
	}
	return newMap;
}

#ifdef FETCH_IMPLEMENTATION_LIBCURL
static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            void *userp) {
	size_t totalSize = size * nmemb;
	std::string *str = static_cast<std::string *>(userp);
	const size_t MAX_SIZE = 5 * 1024 * 1024;
	if (str->size() + totalSize > MAX_SIZE)
		return 0;
	str->append(static_cast<char *>(contents), totalSize);
	return totalSize;
}
#endif

#ifdef FETCH_IMPLEMENTATION_EMSCRIPTEN
struct FetchContext {
	bool done = false;
	emscripten_fetch_t *fetch = nullptr;
};

static void on_fetch_success(emscripten_fetch_t *fetch) {
	if (fetch->userData) {
		auto *ctx = static_cast<FetchContext *>(fetch->userData);
		ctx->fetch = fetch;
		ctx->done = true;
	}
}

static void on_fetch_error(emscripten_fetch_t *fetch) {
	if (fetch->userData) {
		auto *ctx = static_cast<FetchContext *>(fetch->userData);
		ctx->fetch = fetch;
		ctx->done = true;
	}
}
#endif

inline AObject *get(NativeFuncInData) {
	const char *req_url = args[0]->str->data;
	long timeout_ms = static_cast<long>(args[1]->i);
	ClassId resClassId = args[2]->i;
	ClassId mapClassId = args[3]->i;

	int status_code = 0;
	std::string response_body = "";
	std::string response_headers = "";
	std::string effective_url = req_url;

	if (!checkUrlSecurity(req_url, notifier)) {
		status_code = 403;
		response_body = "SecurityError: Domain not allowed.";
	} else {
#ifdef FETCH_IMPLEMENTATION_LIBCURL
		CURL *curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, req_url);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);

			CURLcode res = curl_easy_perform(curl);
			char *url_ptr = nullptr;
			curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url_ptr);
			if (url_ptr)
				effective_url = url_ptr;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

			if (res != CURLE_OK && status_code == 0) {
				status_code = 500;
				response_body = curl_easy_strerror(res);
			}
			curl_easy_cleanup(curl);
		}
#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "GET");
		attr.timeoutMSecs = timeout_ms;
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

		FetchContext ctx;
		attr.onsuccess = on_fetch_success;
		attr.onerror = on_fetch_error;
		attr.userData = &ctx;

		emscripten_fetch(&attr, req_url);
		while (!ctx.done) {
			emscripten_sleep(10);
		}
		emscripten_fetch_t *fetch = ctx.fetch;

		status_code = fetch->status == 0 ? 500 : fetch->status;
		effective_url = fetch->url;
		response_body =
		    std::string(fetch->data && fetch->numBytes > 0 ? fetch->data : "",
		                fetch->numBytes);

		size_t header_len = emscripten_fetch_get_response_headers_length(fetch);
		if (header_len > 0) {
			std::vector<char> h_buf(header_len);
			emscripten_fetch_get_response_headers(fetch, h_buf.data(),
			                                      header_len);
			response_headers = std::string(h_buf.data());
		}
		emscripten_fetch_close(fetch);
#endif
	}

	auto clazz = notifier.vm->data.classes[resClassId];
	AObject *resObj =
	    notifier.createMemberObject(resClassId, clazz->memberMap.size());

	auto member0 = notifier.createInt(status_code);
	member0->retain();
	resObj->member->data[0] = member0;

	auto member1 = notifier.createBool(status_code >= 200 && status_code < 300);
	member1->retain();
	resObj->member->data[1] = member1;

	auto member2 = notifier.createString(effective_url);
	member2->retain();
	resObj->member->data[2] = member2;

	auto member3 = createHeadersMap(notifier, mapClassId, response_headers);
	member3->retain();
	resObj->member->data[3] = member3;

	auto member4 = notifier.createString(response_body);
	member4->retain();
	resObj->member->data[4] = member4;

	return resObj;
}

inline AObject *post(NativeFuncInData) {
	const char *req_url = args[0]->str->data;
	const std::string &body = args[1]->str->data;
	long timeout_ms = static_cast<long>(args[2]->i);
	ClassId resClassId = args[3]->i;
	ClassId mapClassId = args[4]->i;

	int status_code = 0;
	std::string response_body = "";
	std::string response_headers = "";
	std::string effective_url = req_url;

	if (!checkUrlSecurity(req_url, notifier)) {
		status_code = 403;
		response_body = "SecurityError: Domain not allowed.";
	} else {
#ifdef FETCH_IMPLEMENTATION_LIBCURL
		CURL *curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, req_url);
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
			                 static_cast<long>(body.size()));
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);

			CURLcode res = curl_easy_perform(curl);
			char *url_ptr = nullptr;
			curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url_ptr);
			if (url_ptr)
				effective_url = url_ptr;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

			if (res != CURLE_OK && status_code == 0) {
				status_code = 500;
				response_body = curl_easy_strerror(res);
			}
			curl_easy_cleanup(curl);
		}
#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "POST");
		attr.timeoutMSecs = timeout_ms;
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
		attr.requestData = body.c_str();
		attr.requestDataSize = body.size();

		FetchContext ctx;
		attr.onsuccess = on_fetch_success;
		attr.onerror = on_fetch_error;
		attr.userData = &ctx;

		emscripten_fetch(&attr, req_url);
		while (!ctx.done) {
			emscripten_sleep(10);
		}
		emscripten_fetch_t *fetch = ctx.fetch;

		status_code = fetch->status == 0 ? 500 : fetch->status;
		effective_url = fetch->url;
		response_body =
		    std::string(fetch->data && fetch->numBytes > 0 ? fetch->data : "",
		                fetch->numBytes);

		size_t header_len = emscripten_fetch_get_response_headers_length(fetch);
		if (header_len > 0) {
			std::vector<char> h_buf(header_len);
			emscripten_fetch_get_response_headers(fetch, h_buf.data(),
			                                      header_len);
			response_headers = std::string(h_buf.data());
		}
		emscripten_fetch_close(fetch);
#endif
	}

	auto clazz = notifier.vm->data.classes[resClassId];
	AObject *resObj =
	    notifier.createMemberObject(resClassId, clazz->memberMap.size());

	auto member0 = notifier.createInt(status_code);
	member0->retain();
	resObj->member->data[0] = member0;

	auto member1 = notifier.createBool(status_code >= 200 && status_code < 300);
	member1->retain();
	resObj->member->data[1] = member1;

	auto member2 = notifier.createString(effective_url);
	member2->retain();
	resObj->member->data[2] = member2;

	auto member3 = createHeadersMap(notifier, mapClassId, response_headers);
	member3->retain();
	resObj->member->data[3] = member3;

	auto member4 = notifier.createString(response_body);
	member4->retain();
	resObj->member->data[4] = member4;

	return resObj;
}

inline AObject *http_delete(NativeFuncInData) {
	const char *req_url = args[0]->str->data;
	long timeout_ms = static_cast<long>(args[1]->i);
	ClassId resClassId = args[2]->i;
	ClassId mapClassId = args[3]->i;

	int status_code = 0;
	std::string response_body = "";
	std::string response_headers = "";
	std::string effective_url = req_url;

	if (!checkUrlSecurity(req_url, notifier)) {
		status_code = 403;
		response_body = "SecurityError: Domain not allowed.";
	} else {
#ifdef FETCH_IMPLEMENTATION_LIBCURL
		CURL *curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_URL, req_url);
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteCallback);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);

			CURLcode res = curl_easy_perform(curl);
			char *url_ptr = nullptr;
			curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url_ptr);
			if (url_ptr)
				effective_url = url_ptr;
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

			if (res != CURLE_OK && status_code == 0) {
				status_code = 500;
				response_body = curl_easy_strerror(res);
			}
			curl_easy_cleanup(curl);
		}
#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "DELETE");
		attr.timeoutMSecs = timeout_ms;
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

		FetchContext ctx;
		attr.onsuccess = on_fetch_success;
		attr.onerror = on_fetch_error;
		attr.userData = &ctx;

		emscripten_fetch(&attr, req_url);
		while (!ctx.done) {
			emscripten_sleep(10);
		}
		emscripten_fetch_t *fetch = ctx.fetch;

		status_code = fetch->status == 0 ? 500 : fetch->status;
		effective_url = fetch->url;
		response_body =
		    std::string(fetch->data && fetch->numBytes > 0 ? fetch->data : "",
		                fetch->numBytes);

		size_t header_len = emscripten_fetch_get_response_headers_length(fetch);
		if (header_len > 0) {
			std::vector<char> h_buf(header_len);
			emscripten_fetch_get_response_headers(fetch, h_buf.data(),
			                                      header_len);
			response_headers = std::string(h_buf.data());
		}
		emscripten_fetch_close(fetch);
#endif
	}

	auto clazz = notifier.vm->data.classes[resClassId];
	AObject *resObj =
	    notifier.createMemberObject(resClassId, clazz->memberMap.size());

	auto member0 = notifier.createInt(status_code);
	member0->retain();
	resObj->member->data[0] = member0;

	auto member1 = notifier.createBool(status_code >= 200 && status_code < 300);
	member1->retain();
	resObj->member->data[1] = member1;

	auto member2 = notifier.createString(effective_url);
	member2->retain();
	resObj->member->data[2] = member2;

	auto member3 = createHeadersMap(notifier, mapClassId, response_headers);
	member3->retain();
	resObj->member->data[3] = member3;

	auto member4 = notifier.createString(response_body);
	member4->retain();
	resObj->member->data[4] = member4;

	return resObj;
}

inline AObject *get_all(NativeFuncInData) {
	AObject *arrayObj = args[0];
	ClassId arrayClassId = args[1]->i;
	long timeout_ms = static_cast<long>(args[2]->i);
	ClassId resClassId = args[3]->i;
	ClassId mapClassId = args[4]->i;

	size_t url_count = notifier.getArraySize(arrayObj);
	if (url_count == 0)
		return notifier.createArray(arrayClassId);

	auto newArr = notifier.createArray(arrayClassId);
	auto clazz = notifier.vm->data.classes[resClassId];

#ifdef FETCH_IMPLEMENTATION_LIBCURL
	CURLM *multi_handle = curl_multi_init();
	std::vector<CURL *> easy_handles(url_count);
	std::vector<std::string> response_bodies(url_count);
	std::vector<std::string> response_headers_list(url_count);

	for (size_t i = 0; i < url_count; ++i) {
		const char *url = arrayObj->member->data[i]->str->data;
		if (!checkUrlSecurity(url, notifier)) {
			AObject *errObj = notifier.createMemberObject(
			    resClassId, clazz->memberMap.size());
			auto member0 = notifier.createInt(403);
			member0->retain();
			errObj->member->data[0] = member0;

			auto member1 = notifier.createBool(false);
			member1->retain();
			errObj->member->data[1] = member1;

			auto member2 = notifier.createString(url);
			member2->retain();
			errObj->member->data[2] = member2;

			auto member3 = createHeadersMap(notifier, mapClassId, "");
			member3->retain();
			errObj->member->data[3] = member3;

			auto member4 = notifier.createString("SecurityError: Blocked URL");
			member4->retain();
			errObj->member->data[4] = member4;
			notifier.arrayAdd(newArr, errObj);
			easy_handles[i] = nullptr;
			continue;
		}

		easy_handles[i] = curl_easy_init();
		curl_easy_setopt(easy_handles[i], CURLOPT_URL, url);
		curl_easy_setopt(easy_handles[i], CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(easy_handles[i], CURLOPT_TIMEOUT_MS, timeout_ms);
		curl_easy_setopt(easy_handles[i], CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(easy_handles[i], CURLOPT_WRITEDATA,
		                 &response_bodies[i]);
		curl_easy_setopt(easy_handles[i], CURLOPT_HEADERFUNCTION,
		                 WriteCallback);
		curl_easy_setopt(easy_handles[i], CURLOPT_HEADERDATA,
		                 &response_headers_list[i]);
		curl_multi_add_handle(multi_handle, easy_handles[i]);
	}

	int still_running = 0;
	do {
		CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
		if (mc == CURLM_OK && still_running) {
			curl_multi_poll(multi_handle, nullptr, 0, 1000, nullptr);
		}
	} while (still_running);

	for (size_t i = 0; i < url_count; ++i) {
		if (!easy_handles[i])
			continue;

		long status_code = 0;
		char *url_ptr = nullptr;
		std::string effective_url = arrayObj->member->data[i]->str->data;

		curl_easy_getinfo(easy_handles[i], CURLINFO_RESPONSE_CODE,
		                  &status_code);
		curl_easy_getinfo(easy_handles[i], CURLINFO_EFFECTIVE_URL, &url_ptr);
		if (url_ptr)
			effective_url = url_ptr;

		if (status_code == 0)
			status_code = 500;

		AObject *resObj =
		    notifier.createMemberObject(resClassId, clazz->memberMap.size());
		auto member0 = notifier.createInt(status_code);
		member0->retain();
		resObj->member->data[0] = member0;

		auto member1 =
		    notifier.createBool(status_code >= 200 && status_code < 300);
		member1->retain();
		resObj->member->data[1] = member1;

		auto member2 = notifier.createString(effective_url);
		member2->retain();
		resObj->member->data[2] = member2;

		auto member3 =
		    createHeadersMap(notifier, mapClassId, response_headers_list[i]);
		member3->retain();
		resObj->member->data[3] = member3;

		auto member4 = notifier.createString(response_bodies[i]);
		member4->retain();
		resObj->member->data[4] = member4;

		notifier.arrayAdd(newArr, resObj);
		curl_multi_remove_handle(multi_handle, easy_handles[i]);
		curl_easy_cleanup(easy_handles[i]);
	}
	curl_multi_cleanup(multi_handle);

#elif defined(FETCH_IMPLEMENTATION_EMSCRIPTEN)
	std::vector<FetchContext> contexts(url_count);
	std::vector<bool> skipped(url_count, false);

	for (size_t i = 0; i < url_count; ++i) {
		const char *url = arrayObj->member->data[i]->str->data;
		if (!checkUrlSecurity(url, notifier)) {
			AObject *errObj = notifier.createMemberObject(
			    resClassId, clazz->memberMap.size());
			auto member0 = notifier.createInt(403);
			member0->retain();
			errObj->member->data[0] = member0;

			auto member1 = notifier.createBool(false);
			member1->retain();
			errObj->member->data[1] = member1;

			auto member2 = notifier.createString(url);
			member2->retain();
			errObj->member->data[2] = member2;

			auto member3 = createHeadersMap(notifier, mapClassId, "");
			member3->retain();
			errObj->member->data[3] = member3;

			auto member4 = notifier.createString("SecurityError: Blocked URL");
			member4->retain();
			errObj->member->data[4] = member4;
			notifier.arrayAdd(newArr, errObj);
			skipped[i] = true;
			contexts[i].done = true;
			continue;
		}

		emscripten_fetch_attr_t attr;
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "GET");
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
		attr.timeoutMSecs = timeout_ms;
		attr.onsuccess = on_fetch_success;
		attr.onerror = on_fetch_error;
		attr.userData = &contexts[i];

		emscripten_fetch(&attr, url);
	}

	bool all_done = false;
	while (!all_done) {
		all_done = true;
		for (size_t i = 0; i < url_count; ++i) {
			if (!contexts[i].done) {
				all_done = false;
				break;
			}
		}
		if (!all_done)
			emscripten_sleep(10);
	}

	for (size_t i = 0; i < url_count; ++i) {
		if (skipped[i])
			continue;

		emscripten_fetch_t *fetch = contexts[i].fetch;
		int status_code = fetch->status == 0 ? 500 : fetch->status;
		std::string effective_url = fetch->url;
		std::string response_body(
		    fetch->data && fetch->numBytes > 0 ? fetch->data : "",
		    fetch->numBytes);
		std::string response_headers = "";

		size_t header_len = emscripten_fetch_get_response_headers_length(fetch);
		if (header_len > 0) {
			std::vector<char> h_buf(header_len);
			emscripten_fetch_get_response_headers(fetch, h_buf.data(),
			                                      header_len);
			response_headers = std::string(h_buf.data());
		}
		emscripten_fetch_close(fetch);

		AObject *resObj =
		    notifier.createMemberObject(resClassId, clazz->memberMap.size());
		auto member0 = notifier.createInt(status_code);
		member0->retain();
		resObj->member->data[0] = member0;

		auto member1 =
		    notifier.createBool(status_code >= 200 && status_code < 300);
		member1->retain();
		resObj->member->data[1] = member1;

		auto member2 = notifier.createString(effective_url);
		member2->retain();
		resObj->member->data[2] = member2;

		auto member3 = createHeadersMap(notifier, mapClassId, response_headers);
		member3->retain();
		resObj->member->data[3] = member3;

		auto member4 = notifier.createString(response_body);
		member4->retain();
		resObj->member->data[4] = member4;

		notifier.arrayAdd(newArr, resObj);
	}
#endif
	return newArr;
}

void init(ACompiler &compiler) {
	compiler.registerBuiltInLibrary("std/http", R"###(
@no_constructor
@no_extends
class HttpResponse {
    lateinit val status: Int
    lateinit val ok: Bool
    lateinit val url: String
    lateinit val headers: Map<String, String>
    lateinit val body: String
}

@no_constructor
@no_extends
class Http {
    @native("http_get")
    private static fun _get(url: String, timeoutMs: Int, resClassId: Int, mapClassId: Int): HttpResponse

    @native("http_post")
    private static fun _post(url: String, body: String, timeoutMs: Int, resClassId: Int, mapClassId: Int): HttpResponse

    @native("http_delete")
    private static fun _delete(url: String, timeoutMs: Int, resClassId: Int, mapClassId: Int): HttpResponse

    @native("http_get_all")
    private static fun _getAll(urls: Array<String>, arrayClassId: Int, timeoutMs: Int, resClassId: Int, mapClassId: Int): Array<HttpResponse>
    
    static fun get(url: String, timeoutMs: Int = 10000): HttpResponse = 
        _get(url, timeoutMs, getClassId(HttpResponse), getClassId(Map<String, String>))

    static fun post(url: String, body: String, timeoutMs: Int = 10000): HttpResponse = 
        _post(url, body, timeoutMs, getClassId(HttpResponse), getClassId(Map<String, String>))

    static fun delete(url: String, timeoutMs: Int = 10000): HttpResponse = 
        _delete(url, timeoutMs, getClassId(HttpResponse), getClassId(Map<String, String>))

    static fun getAll(urls: Array<String>, timeoutMs: Int = 10000): Array<HttpResponse> = 
        _getAll(urls, getClassId(Array<HttpResponse>), timeoutMs, getClassId(HttpResponse), getClassId(Map<String, String>))
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
} // namespace Autolang
#endif