//----------------------------------------------------------------------
/// RKEngine	(c) 2005 + Daniel Stephens
///
/// File		:	RKTimer.cpp
///
/// Description :	X86 High resolution timer class
///----------------------------------------------------------------------

///----------------------------------------------------------------------
/// Includes
///----------------------------------------------------------------------

#include "Core/Heap/Heap.h"
#include "Core/Thread/Timer.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
namespace App
{
	static Timer* Timer_pClock = 0;

	///----------------------------------------------------------------------
	/// RKTimer_InitModule
	///----------------------------------------------------------------------
	void Timer_InitModule()
	{
		Timer_pClock = NEW(Timer);
		Timer_pClock->Init();
	}

	///----------------------------------------------------------------------
	/// RKTimer_DeinitModule
	///----------------------------------------------------------------------
	void Timer_DeinitModule()
	{
		if (Timer_pClock)
		{
			DELETE(Timer_pClock);
			Timer_pClock = NULL;
		}
	}

	///----------------------------------------------------------------------
	/// RKTimer_GetUpTime
	///----------------------------------------------------------------------
	float Timer_GetUpTime()
	{
		return Timer_pClock->GetUpTime();
	}

	///----------------------------------------------------------------------
	/// RKTimer_GetElapsedTime
	///----------------------------------------------------------------------
	float Timer_GetElapsedTime()
	{
		return Timer_pClock->GetElapsedTime();
	}

	float Timer_GetBufferedElapsedTime()
	{
		return Timer_pClock->GetBufferedElapsedTime();
	}

	float Timer_GetBufferedUpTime()
	{
		return Timer_pClock->GetBufferedUpTime();
	}


	///----------------------------------------------------------------------
	/// RKTimer:: Constructor
	///----------------------------------------------------------------------
	Timer::Timer() :
		m_FrameTime(0.f)
		, m_UpTime(0.f)
	{
	}

	///----------------------------------------------------------------------
	/// RKTimer:: Destructor
	///----------------------------------------------------------------------
	Timer::~Timer()
	{
	}

	///----------------------------------------------------------------------
	/// RKTimer::Init()
	///----------------------------------------------------------------------
	void Timer::Init()
	{
		QueryPerformanceFrequency(&m_PerformanceFrequency);
		QueryPerformanceCounter(&m_InitTime);
		m_LastTime = m_InitTime;
	}

	///----------------------------------------------------------------------
	/// RKTimer::GetUpTime()
	///----------------------------------------------------------------------
	float Timer::GetUpTime()
	{
		LARGE_INTEGER currentTime; QueryPerformanceCounter(&currentTime);
		m_UpTime = (float)(currentTime.QuadPart - m_InitTime.QuadPart) / m_PerformanceFrequency.QuadPart;
		return m_UpTime;
	}

	///----------------------------------------------------------------------
	/// RKTimer::GetElapsedTime()
	///----------------------------------------------------------------------
	float Timer::GetElapsedTime()
	{
		LARGE_INTEGER currentTime; QueryPerformanceCounter(&currentTime);
		m_FrameTime = (float)(currentTime.QuadPart - m_LastTime.QuadPart) / m_PerformanceFrequency.QuadPart;
		m_LastTime = currentTime;
		return m_FrameTime;
	}

	///----------------------------------------------------------------------
	/// RKTimer::GetBufferedElapsedTime()
	///----------------------------------------------------------------------
	float Timer::GetBufferedElapsedTime()
	{
		return m_FrameTime;
	}

	///----------------------------------------------------------------------
	/// RKTimer::GetBufferedUpTime()
	///----------------------------------------------------------------------
	float Timer::GetBufferedUpTime()
	{
		return m_UpTime;
	}
}