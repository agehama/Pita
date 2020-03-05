#pragma once
#include <cstdlib>
#include <new>
#include <array>
#include <vector>
#include <deque>

//�O���[�o����new���I�[�o�[���[�h���Ă��邽�ߑ���̃A���P�[�^���`���Ă���
template <class T>
struct SubstituteAllocator
{
	using value_type = T;

	SubstituteAllocator() {}

	template <class U>
	SubstituteAllocator(const SubstituteAllocator<U>&) {}

	T* allocate(std::size_t size)
	{
		return reinterpret_cast<T*>(std::malloc(sizeof(T) * size));
	}

	void deallocate(T* p, std::size_t)
	{
		std::free(p);
	}
};

template <class T, class U>
bool operator==(const SubstituteAllocator<T>&, const SubstituteAllocator<U>&)
{
	return true;
}

template <class T, class U>
bool operator!=(const SubstituteAllocator<T>&, const SubstituteAllocator<U>&)
{
	return false;
}

class RoughMemoryManager
{
public:
	static RoughMemoryManager& instance()
	{
		static RoughMemoryManager i;
		return i;
	}

	bool isEnabled() const { return enabled; }

	void* newMemory(size_t size)
	{
		if (enabled)
		{
			//�m�ۂ��悤�Ƃ����T�C�Y���������u���b�N�̑傫�������傫���ꍇ�͂ǂ�����Ă��m�ۂł��Ȃ�
			if (blockSize < size)
			{
				throw std::bad_alloc();
			}

			const std::uint8_t* expectedEnd = currentNewAddress + size;
			const std::uint8_t* currentBlockEnd = &memoryPool[blockIndex].back();

			//���݂̃����u���b�N�ɓ���Ȃ��傫���������玟�̃u���b�N�̐擪�A�h���X��Ԃ�
			if (currentBlockEnd < expectedEnd)
			{
				push_back();
			}

			std::uint8_t* newAddress = currentNewAddress;
			currentNewAddress += size;
			return newAddress;
		}
		else
		{
			return ::malloc(size);
		}
	}

	void deleteMemory(void* p)
	{
		if (!enabled)
		{
			::free(p);
		}
	}

	void resetPool(std::uint32_t size = 1024)
	{
		blockSize = size;
		memoryPool.clear();

		blockIndex = -1;
		push_back();
	}

	void rewind()
	{
		blockIndex = -1;
		push_back();
	}

	void enable()
	{
		enabled = true;
	}

	void disable()
	{
		enabled = false;
	}

private:
	//currentNewAddress �����̃������u���b�N�̐擪�Ɉړ�������
	void push_back()
	{
		++blockIndex;

		if (memoryPool.size() <= static_cast<std::size_t>(blockIndex))
		{
			memoryPool.emplace_back(blockSize);
		}

		currentNewAddress = memoryPool[blockIndex].data();
	}

	RoughMemoryManager() = default;
	RoughMemoryManager(const RoughMemoryManager&) = default;

	using MemoryBlock = std::vector<std::uint8_t, SubstituteAllocator<std::uint8_t>>;

	std::deque<MemoryBlock, SubstituteAllocator<MemoryBlock>> memoryPool;

	//blockSize �� �S�̂̃��Z�b�g���̂ݕύX�ł���
	std::uint32_t blockSize = 1024;

	std::int32_t blockIndex = -1;
	std::uint8_t* currentNewAddress = nullptr;
	bool enabled = false;
};
