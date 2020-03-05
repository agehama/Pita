#include <Pita/RoughMemoryManager.hpp>

void* operator new(std::size_t size)
{
	return RoughMemoryManager::instance().newMemory(size);
}

void* operator new[](size_t size)
{
	return RoughMemoryManager::instance().newMemory(size);
}

void operator delete(void* p)
{
	RoughMemoryManager::instance().deleteMemory(p);
}

void operator delete[](void* p)
{
	RoughMemoryManager::instance().deleteMemory(p);
}
