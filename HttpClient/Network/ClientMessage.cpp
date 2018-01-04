#include "ClientMessage.h"

namespace App
{
	namespace Network
	{
		ClientMessage::ClientMessage(ProxyServer * server) :
			socket_client_id(-1),
			socket_server_id(-1),
			_func(nullptr),
			_thread(nullptr),
			_server(server),
			state_ssl(0),
			socket_client_id_ssl(-1),
			socket_server_id_ssl(-1)
		{

		}

		ClientMessage::~ClientMessage()
		{
			DELETE(_thread);
			_func = nullptr;
		}


		void ClientMessage::startRequest( std::function<u32(void *)>  fc)
		{
			_func = fc;
			if (! _thread)
			{
				_thread = NEW(App::Thread::Thread);
				_thread->CreateThread("client_handle_msg", ([](void * data) -> u32
				{
					ClientMessage * msg = (ClientMessage*)data;
					msg->_thread->OnCheckUpdateThread
					([](void * d)
					{
						((ClientMessage*)d)->_func(d);
					} , msg);
					return 1;
				}), (void*)this);

			}
		}

		void ClientMessage::onCheckCLientMsg()
		{
			if (_thread)
			{
				if (!_thread->IsThreadRunning())
				{
					_thread->OnWakeUpAndRunThread();
				}
				else
				{
					//OUTPUT_OUT("Thread Client Msg : %d , is on running!", _thread->GetCurrentID());
				}
			}
			else
			{
				ASSERT(false, "this client msg have null thread");
			}
		}


	}
}
