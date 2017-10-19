#ifndef __HEAP_H__
#define __HEAP_H__

#include <new>
#include "Core/CoreMacros.h"

//#define USE_MANAGE_HEAP

namespace App
{
	void Heap_InitModule();
	void RKHeap_DeinitModule();
	bool RKHeap_IsInited();

	void* Heap_Alloc(_ssize_t size, const char* tag = 0);
	void* Heap_AllocAndZero(_ssize_t size, const char* tag = 0);
	void* Heap_Realloc(void* p, _ssize_t newSize);
	void  Heap_Free(void* p, const char* tag = 0);
	void* Heap_AllocAligned(_ssize_t size, _ssize_t alignment, const char *tag = 0);
	void  Heap_FreeAligned(void* p, const char *tag = 0);

	_ssize_t	  Heap_GetUsage();
	void  Heap_CheckIntegrity();
	unsigned int Heap_GetAllocedSize(void* memaddress);
	void Heap_CheckValidPointer(void* memaddress);
	void Heap_SetDumpFilename(const char *pDumpFilename);
	void Heap_Report();
	void Heap_EnableSeriousMemoryChecking(bool enabled);
	void HeapError_EnableReport(bool enab);


	template <class T>
	void Heap_Destroy(const T* p, const char* tag = 0)
	{
		if (p)
		{
#if 0 //#ifdef _DEBUG
			//to have an assert here, instead of deeper in elephant error report handler /comment out if you feel some slowdown - [christophe.lebouil - 2014-04-01], leak hunt fixes
			RKHeap_CheckValidPointer((T*)p);
#endif

			p->~T();
			Heap_Free((T*)p, tag);
		}
	}

	/**
	* You may be wondering why on earth this function exists and you would be right to do so.
	*
	* Basically, according to the C++ standard, when an array of N objects is allocated for
	* a class T, the space it takes up is N*sizeof(T)+Y. What is Y? Y is used to keep track of how
	* big the array is so that it can be cleaned up later. However, the sizeof(Y) is completely
	* implementation specific, meaning that placement new for arrays is effectively useless, and we
	* will have no idea how much space to allocate for an array without, well, allocating the array.
	*
	* The implementation we have here mimics what Visual C++ does by inserting a count field at the
	* head of the array. Note that this means any special alignment you planned that is > 4 bytes is
	* effectively screwed. Anyway, if it was that important to you, you would be using Heap_AllocAligned()!
	*/
	template <class T>
	T* Heap_CreateArray(u32 numelements, const char* tag = 0)
	{
		uint32* storage = (u32*)Heap_Alloc(numelements * sizeof(T) + sizeof(u32), tag);
		*storage = numelements;
		T* data = (T*)(++storage);
		for (u32 i = 0; i < numelements; ++i)
		{
			new (data + i) T;
		}
		return data;
	}

	template <class T>
	void Heap_DestroyArray(const T* p, const char* tag = 0)
	{
		if (p)
		{
			u32* numelements = (u32*)(p)-1;
			for (int i = *numelements - 1; i >= 0; --i) // destruction in reverse order of construction
			{
				p[i].~T();
			}
			Heap_Free(numelements, tag);
		}
	}

#ifndef USE_MANAGE_HEAP
#define NEW(T) new T
#define DELETE(P) delete P
#define NEWARRAY(T, X) new T[X]
#define DELETEARRAY(P) delete [] P
#else
#define NEW(T) new(Heap_Alloc(sizeof(T))) T
#define DELETE(P) Heap_Destroy(P)
#define NEWARRAY(T, X) Heap_CreateArray<T>(X)
#define DELETEARRAY(P) Heap_DestroyArray(P)
#endif


}

#endif //__HEAP_H__