#include "ProxyServer.h"
#include "ClientMessage.h"
#include "Network/Proxy/proxy_parse.h"

namespace App
{
	namespace Network
	{
		void FREE(void * block , size_t size)
		{
			delete block;
		}

		void appExit(int d)
		{
			exit(d);
		}

		char* convert_Request_to_string(struct ParsedRequest *req)
		{

			/* Set headers */

			ParsedHeader_set(req, "Host", req->host);
			if (strcmp(req->method, "GET") == 0)
			{
				ParsedHeader_set(req, "Connection", "keep-alive");
			}
			else if (strcmp(req->method, "POST") == 0)
			{
				ParsedHeader_set(req, "Connection", "keep-alive");
			}

			int iHeadersLen = ParsedHeader_headersLen(req);

			char *headersBuf;

			headersBuf = (char*)malloc(iHeadersLen + 1);

			if (headersBuf == NULL) {
				ERROR_OUT(" [convert_Request_to_string] Error in memory allocation  of headersBuffer !");
				return NULL;
			}


			ParsedRequest_unparse_headers(req, headersBuf, iHeadersLen);
			headersBuf[iHeadersLen] = '\0';


			int request_size = strlen(req->method) + strlen(req->path) + strlen(req->version) + iHeadersLen + 4;

			char *serverReq;

			serverReq = (char *)malloc(request_size + 1);

			if (serverReq == NULL) {
				ERROR_OUT(" [convert_Request_to_string]  Error in memory allocation for serverrequest !");
				return NULL;
			}

			serverReq[0] = '\0';
			strcpy(serverReq, req->method);
			strcat(serverReq, " ");
			if (strcmp(req->method, "POST") == 0)
			{
				strcat(serverReq, req->host);
			}
			strcat(serverReq, req->path);
			strcat(serverReq, " ");
			strcat(serverReq, req->version);
			strcat(serverReq, "\r\n");
			strcat(serverReq, headersBuf);

			free(headersBuf);

			return serverReq;

		}

		u32 createserverSocket(char *pcAddress, char *pcPort) {
			struct addrinfo ahints;
			struct addrinfo *paRes;

			int iSockfd;

			/* Get address information for stream socket on input port */
			memset(&ahints, 0, sizeof(ahints));
			ahints.ai_family = AF_UNSPEC;
			ahints.ai_socktype = SOCK_STREAM;
			if (getaddrinfo(pcAddress, pcPort, &ahints, &paRes) != 0) {
				ERROR_OUT("[createserverSocket] Error in server address format !");
				return S_FAILED;
			}

			/* Create and connect */
			if ((iSockfd = socket(paRes->ai_family, paRes->ai_socktype, paRes->ai_protocol)) < 0) {
				ERROR_OUT("[createserverSocket] Error in creating socket to server ! ");
				return S_FAILED;
			}

			if (connect(iSockfd, paRes->ai_addr, paRes->ai_addrlen) < 0) {
				ERROR_OUT("[createserverSocket] Error in connecting to server ! ");
				return S_FAILED;
			}

			/* Free paRes, which was dynamically allocated by getaddrinfo */
			freeaddrinfo(paRes);

			return iSockfd;
		}

		HRESULT sendBufferViaSocket(const char* buff_to_server, u32 buff_length, s32 sockfd)
		{

			std::string temp;

			temp.append(buff_to_server);

			int totalsent = 0;

			int senteach;

			while (totalsent < buff_length) {
				if ((senteach = send(sockfd, (const char *)(buff_to_server + totalsent), buff_length - totalsent, 0)) < 0) {
					ERROR_OUT(" [sendBufferViaSocket]  Error in sending to server ! \n");
					return S_FAILED;
				}
				totalsent += senteach;

			}
			return S_OK;
		}

#define MAX_BUF_SIZE  500000
		HRESULT writeToClientViaServer(s32 Clientfd, s32 Serverfd);

		void HandleRequestConnectWithClient(ClientMessage * msg, struct ParsedRequest *req)
		{
			s32 revSize = 0;
			char buf[MAX_BUF_SIZE];
			HRESULT hr;
			
			std::string buff;
			buff.append("HTTP/");
			buff.append(req->version);
			buff.append(" 200 Connection established\r\nProxy-Agent: Mentalis Proxy Server\r\n\r\n");
					//buff.append(req->path);
			u32 state = 0;
			do {
				if (state == 0)
				{
					hr = sendBufferViaSocket(buff.c_str(), buff.size(), msg->getsocket_client_id());
					if (hr == S_FAILED)
						continue;
					state = 1;
					memset(buf, 0, sizeof buf);
				}

				//start handshake here
				if (state == 1)
				{
					//receive SSL engryption from client
					revSize = recv(msg->getsocket_client_id(), buf, MAX_BUF_SIZE, 0);
					if (revSize <= 0)
						continue;
					state = 2;
				}
				

				if (state == 2)
				{
					//after receiving SSL from client, send its to server
					hr = sendBufferViaSocket(buf, revSize, msg->getsocket_server_id());
					if (hr == S_FAILED)
						continue;
					state = 3;
				}

				if (state == 3)
				{
					//after sending to server, wait respond from server and send its back to client
					hr = writeToClientViaServer(msg->getsocket_client_id(), msg->getsocket_server_id());
					if (hr == S_FAILED)
						continue;
					state = 4;
				}

				if (state == 4)
				{
					memset(buf, 0, sizeof(buf));
					break;
				}
			} while (true);
		}

		HRESULT writeToClientViaServer(int Clientfd, int Serverfd) {

			s32 size;
			char buf[MAX_BUF_SIZE];

			while ((size = recv(Serverfd, buf, MAX_BUF_SIZE, 0)) > 0) {
				sendBufferViaSocket(buf, size, Clientfd);         // writing to client	    
				memset(buf, 0, sizeof buf);
				return S_OK;
			}


			/* Error handling */
			if (size < 0) {
				ERROR_OUT(" [writeToClientViaServer] Yo..!! Error while recieving from server ! \n");
				return S_FAILED;
			}

			return S_FAILED;
		}



		HRESULT datafromclient(ClientMessage * msg)
		{
			int maxbuff = MAX_BUF_SIZE;
			char buf[MAX_BUF_SIZE];

		
			u32 total_recieved_bits = 0;
			HRESULT hr;
			if (msg->getSSLState() == 1)
			{
				s32 size = recv(msg->getsocket_server_id(), buf, MAX_BUF_SIZE, 0);
				if (size > 0) {
					sendBufferViaSocket(buf, size, msg->getsocket_client_id());         // writing to client	    
					memset(buf, 0, sizeof(buf));
				}
				else
				{
					msg->setSSLState(0);
					return S_FAILED;
				}
				//closesocket(msg->getsocket_server_id());
				msg->setSSLState(2);
				return S_OK;
			}
			else if (msg->getSSLState() == 2)
			{
				u32 size = recv(msg->getsocket_client_id(), buf, MAX_BUF_SIZE, 0);
				if (size > 0)
				{
					hr = sendBufferViaSocket(buf, size, msg->getsocket_server_id());
					memset(buf, 0, sizeof buf);
				}
				if (hr == S_OK)
				{
					size = recv(msg->getsocket_server_id(), buf, MAX_BUF_SIZE, 0);
					if (size > 0)
					{
						hr = sendBufferViaSocket(buf, size, msg->getsocket_client_id());
						memset(buf, 0, sizeof buf);
					}
					else
					{
						msg->setSSLState(0);
						return S_FAILED;
					}
				}
				msg->setSSLState(2);
				return S_OK;
			}

			char *request_message;  // Get message from URL
			request_message = (char *)malloc(maxbuff);
			if (request_message == NULL) {
				ERROR_OUT("[datafromclient] Error in memory allocation : request_message = null !");
				return S_FAILED;
			}
			request_message[0] = '\0';

			while (strstr(request_message, "\r\n\r\n") == NULL) {
				s32 recvd = recv(msg->getsocket_client_id(), buf, MAX_BUF_SIZE, 0);

				if (recvd < 0) {
					ERROR_OUT("[datafromclient] Error while receiving !");
					FREE(request_message, maxbuff);
					return S_FAILED;

				}
				else if (recvd == 0) //nothing to get from client so close down the socket
				{
					closesocket(msg->getsocket_server_id());
					msg->setsocket_server_id(-1);
					FREE(request_message, maxbuff);
					return S_FAILED;
				}
				else {

					total_recieved_bits += recvd;

					/* if total message size greater than our string size,double the string size */
					buf[recvd] = '\0';

					if (total_recieved_bits > maxbuff) {
						maxbuff *= 2;
						request_message = (char *)realloc(request_message, maxbuff);
						if (request_message == NULL) {
							ERROR_OUT("[datafromclient] Error in memory re-allocation !");
							return S_FAILED;
						}
					}

					strcat(request_message, buf);
				}
			}
			

			if (strlen(request_message) <= 0)
			{
				FREE(request_message, maxbuff);
				ERROR_OUT("[datafromclient] request_message is zero ");
				return S_FAILED;
			}

			OUTPUT_PRINT_OUT("request message from client: ");
			OUTPUT_PRINT_OUT(request_message);

			struct ParsedRequest *req;
				// contains parsed request

			req = ParsedRequest_create();

				if (ParsedRequest_parse(req, request_message, strlen(request_message)) < 0) {
					FREE(request_message, maxbuff);
					ERROR_OUT("[datafromclient] Error in parse request message..");
					return S_FAILED;
				}

				if (req->port == NULL)             // if port is not mentioned in URL, we take default as 80 
					req->port = (char *) "80";

				if (msg->getsocket_server_id() <= 0)
				{
					s32 iServerfd = createserverSocket(req->host, req->port);
					msg->setsocket_server_id(iServerfd);
				}
				if (msg->getsocket_server_id() == S_FAILED)
				{
					ParsedRequest_destroy(req);
					//closesocket(newsockfd);   // close the sockets
					FREE(request_message, maxbuff);
					return S_FAILED;
				}

				if (strcmp(req->method, "CONNECT") == 0)
				{
					s32 revSize = -1;
					/*do
					{
						hr = sendBufferViaSocket(request_message, total_recieved_bits, msg->getsocket_server_id());
						if (hr == S_OK)
						{
							revSize = recv(msg->getsocket_server_id(), buf, MAX_BUF_SIZE, 0);
							break;
						}
					} while (true);*/
					//if (revSize <= 0)
					//{
					//	hr = sendBufferViaSocket(buf, revSize, msg->getsocket_client_id());
					//	closesocket(msg->getsocket_server_id());
					//	msg->setsocket_server_id(-1);
					//	return S_FAILED;
					//	//HandleRequestConnectWithClient(msg, req);
					//}
					//else
					{
						msg->setSSLState(1);
						HandleRequestConnectWithClient(msg, req);
					}
					//closesocket(iServerfd);
				}
				else if (strcmp(req->method, "GET") == 0)
				{
					/*final request to be sent*/
					char*  browser_req = convert_Request_to_string(req);
					
					hr = sendBufferViaSocket(browser_req, total_recieved_bits, msg->getsocket_server_id());
					if (hr == S_OK)
					{
						do
						{
							hr = writeToClientViaServer(msg->getsocket_client_id(), msg->getsocket_server_id());
							if (hr == S_FAILED)
								break;
						} while (true);
					}

					if (hr == S_FAILED)
					{
						
					}
				}
				else if (strcmp(req->method, "POST") == 0)
				{
					//char*  browser_req = convert_Request_to_string(req);
					hr = sendBufferViaSocket(request_message, total_recieved_bits, msg->getsocket_server_id());
					if (hr == S_OK)
					{
						do
						{
							hr = writeToClientViaServer(msg->getsocket_client_id(), msg->getsocket_server_id());
							if (hr == S_FAILED)
								break;

						} while (true);
					}
					//closesocket(msg->getsocket_server_id());
			}
			FREE(request_message, maxbuff);
			//  Doesn't make any sense ..as to send something
			return S_OK;

		}
		struct sockaddr cli_addr;
		void handleRequestThread(int sockfd, ClientMessage * ClientMsg )
		{
			/* A browser request starts here */
			int clilen = sizeof(struct sockaddr);
			int newsockfd = -1;
			HRESULT hr = S_OK;
			do{
				if (newsockfd < 0)
				{
					newsockfd = accept(sockfd, &cli_addr, (socklen_t*)&clilen);
					ClientMsg->setsocket_client_id(newsockfd);
					if (newsockfd < 0) {
						ERROR_OUT("ERROR On Accepting Request ! i.e requests limit crossed");
						return;
					}
				}
			    hr = datafromclient(ClientMsg);
			} while (hr == S_OK);
			closesocket(newsockfd);
		}

#define DEF_CAPILITY 100
		ProxyServer::ProxyServer() :
			port(0),
			ip(0),
			numberClientMsg(0),
			maxCapility(DEF_CAPILITY),
			socketId(-1)
		{

		}

		ProxyServer::ProxyServer(u32 _port, IPv4 _ip) :
			port(_port),
			ip(_ip),
			numberClientMsg(0),
			maxCapility(DEF_CAPILITY),
			socketId(-1)
		{

		}

		ProxyServer::~ProxyServer()
		{
			for (u32 i = 0; i < numberClientMsg; i++)
			{
				DELETE(_listClientMsg[i]);
				_listClientMsg[i] = nullptr;
			}
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
			serv_addr.sin_addr.s_host = ip.net1;
			serv_addr.sin_addr.s_lh = ip.subnet;
			serv_addr.sin_addr.s_impno = ip.host;

			serv_addr.sin_port = htons(port);

			OUTPUT_PRINT_OUT("WSAData init with port: %d ,  IP:  %i:%i:%i:%i and socketID: %i", port, ip.net0, ip.net1, ip.subnet, ip.host , socketId);

			int binded = bind(socketId, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

			if (binded < 0) {
				ERROR_OUT("Error on binding!");
				return S_FAILED;
			}

			listen(socketId, maxCapility);  // can have maximum of 100 browser requests

			//create
			numberClientMsg = 1;
			struct sockaddr cli_addr;
			for (u32 i = 0; i < numberClientMsg; i++)
			{
				_listClientMsg[i] = NEW(ClientMessage);
				ClientMessage * p  = _listClientMsg[i];
				p->setsocket_id(socketId);
				p->startRequest([](void * data) -> u32
				{
					ClientMessage * client = (ClientMessage*)data;
					handleRequestThread(client->getsocket_id(), client);
					return 1;
				});
			}

			return S_OK;

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

			for (u32 i = 0; i < numberClientMsg; i++)
			{
				ClientMessage * p = _listClientMsg[i];
				p->onCheckCLientMsg();
			}
			return S_OK;

		}

	}
}
