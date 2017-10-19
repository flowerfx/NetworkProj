#ifndef __CLIENT_MESSAGE_H__
#define __CLIENT_MESSAGE_H__
#include "Core/CoreMacros.h"
#include "Core/Thread/CoreThread.h"
namespace App
{
	namespace Network
	{
		class ClientMessage : public App::Core
		{
			//handle the thread that its process
			Thread::Thread * _thread;
			//store id of socket client and server
			SYNC_VALUE(socket_client_id, s32);
			SYNC_VALUE(socket_server_id, s32);

			//store the function that handle the client msg thread
			std::function<u32(void * data)> _func;
		public:
			ClientMessage();
			virtual ~ClientMessage();

			void startRequest( std::function<u32(void *)>  fc);
			void onCheckCLientMsg();
		};
	}
}

#endif // __CLIENT_MESSAGE_H__