#ifndef __CLIENT_MESSAGE_H__
#define __CLIENT_MESSAGE_H__
#include <vector>
#include "Core/CoreMacros.h"
#include "Core/Thread/CoreThread.h"
namespace App
{
	namespace Network
	{
		class ProxyServer;
		class ClientMessage : public App::Core
		{
			//handle the thread that its process
			Thread::Thread * _thread;
			//store id of socket client and server
			SYNC_VALUE(socket_client_id, s32);
			SYNC_VALUE(socket_server_id, s32);
			SYNC_VALUE(socket_id, s32);
			//store the function that handle the client msg thread
			std::function<u32(void * data)> _func;

			//
			u32		state_ssl;
			SYNC_VALUE(socket_client_id_ssl, s32);
			SYNC_VALUE(socket_server_id_ssl, s32);
			//
			ProxyServer * _server;
			//
			std::vector<ClientMessage*> _childs;
			ClientMessage * _parent;
		public:
			ClientMessage(ProxyServer * server);
			virtual ~ClientMessage();

			void startRequest( std::function<u32(void *)>  fc);
			void onCheckCLientMsg();

			u32		getSSLState() {return state_ssl ;}
			void	setSSLState(u32 s) { state_ssl = s; }

			ProxyServer * getServer() {return _server;}

			ClientMessage * getParent() { return _parent; }
			void addChild(ClientMessage * c) { c->_parent = this;  _childs.push_back(c); }

			void deleteAllChild() {				
				for (auto it : _childs)
				{
					DELETE(it);
				}
				_childs.clear();
			}
			void checkMsgChild() {
				for (auto it : _childs)
				{
					it->onCheckCLientMsg();
				}
			}

		};

	}
}

#endif // __CLIENT_MESSAGE_H__