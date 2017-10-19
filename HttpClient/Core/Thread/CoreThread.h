#ifndef __CORE_THREAD_H__
#define __CORE_THREAD_H__

///----------------------------------------------------------------------
///----------------------------------------------------------------------
#include <functional>
#include "Thread.h"
#include "Interlock.h"

namespace App
{
	namespace Thread
	{
		class Thread : Core
		{
		private:
			RKCriticalSection*   m_criticalSection;
			RKThread *			 m_Thread;
			RKInterlock*         m_exit;
			RKThreadCondition*   m_condition;

			volatile bool		 m_IsThreadOnRunning;
			DWORD				 m_time_sleep;

			s32					m_id;
		public:
			Thread();
			Thread(DWORD time_sleep);

			virtual ~Thread();

			void CreateThread(const std::string & name_thread, 
									RKThreadCallback* pThreadAddress, 
									void* pThreadParams, 
									RKThreadPriority priority = RKThreadPriority_Normal, 
									RKThreadStackSize stackSize = RKThreadStackSize_2048K);
			void ReleaseThread();
			void OnCheckUpdateThread(const std::function<void(void *)>& func , void * data);
			void OnWakeUpAndRunThread();

			bool IsThreadRunning() { return m_IsThreadOnRunning; }

			_ssize_t GetCurrentID();
		};
	}
}
///----------------------------------------------------------------------
#endif // __CORE_THREAD_H__