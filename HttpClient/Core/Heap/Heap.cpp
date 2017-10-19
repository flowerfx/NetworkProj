#include "Heap.h"

#ifdef _WIN32
#include <crtdbg.h>
#endif

namespace App
{
	void Heap_InitModule()
	{
	}

	void Heap_DeinitModule()
	{
	}

	bool Heap_IsInited()
	{
		return true;
	}

	void* Heap_Alloc(_ssize_t size, const char* tag)
	{
		return malloc(size);
	}

	void* Heap_AllocAndZero(_ssize_t size, const char* tag)
	{
		void* p = malloc(size);
		if (p)
		{
			memset(p, 0, size);
		}
		return p;
	}

	void* Heap_Realloc(void* p, _ssize_t newSize)
	{
		return realloc(p, newSize);
	}

	void  Heap_Free(void* p, const char* tag)
	{
		free(p);
	}

	void* Heap_AllocAligned(_ssize_t size, _ssize_t alignment, const char *tag)
	{
#if defined (_WIN32)
		return _aligned_malloc(size, alignment);
#elif defined (_IOS)
		void* pMemory = 0;
		posix_memalign(&pMemory, alignment, size); // malloc(size);
		return pMemory;
#else
		return malloc(size);
#endif
	}

	void  Heap_FreeAligned(void* p, const char *tag)
	{
#if defined (_WIN32)
		_aligned_free(p);
#else
		free(p);
#endif // PLATFORM_WIN32
	}

	_ssize_t Heap_GetUsage()
	{
		return 0;
	}

	void Heap_CheckIntegrity()
	{
#ifdef _WIN32
		if (_CrtCheckMemory() != TRUE)
		{
			ASSERT(0, "Some memory has been corrupted!");
		}
#else
		//iOS : todo: use a system memory check ?
#ifndef RETAIL
		//use system debug check at alloc/free time, and hope to catch problem if any
		void *checkAlloc1 = Heap_Alloc(50);
		Heap_Free(checkAlloc1);
#endif
#endif
	}

	void Heap_SetDumpFilename(const char *pDumpFilename)
	{
	}

	void HeapError_EnableReport(bool enab)
	{
	}

	void Heap_Report()
	{
	}

	void Heap_EnableSeriousMemoryChecking(bool enabled)
	{
	}
}