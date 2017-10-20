#include "ProxyServer.h"
#include "ClientMessage.h"
#include "Network/Proxy/proxy_parse.h"

namespace App
{
	namespace Network
	{
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
				ParsedHeader_set(req, "Connection", "close");
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

		HRESULT writeToserverSocket(const char* buff_to_server, int sockfd, int buff_length)
		{

			std::string temp;

			temp.append(buff_to_server);

			int totalsent = 0;

			int senteach;

			while (totalsent < buff_length) {
				if ((senteach = send(sockfd, (const char *)(buff_to_server + totalsent), buff_length - totalsent, 0)) < 0) {
					ERROR_OUT(" [writeToserverSocket] Error in sending to server ! \n");
					return S_FAILED;
				}
				totalsent += senteach;

			}
			return S_OK;
		}

		HRESULT writeToclientSocket(const char* buff_to_server, int sockfd, int buff_length)
		{

			std::string temp;

			temp.append(buff_to_server);

			int totalsent = 0;

			int senteach;

			while (totalsent < buff_length) {
				if ((senteach = send(sockfd, (const char *)(buff_to_server + totalsent), buff_length - totalsent, 0)) < 0) {
					ERROR_OUT(" [writeToclientSocket]  Error in sending to server ! \n");
					return S_FAILED;
				}
				totalsent += senteach;

			}
			return S_OK;
		}

#define MAX_BUF_SIZE  500000
		HRESULT writeToClient(int Clientfd, int Serverfd);

		void HandleRequestConnectWithClient(int Clientfd, int Serverfd, struct ParsedRequest *req)
		{
			u32 revSize = 0;
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
					hr = writeToclientSocket(buff.c_str(), Clientfd, buff.size());
					if (hr == S_FAILED)
						continue;
					state = 1;
					memset(buf, 0, sizeof buf);
				}

				//start handshake here
				if (state == 1)
				{
					//receive SSL engryption from client
					revSize = recv(Clientfd, buf, MAX_BUF_SIZE, 0);
					if (revSize <= 0)
						continue;
					state = 2;
				}
				

				if (state == 2)
				{
					//after receiving SSL from client, send its to server
					hr = writeToserverSocket(buf, Serverfd, revSize);
					if (hr == S_FAILED)
						continue;
					state = 3;
				}

				if (state == 3)
				{
					//after sending to server, wait respond from server and send its back to client
					hr = writeToClient(Clientfd, Serverfd);
					if (hr == S_FAILED)
						continue;
					state = 4;
				}

				if (state == 4)
				{
					break;
				}
			} while (true);
		}

		HRESULT writeToClient(int Clientfd, int Serverfd) {

			int iRecv;
			char buf[MAX_BUF_SIZE];

			while ((iRecv = recv(Serverfd, buf, MAX_BUF_SIZE, 0)) > 0) {
				writeToclientSocket(buf, Clientfd, iRecv);         // writing to client	    
				memset(buf, 0, sizeof buf);
				return S_OK;
			}


			/* Error handling */
			if (iRecv < 0) {
				ERROR_OUT(" [writeToClient] Yo..!! Error while recieving from server ! \n");
				return S_FAILED;
			}

			return S_FAILED;
		}

		HRESULT datafromclient(s32 sockid)
		{
			int maxbuff = MAX_BUF_SIZE;
			char buf[MAX_BUF_SIZE];

			s32 newsockfd = sockid;

			char *request_message;  // Get message from URL

			request_message = (char *)malloc(maxbuff);

			if (request_message == NULL) {
				ERROR_OUT("[datafromclient] Error in memory allocation : request_message = null !");
				return S_FAILED;
			}

			request_message[0] = '\0';

			int total_recieved_bits = 0;

			while (strstr(request_message, "\r\n\r\n") == NULL) {  // determines end of request

				int recvd = recv(newsockfd, buf, MAX_BUF_SIZE, 0);

				if (recvd < 0) {
					ERROR_OUT("[datafromclient] Error while receiving !");
					return S_FAILED;

				}
				else if (recvd == 0) {
					break;
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


				}

				strcat(request_message, buf);

			}

			if (strlen(request_message) <= 0)
			{
				ERROR_OUT("[datafromclient] request_message is zero ");
				return S_FAILED;
			}

			OUTPUT_PRINT_OUT("request message from client: ");
			OUTPUT_PRINT_OUT(request_message);


			struct ParsedRequest *req;    // contains parsed request

			req = ParsedRequest_create();

			if (ParsedRequest_parse(req, request_message, strlen(request_message)) < 0) {
				ERROR_OUT("[datafromclient] Error in parse request message..");
				return S_FAILED;
			}

			if (req->port == NULL)             // if port is not mentioned in URL, we take default as 80 
				req->port = (char *) "80";


			int iServerfd = createserverSocket(req->host, req->port);

			if (iServerfd == S_FAILED)
			{
				ParsedRequest_destroy(req);
				closesocket(newsockfd);   // close the sockets

				return S_FAILED;
			}

			HRESULT hr  ;
			if (strcmp(req->method, "CONNECT") == 0)
			{
				HandleRequestConnectWithClient(newsockfd, iServerfd, req);
				closesocket(iServerfd);
			}
			else if (strcmp(req->method, "GET") == 0)
			{
				/*final request to be sent*/
				char*  browser_req = convert_Request_to_string(req);
				hr = writeToserverSocket(browser_req, iServerfd, total_recieved_bits);
				if (hr == S_OK)
				{
					writeToClient(newsockfd, iServerfd);
				}
				closesocket(iServerfd);
			}
			else if (strcmp(req->method, "POST") == 0)
			{
				//char*  browser_req = convert_Request_to_string(req);
				hr = writeToserverSocket(request_message, iServerfd, total_recieved_bits);
				if (hr == S_OK)
				{
					do
					{
						hr = writeToClient(newsockfd, iServerfd);
						if (hr == S_OK)
							break;

					} while (true);
				}
			}

			//  Doesn't make any sense ..as to send something
			return S_OK;

		}
		struct sockaddr cli_addr;
		void handleRequestThread(int sockfd, ClientMessage * ClientMsg )
		{
			/* A browser request starts here */
			int clilen = sizeof(struct sockaddr);
			int newsockfd;
			newsockfd = accept(sockfd, &cli_addr, (socklen_t*)&clilen);
			ClientMsg->setsocket_client_id(newsockfd);
			if (newsockfd < 0) {
				ERROR_OUT("ERROR On Accepting Request ! i.e requests limit crossed");
				return;
			}
			datafromclient(newsockfd);
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

			OUTPUT_PRINT_OUT("WSAData init with port: %d ,  IP:  %i:%i:%i:%i and socketID: %i", port, ip.net0, ip.net1, ip.host, ip.subnet , socketId);

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
				p->setsocket_server_id(socketId);
				p->startRequest([](void * data) -> u32
				{
					ClientMessage * client = (ClientMessage*)data;
					handleRequestThread(client->getsocket_server_id(), client);
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
