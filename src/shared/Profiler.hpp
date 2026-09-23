#ifndef AUTOLANG_PROFILER_HPP
#define AUTOLANG_PROFILER_HPP

#ifndef AUTOLANG_ENABLE_PROFILING
// Comment out or undefine to disable profiling across the entire project
#define AUTOLANG_ENABLE_PROFILING
#endif

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#ifdef CONST
#undef CONST
#endif
#ifdef IN
#undef IN
#endif
#ifdef OUT
#undef OUT
#endif
#ifdef VOID
#undef VOID
#endif
#ifdef ERROR
#undef ERROR
#endif
#elif defined(__linux__)
#include <fstream>
#include <unistd.h>
#include <sys/resource.h>
#endif

namespace Autolang {

struct MemorySnapshot {
	size_t workingSet = 0;       // Bytes
	size_t peakWorkingSet = 0;   // Bytes
	size_t privateBytes = 0;     // Bytes
	size_t peakPrivateBytes = 0; // Bytes

	static MemorySnapshot capture() {
		MemorySnapshot snap;
#if defined(_WIN32) || defined(_WIN64)
		PROCESS_MEMORY_COUNTERS info;
		if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
			snap.workingSet = info.WorkingSetSize;
			snap.peakWorkingSet = info.PeakWorkingSetSize;
			snap.privateBytes = info.PagefileUsage;
			snap.peakPrivateBytes = info.PeakPagefileUsage;
		}
#elif defined(__linux__)
		std::ifstream statm("/proc/self/statm");
		if (statm.is_open()) {
			unsigned long size = 0, resident = 0, share = 0;
			statm >> size >> resident >> share;
			long pageSize = sysconf(_SC_PAGESIZE);
			if (pageSize <= 0) pageSize = 4096;
			snap.workingSet = resident * pageSize;
			snap.privateBytes = (size > share ? (size - share) : size) * pageSize;
		}
		struct rusage usage;
		if (getrusage(RUSAGE_SELF, &usage) == 0) {
			snap.peakWorkingSet = static_cast<size_t>(usage.ru_maxrss) * 1024;
		}
#endif
		return snap;
	}
};

struct ProfileCheckpoint {
	std::string name;
	std::chrono::high_resolution_clock::time_point timestamp;
	MemorySnapshot memory;
	double elapsedFromStartUs = 0.0;
	double stepDurationUs = 0.0;
	int64_t deltaWorkingSet = 0;
	int64_t deltaPrivateBytes = 0;
};

class Profiler {
private:
	std::vector<ProfileCheckpoint> checkpoints;
	std::chrono::high_resolution_clock::time_point startTime;
	MemorySnapshot initialMemory;
	bool active = false;

	static std::string formatDouble(double val, int precision) {
		std::ostringstream ss;
		ss << std::fixed << std::setprecision(precision) << val;
		return ss.str();
	}

public:
	static Profiler &instance() {
		static Profiler s_instance;
		return s_instance;
	}

	void start(const std::string &initialName = "Start") {
		checkpoints.clear();
		startTime = std::chrono::high_resolution_clock::now();
		initialMemory = MemorySnapshot::capture();
		active = true;

		ProfileCheckpoint cp;
		cp.name = initialName;
		cp.timestamp = startTime;
		cp.memory = initialMemory;
		cp.elapsedFromStartUs = 0.0;
		cp.stepDurationUs = 0.0;
		cp.deltaWorkingSet = 0;
		cp.deltaPrivateBytes = 0;
		checkpoints.push_back(cp);
	}

	void mark(const std::string &name) {
		if (!active) {
			start(name);
			return;
		}
		auto now = std::chrono::high_resolution_clock::now();
		auto mem = MemorySnapshot::capture();

		ProfileCheckpoint cp;
		cp.name = name;
		cp.timestamp = now;
		cp.memory = mem;

		const auto &prev = checkpoints.back();
		cp.elapsedFromStartUs = std::chrono::duration<double, std::micro>(now - startTime).count();
		cp.stepDurationUs = std::chrono::duration<double, std::micro>(now - prev.timestamp).count();
		cp.deltaWorkingSet = static_cast<int64_t>(mem.workingSet) - static_cast<int64_t>(prev.memory.workingSet);
		cp.deltaPrivateBytes = static_cast<int64_t>(mem.privateBytes) - static_cast<int64_t>(prev.memory.privateBytes);

		checkpoints.push_back(cp);
	}

	void printReport(const std::string &title = "AUTOLANG PERFORMANCE & RAM PROFILING REPORT") const {
		if (checkpoints.empty()) return;

		double totalUs = checkpoints.back().elapsedFromStartUs;
		if (totalUs <= 0.0) totalUs = 1.0;

		std::cout << "\n" << std::string(118, '=') << "\n";
		std::cout << title << "\n";
		std::cout << std::string(118, '=') << "\n";
		std::cout << std::left << std::setw(36) << "Checkpoint / Phase" << " | "
		          << std::right << std::setw(12) << "Time (ms)" << " | "
		          << std::setw(9) << "Share (%)" << " | "
		          << std::setw(12) << "WS (MB)" << " | "
		          << std::setw(14) << "Delta WS (MB)" << " | "
		          << std::setw(12) << "Priv (MB)" << " | "
		          << std::setw(14) << "Delta Priv" << "\n";
		std::cout << std::string(118, '-') << "\n";

		for (size_t i = 0; i < checkpoints.size(); ++i) {
			const auto &cp = checkpoints[i];
			double ms = cp.stepDurationUs / 1000.0;
			double pct = (cp.stepDurationUs / totalUs) * 100.0;
			double wsMB = cp.memory.workingSet / (1024.0 * 1024.0);
			double dWsMB = cp.deltaWorkingSet / (1024.0 * 1024.0);
			double privMB = cp.memory.privateBytes / (1024.0 * 1024.0);
			double dPrivMB = cp.deltaPrivateBytes / (1024.0 * 1024.0);

			std::string dWsStr = (cp.deltaWorkingSet >= 0 ? "+" : "") + formatDouble(dWsMB, 3) + " MB";
			std::string dPrivStr = (cp.deltaPrivateBytes >= 0 ? "+" : "") + formatDouble(dPrivMB, 3) + " MB";

			std::cout << std::left << std::setw(36) << cp.name << " | "
			          << std::right << std::setw(9) << std::fixed << std::setprecision(3) << ms << " ms | "
			          << std::setw(8) << std::fixed << std::setprecision(1) << pct << "% | "
			          << std::setw(9) << std::fixed << std::setprecision(3) << wsMB << " MB | "
			          << std::setw(14) << dWsStr << " | "
			          << std::setw(9) << std::fixed << std::setprecision(3) << privMB << " MB | "
			          << std::setw(14) << dPrivStr << "\n";
		}
		std::cout << std::string(118, '-') << "\n";

		double totalMs = totalUs / 1000.0;
		auto finalMem = checkpoints.back().memory;
		int64_t netWs = static_cast<int64_t>(finalMem.workingSet) - static_cast<int64_t>(initialMemory.workingSet);
		int64_t netPriv = static_cast<int64_t>(finalMem.privateBytes) - static_cast<int64_t>(initialMemory.privateBytes);

		std::cout << std::left << std::setw(36) << "TOTAL DURATION & NET RAM" << " | "
		          << std::right << std::setw(9) << std::fixed << std::setprecision(3) << totalMs << " ms | "
		          << std::setw(8) << "100.0% | "
		          << std::setw(9) << std::fixed << std::setprecision(3) << (finalMem.workingSet / (1024.0 * 1024.0)) << " MB | "
		          << std::setw(14) << ((netWs >= 0 ? "+" : "") + formatDouble(netWs / (1024.0 * 1024.0), 3) + " MB") << " | "
		          << std::setw(9) << std::fixed << std::setprecision(3) << (finalMem.privateBytes / (1024.0 * 1024.0)) << " MB | "
		          << std::setw(14) << ((netPriv >= 0 ? "+" : "") + formatDouble(netPriv / (1024.0 * 1024.0), 3) + " MB") << "\n";
		std::cout << std::string(118, '=') << "\n\n";
	}

	void reset() {
		checkpoints.clear();
		active = false;
	}
};

} // namespace Autolang

#ifdef AUTOLANG_ENABLE_PROFILING
#define AUTOLANG_PROFILE_START(name) ::Autolang::Profiler::instance().start(name)
#define AUTOLANG_PROFILE_MARK(name) ::Autolang::Profiler::instance().mark(name)
#define AUTOLANG_PROFILE_REPORT(title) ::Autolang::Profiler::instance().printReport(title)
#define AUTOLANG_PROFILE_RESET() ::Autolang::Profiler::instance().reset()
#else
#define AUTOLANG_PROFILE_START(name) ((void)0)
#define AUTOLANG_PROFILE_MARK(name) ((void)0)
#define AUTOLANG_PROFILE_REPORT(title) ((void)0)
#define AUTOLANG_PROFILE_RESET() ((void)0)
#endif

#endif // AUTOLANG_PROFILER_HPP
