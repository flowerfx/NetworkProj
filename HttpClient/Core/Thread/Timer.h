#ifndef __TIMER_H__
#define __TIMER_H__

#include <windows.h>
#include "Core/CoreMacros.h"
namespace App
{

	///----------------------------------------------------------------------
	/// Includes
	///----------------------------------------------------------------------
	void Timer_InitModule();
	void Timer_DeinitModule();

	float Timer_GetUpTime();
	float Timer_GetElapsedTime();
	float Timer_GetBufferedElapsedTime();
	float Timer_GetBufferedUpTime();

	///----------------------------------------------------------------------
	/// Class Timer
	///----------------------------------------------------------------------
	class Timer : public Core
	{
	public:
		Timer();
		virtual ~Timer();

		void	Init();							  // start the timer
		float	GetUpTime();          // time since the timer was initialized
		float	GetElapsedTime();			// get time since last call to GetElapsedTime
		float	GetBufferedUpTime();
		float	GetBufferedElapsedTime();

	protected:
		float m_FrameTime;
		float m_UpTime;
#if defined (_IOS)
		double m_InitTime;
		double m_LastTime;
#elif defined (_WIN32)
		LARGE_INTEGER m_PerformanceFrequency;
		LARGE_INTEGER m_InitTime;
		LARGE_INTEGER	m_LastTime;
#elif defined (_ANDROID)
		double m_InitTime;
		double m_LastTime;
#endif // RKPLATFORM_IOS
	};

}
#endif // __TIMER_H__