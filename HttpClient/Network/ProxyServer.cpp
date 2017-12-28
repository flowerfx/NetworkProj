#include <algorithm>
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

		void CLOSE_SOCKET(s32 socketid)
		{
			if (socketid < 0)
				return;
			closesocket(socketid);
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

		u32 createserverSocket(char *pcAddress, char *pcPort, ParsedRequest *req) {
			struct addrinfo ahints;
			struct addrinfo *paRes;

			int iSockfd;

			/* Get address information for stream socket on input port */
			memset(&ahints, 0, sizeof(ahints));
			if (strcmp(req->method, "CONNECT") == 0)
			{
				ahints.ai_family = AF_INET;
			}
			else
			{
				ahints.ai_family = AF_UNSPEC;
			}
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

		HRESULT writeToClientViaServer(int Clientfd, int Serverfd) {

			s32 size;
			char buf[MAX_BUF_SIZE];

			while ((size = recv(Serverfd, buf, MAX_BUF_SIZE, 0)) > 0) {
				sendBufferViaSocket(buf, size, Clientfd);         // writing to client	
				buf[size] = '\0';
				PRINT_OUT("[REV] size : %d , data: %s", size, buf);
				memset(buf, 0, sizeof buf);
				return S_OK;
			}


			/* Error handling */
			if (size == 0) {
				PRINT_OUT(" recv failed: %d\n", WSAGetLastError());
				ERROR_OUT(" [writeToClientViaServer] Yo..!! socket with server has close ! \n");
				return S_CLOSE;
			}
			ERROR_OUT(" [writeToClientViaServer] Yo..!! Error while recieving from server ! \n");
			return S_FAILED;
		}

		HRESULT HandleRequestConnectWithClient(ClientMessage * msg, struct ParsedRequest *req)
		{
			s32 revSize = 0;
			char buf[MAX_BUF_SIZE];
			HRESULT hr;
			u32 count = 0;
			std::string buff;
			buff.append("HTTP/");
			buff.append(req->version);
			buff.append(" 200 Connection established\r\nProxy-Agent: Mentalis Proxy Server\r\n\Connection: close\r\n\r\n");
			//buff.append(req->path);
			u32 state = 0;
			do {
				if (state == 0)
				{
					PRINT_OUT("[Connect] state 0 : send Connection established to client ");
					hr = sendBufferViaSocket(buff.c_str(), buff.size(), msg->getsocket_client_id());
					if (hr == S_FAILED)
						return hr;
					state = 1;
					memset(buf, 0, sizeof buf);
				}

				//rev data from client
				if (state == 1)
				{
					PRINT_OUT("[Connect] state 1 : receive data from client");
					revSize = recv(msg->getsocket_client_id(), buf, MAX_BUF_SIZE, 0);
					if (revSize <= 0)
					{
						PRINT_OUT("recv failed: %d\n", WSAGetLastError());
						return hr;
					}
					state = 2;
				}

				//send to server
				if (state == 2)
				{
					PRINT_OUT("[Connect] state 2: send data receive to server");
					hr = sendBufferViaSocket(buf, revSize, msg->getsocket_server_id());
					if (hr == S_FAILED)
						return hr;
					state = 3;
					memset(buf, 0, sizeof(buf));
				}

				//receive from server and write to client
				if (state == 3)
				{
					do {
						PRINT_OUT("[Connect] state 3: rev data from server and send to client");
						hr = writeToClientViaServer(msg->getsocket_client_id(), msg->getsocket_server_id());
						if (hr == S_CLOSE)
						{
							count++;
							if (count >= 2)
								return hr;
							PRINT_OUT("[Connect][CLOSE] state 3: dont receive any thing from server , break !");
							break;
						}
						else if (hr == S_FAILED)
						{
							PRINT_OUT("[Connect][ERROR] state 3: have error with socket server , return !");
							return hr;
						}

					} while (true);


					state = 1;
				}

			} while (true);
		}

		HRESULT datafromclient(s32 socket , ClientMessage * msg)
		{
			int maxbuff = MAX_BUF_SIZE;
			char buf[MAX_BUF_SIZE];
			u32 total_recieved_bits = 0;
			HRESULT hr;

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
					PRINT_OUT("recv failed: %d\n", WSAGetLastError());
					ERROR_OUT("[datafromclient] Error while receiving !");
					FREE(request_message, maxbuff);
					return S_FAILED;

				}
				else if (recvd == 0) //nothing to get from client so close down the socket
				{
					CLOSE_SOCKET(msg->getsocket_server_id());
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

				if (msg->getServer()->isDenyURL(req->host))
				{
					CLOSE_SOCKET(msg->getsocket_server_id());
					msg->setsocket_server_id(-1);
					//CLOSE_SOCKET(msg->getsocket_id());
					//msg->setsocket_id(-1);
					FREE(request_message, maxbuff);
					return S_FAILED;
				}

				if (req->port == NULL)             // if port is not mentioned in URL, we take default as 80 
					req->port = (char *) "80";

				if (msg->getsocket_server_id() <= 0)
				{
					s32 iServerfd = createserverSocket(req->host, req->port , req);
					msg->setsocket_server_id(iServerfd);
				}
				if (msg->getsocket_server_id() == S_FAILED)
				{
					ParsedRequest_destroy(req);
					//CLOSE_SOCKET(newsockfd);   // close the sockets
					FREE(request_message, maxbuff);
					return S_FAILED;
				}

				if (strcmp(req->method, "CONNECT") == 0)
				{				
					msg->setSSLState(0);
					hr = HandleRequestConnectWithClient(msg, req);
					if (hr == S_FAILED)
					{
						CLOSE_SOCKET(msg->getsocket_server_id());
						msg->setsocket_server_id(-1);
						FREE(request_message, maxbuff);
						return S_FAILED;
					}
					else if (hr == S_CLOSE)
					{
						CLOSE_SOCKET(msg->getsocket_server_id());
						msg->setsocket_server_id(-1);
						FREE(request_message, maxbuff);
						return S_FAILED;
					}
				}
				else if (strcmp(req->method, "GET") == 0)
				{
					/*final request to be sent*/
					char*  browser_req = convert_Request_to_string(req);
					
					hr = sendBufferViaSocket(browser_req, total_recieved_bits, msg->getsocket_server_id());
					if (hr == S_OK)
					{
						bool have_rev = false;
						do
						{
							hr = writeToClientViaServer(msg->getsocket_client_id(), msg->getsocket_server_id());
							if (hr == S_FAILED || hr == S_CLOSE)
								break;
							else
								have_rev = true;
						} while (true);

						if (have_rev == false)
						{
							CLOSE_SOCKET(msg->getsocket_server_id());
							msg->setsocket_server_id(-1);
							FREE(request_message, maxbuff);
							return S_FAILED;
						}
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
							if (hr == S_FAILED || hr == S_CLOSE)
							{
								CLOSE_SOCKET(msg->getsocket_server_id());
								msg->setsocket_server_id(-1);
								break;
							}

						} while (true);
					}
					//CLOSE_SOCKET(msg->getsocket_server_id());
			}
			FREE(request_message, maxbuff);
			//  Doesn't make any sense ..as to send something
			return S_OK;

		}
	
		void handleRequestThread(int sockfd, ClientMessage * ClientMsg )
		{
			/* A browser request starts here */
			struct sockaddr cli_addr;
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
			    hr = datafromclient(sockfd, ClientMsg);
			} while (hr == S_OK);
			CLOSE_SOCKET(newsockfd);
			//WSACleanup();
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
			numberClientMsg = 100;
			struct sockaddr cli_addr;
			for (u32 i = 0; i < numberClientMsg; i++)
			{
				_listClientMsg[i] = new ClientMessage(this);
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

		const std::vector<std::string> explode(const std::string& s, const char& c)
		{
			std::string buff{ "" };
			std::vector<std::string> v;

			for (auto n : s)
			{
				if (n != c) buff += n; else
					if (n == c && buff != "") { v.push_back(buff); buff = ""; }
			}
			if (buff != "") v.push_back(buff);

			return v;
		}

		void ProxyServer::onReadDenyListFromFile(const char * name)
		{
			u64 size = 0;
			char * buffer;
			FILE * fp = fopen(name , "rb");
			if (!fp)
			{
				ERROR_OUT("[ProxyServer] cannot read file %s", name);
				return;
			}

			// obtain file size:
			fseek(fp, 0, SEEK_END);
			size = ftell(fp);
			rewind(fp);

			// allocate memory to contain the whole file:
			buffer = (char*)malloc(sizeof(char)*size);
			if (buffer == NULL) { fputs("Memory error", stderr); exit(2); }

			// copy the file into the buffer:
			size_t result = fread(buffer, 1, size, fp);
			if (result != size) { fputs("Reading error", stderr); exit(3); }

			/* the whole file is now loaded in the memory buffer. */

			// terminate
			fclose(fp);
			buffer[size] = '\0';
			std::string str = buffer;

			str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
			denyList = explode(str, '\n');
		}

		bool ProxyServer::isDenyURL(const char * url)
		{
			if (denyList.size() <= 0)
				return false;

			for (auto it : denyList)
			{
				if (it == url)
				{
					return true;
				}
			}

			return false;
		}

	}
}
