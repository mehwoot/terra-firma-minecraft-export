#include "Includes.h"
#include "Memory.h"
#include "Util/OS.h"
#include "Util/Sentry.h"

using namespace Util;

MemoryManager::MemoryManager() {

}

MemoryManager::~MemoryManager() {

}

void MemoryManager::printMemoryStats() {
	auto memoryStats = getMemoryStats();

	std::ostringstream systemMemory, trackedMemory, processMemory;
	systemMemory.precision(3);
	systemMemory << memoryStats.globalMemoryPresentGb << "gb";
	trackedMemory.precision(3);
	trackedMemory << memoryStats.processTrackedUsedGb << "gb";
	processMemory.precision(3);
	processMemory << memoryStats.processTotalUsedGb << "gb";

	Sentry::get().setContext({
		{ "systemMemory"s, systemMemory.str() },
		{ "processMemory"s, processMemory.str() },
		{ "trackedMemory"s, trackedMemory.str() },
	});
}

void MemoryManager::recordAllocation(const std::string& type, size_t size, void* memory) {
	if (memory != nullptr) {
		std::unique_lock<std::shared_mutex> lock(allocationMutex);

		sizes[type] += size;
		allocations[memory] = { type, size };
	} else {
		printMemoryStats();
		fatal_assert(false, "out of memory");
	}
}

void MemoryManager::recordDeallocation(void* memory) {
	std::unique_lock<std::shared_mutex> lock(allocationMutex);

	if (memory == nullptr) return;
	auto allocationIt = allocations.find(memory);
	soft_assert_if(allocationIt != allocations.end(), "could not find allocation") {
		sizes[allocationIt->second.first] -= allocationIt->second.second;
		allocations.erase(allocationIt);
	}
}

MemoryStats MemoryManager::getMemoryStats() const {
	auto stats = Util::OS::getMemoryStats();

	size_t processTrackedUsedBytes = 0;
	for (const auto& sizePair : sizes) {
		processTrackedUsedBytes += sizePair.second;
	}
	stats.processTrackedUsedGb = processTrackedUsedBytes / (1024.0 * 1024.0 * 1024.0);

	return stats;
}