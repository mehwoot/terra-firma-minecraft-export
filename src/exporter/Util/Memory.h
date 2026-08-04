#pragma once

#include "Singleton.h"
#include "Macros.h"

#include <map>
#include <string>
#include <shared_mutex>

namespace Util {
	struct MemoryStats {
		double globalMemoryPresentGb;
		double processTrackedUsedGb;
		double processTotalUsedGb;
	};

	StandardTypes(MemoryManager);

	class MemoryManager : public Util::Singleton<MemoryManager> {
	protected:
		std::map<std::string, size_t> sizes;
		std::map<void*, std::pair<std::string, size_t>> allocations;
		std::shared_mutex allocationMutex;

		void printMemoryStats();

	public:
		MemoryManager();
		~MemoryManager();

		void recordAllocation(const std::string& type, size_t size, void* memory);
		void recordDeallocation(void* memory);
		const std::map<std::string, size_t>& getAllocations() const { return sizes; }
		MemoryStats getMemoryStats() const;

		template<class T>
		void operator()(T* memory) {
			recordDeallocation(memory);
			delete memory;
		};

		template<class ArrayType>
		std::unique_ptr<ArrayType, MemoryManagerRef> make_unique(size_t size) {
			typedef typename std::remove_extent<ArrayType>::type OriginalType;
			try {
				auto memory = new OriginalType[size]();

				recordAllocation(typeid(ArrayType).name(), sizeof(OriginalType) * size, memory);
				return std::unique_ptr<ArrayType, MemoryManagerRef>(memory, *this);
			} catch (const std::bad_alloc&) {
				printMemoryStats();
				fatal_assert(false, "out of memory");
				return std::unique_ptr<ArrayType, MemoryManagerRef>(nullptr, *this);
			}
		}
	};

	template<class T>
	std::unique_ptr<T, MemoryManagerRef> make_unique(size_t size) {
		return MemoryManager::get().make_unique<T>(size);
	}
}