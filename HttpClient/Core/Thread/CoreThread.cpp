///----------------------------------------------------------------------
///----------------------------------------------------------------------
#include "CoreThread.h"

namespace App
{
	namespace Thread
	{
		Thread::Thread()
			: m_criticalSection(0)
			, m_Thread(0)
			, m_exit(0)
			, m_condition(0)
		{
			m_IsThreadOnRunning = false;
			m_time_sleep = 0xFFFFFFF;//sleep infinity
			m_id = -1;
		}

		Thread::Thread(DWORD time_sleep)
			: m_criticalSection(0)
			, m_Thread(0)
			, m_exit(0)
			, m_condition(0)
		{
			m_IsThreadOnRunning = false;
			m_time_sleep = time_sleep;
			m_id = -1;
		}

		Thread::~Thread()
		{
			bool bExit = false;
			if (m_exit)
			{
				bExit = RKInterlock_Query(m_exit) != 0;
			}

			if (!bExit)
			{
				// tell the work thread to exit
				RKInterlock_Lock(m_exit);

				// wake work thread
				if (m_condition)
				{
					RKThreadCondition_WakeAll(m_condition);
				}

				if (m_Thread)
				{
					RKThread_WaitForExit(m_Thread);
					RKThread_Destroy(&m_Thread);
				}

				if (m_condition)
				{
					RKThreadCondition_Destroy(&m_condition);
				}

				if (!m_exit)
				{
					RKInterlock_Destroy(&m_exit);
				}
			}

			m_IsThreadOnRunning = false;
			m_time_sleep = 0xFFFFFFF;//sleep infinity

		}

		void Thread::CreateThread(const std::string & name_thread, RKThreadCallback* pThreadAddress, void* pThreadParams, RKThreadPriority priority, RKThreadStackSize stackSize)
		{
			m_IsThreadOnRunning = true;

			m_criticalSection = RKCriticalSection_Create((name_thread + "_mutex").c_str());

			m_condition = RKThreadCondition_Create((name_thread + "_condition").c_str());

			m_exit = RKInterlock_Create((name_thread + "_exit").c_str());

			m_Thread = RKThread_Create(name_thread.c_str(), pThreadAddress, pThreadParams, priority, stackSize);

			RKThread_Start(m_Thread);
		}

		_ssize_t Thread::GetCurrentID()
		{
			if (m_Thread != NULL)
			{
				return (size_t)RKThread_GetID(m_Thread);
			}
			return 0;
		}

		void Thread::ReleaseThread()
		{
			if (this)
			{
				DELETE(this);
			}
		}

		void Thread::OnCheckUpdateThread(const std::function<void()>& func)
		{
			if (m_exit == NULL)
				return;
			// keep the thread running until we want to stop it
			bool bExit = RKInterlock_Query(m_exit) != 0;
			while (!bExit)
			{
				// wait for work to be added
				RKCriticalSection_Enter(m_criticalSection);
				bExit = RKInterlock_Query(m_exit) != 0;

				while (!m_IsThreadOnRunning && !bExit)
				{
					RKThreadCondition_Sleep(m_condition, m_criticalSection, m_time_sleep); //sleep infinity and wait wake up
					bExit = RKInterlock_Query(m_exit) != 0;
				}

				if (bExit)
				{
					RKCriticalSection_Leave(m_criticalSection);
					break;
				}

				func();
				//run the function here !

				m_IsThreadOnRunning = false;

				RKCriticalSection_Leave(m_criticalSection);
			}
			RKInterlock_Reset(m_exit);
		}

		void Thread::OnWakeUpAndRunThread()
		{
			m_IsThreadOnRunning = true;
			RKThreadCondition_WakeAll(m_condition);
		}
	}
}