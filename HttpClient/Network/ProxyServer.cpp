#include "ProxyServer.h"
#include "Network/Proxy/proxy_parse.h"

namespace App
{
	namespace Network
	{
#define DEF_CAPILITY 100
		ProxyServer::ProxyServer() :
			port(0),
			ip(0),
			maxCapility(DEF_CAPILITY),
			socketId(-1)
		{

		}

		ProxyServer::ProxyServer(u32 _port, IPv4 _ip) :
			port(_port),
			ip(_ip),
			maxCapility(DEF_CAPILITY),
			socketId(-1)
		{

		}

		ProxyServer::~ProxyServer()
		{

		}

		HRESULT ProxyServer::init()
		{
			WSADATA wsaData;

			// Initialize Winsock
			s32 res =  WSAStartup(MAKEWORD(2, 2), &wsaData);
			if (res != 0) {
				ERROR_OUT("WSAStartup failed with error: %d", res);
				return S_FAILED;
			}

			return init(wsaData);
		}

		HRESULT ProxyServer::init(WSADATA & wsaData)
		{
			if (wsaData.wVersion == 0)
			{
				WARN_OUT("The socket not init , init its now !");

				// Initialize Winsock
				s32 res = WSAStartup(MAKEWORD(2, 2), &wsaData);
				if (res != 0) {
					ERROR_OUT("WSAStartup failed with error: %d", res);
					return S_FAILED;
				}
			}

			OUTPUT_PRINT_OUT("WSAData startup with version %d", wsaData.wVersion);

			if (port == 0)
			{
				ERROR_OUT("PORT Proxy Server is zero !");
				return S_FAILED;
			}

			if (ip == IPv4(0))
			{
				ERROR_OUT("Array IP of server is zero !");
				return S_FAILED;
			}

			socketId = socket(AF_INET, SOCK_STREAM, 0);   // create a socket
			if (socketId <0) {
				ERROR_OUT("Cannot create a socket !");
				return S_FAILED;
			}

			memset(&serv_addr, 0, sizeof(serv_addr));

			serv_addr.sin_family = AF_INET;     // ip4 family 
			serv_addr.sin_addr.s_net = ip.net0;
			serv_addr.sin_addr.s_host = ip.net0;
			serv_addr.sin_addr.s_lh = ip.subnet;
			serv_addr.sin_addr.s_impno = ip.host;

			serv_addr.sin_port = htons(port);

			OUTPUT_PRINT_OUT("WSAData init with port: %d ,  IP:  %i:%i:%i:%i and socketID: %i", port, ip.net0, ip.net1, ip.host, ip.subnet , socketId);

			int binded = bind(socketId, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

			if (binded < 0) {
				ERROR_OUT("Error on binding!");
				return S_FAILED;
			}

			listen(socketId, maxCapility);  // can have maximum of 100 browser requests

		}

		HRESULT ProxyServer::onHandleRequestFromClient()
		{
			if (port == 0)
			{
				ERROR_OUT("PORT Proxy Server is zero !");
				return S_FAILED;
			}

			if (ip == IPv4(0))
			{
				ERROR_OUT("Array IP of server is zero !");
				return S_FAILED;
			}

			if (socketId < 0)
			{
				ERROR_OUT("socketId cannot create !");
				return S_FAILED;
			}


		}

	}
}
