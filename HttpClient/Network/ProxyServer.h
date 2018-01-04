#ifndef __PROXY_SERVER_H__
#define __PROXY_SERVER_H__

#include <vector>
#include "NetworkMacros.h"
#include "Core/Heap/Heap.h"
namespace App
{
	namespace Network
	{

		#define MAX_CLIENT 1600
		class ClientMessage;
		class ProxyServer : public Core
		{
		private:
			//store the socket ID that the proxy server is binding into its
			s32 socketId;
			//store the port;
			SYNC_VALUE(port, u32);
			//store the ipv4
			SYNC_VALUE(ip, IPv4);

			//store the add handle with API
			struct sockaddr_in serv_addr;

			//maximum capility client request
			SYNC_VALUE(maxCapility, u64);

			//handle message from client
			u32 numberClientMsg;
			ClientMessage * _listClientMsg[MAX_CLIENT];

			//deny list;
			std::vector<std::string> denyList;

		public:
			ProxyServer();
			ProxyServer(u32 _port, IPv4 _ip);
			virtual ~ProxyServer();

			//init with wsaData, and null data
			HRESULT init();
			HRESULT init(WSADATA & wsaData);

			//listen and get info from client connect to port
			HRESULT onHandleRequestFromClient();

			//
			void onReadDenyListFromFile(const char * name);

			//
			bool isDenyURL(const char * url);
		


		};
	}
}

#endif //__PROXY_SERVER_H__
