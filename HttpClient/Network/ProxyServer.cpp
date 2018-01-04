#include <algorithm>
#include "ProxyServer.h"
#include "ClientMessage.h"
#include "Network/Proxy/proxy_parse.h"

namespace App
{
	namespace Network
	{
		void handleRequestThread(int sockfd, ClientMessage * ClientMsg);
		void FREE(void * block , size_t size)
		{
			delete block;
		}

		void CLOSE_SOCKET(s32 socketid, ClientMessage * msg = nullptr)
		{
			if (socketid < 0)
				return;

			if (msg)
			{
				if (msg->getsocket_client_id() == socketid)
				{
					msg->setsocket_client_id(-1);
				}
				else if (msg->getsocket_client_id_ssl() == socketid)
				{
					msg->setsocket_client_id_ssl(-1);
				}
				else if (msg->getsocket_server_id() == socketid)
				{
					msg->setsocket_server_id(-1);
				}
				else if (msg->getsocket_server_id_ssl() == socketid)
				{
					msg->setsocket_server_id_ssl(-1);
				}
			}

			if (msg && msg->getParent() != nullptr)
			{
				//return;
			}

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
			//ahints.ai_family = AF_UNSPEC;
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

		HRESULT HandleBufferFromServer(AnalyticBuffer * ana, char * buffer, s32 size)
		{
			if (size <= 0 || buffer == 0)
				return S_FAILED;

			if (size < 4)
				return S_FAILED;

			if (!(buffer[0] == 'H' && buffer[1] == 'T' && buffer[2] == 'T' && buffer[3] == 'P'))
				return S_FAILED;

			std::string str(buffer, size);
			std::size_t found = 0;
			std::size_t found_next = 0;
			found = str.find("/");
			ana->httpType = str.substr(0, found);
			found++;
			found_next = str.find(" ", found);
			ana->httpVersion = str.substr(found, found_next - found);
			found = found_next;
			found++;
			found_next = str.find(" ", found);
			ana->httpCode = atoi((str.substr(found, found_next - found)).c_str());
			found = found_next;
			found++;

			found_next = str.find("\r\n", found);
			found = found_next;
			found_next = str.find("Content-Type:", found);
			found = found_next;
			found_next = str.find(" ", found);
			found = found_next;
			found++;
			found_next = str.find("\r\n", found);
			ana->contentType = str.substr(found, found_next - found);
			found = found_next;
			found_next = str.find("Content-Length:", found);
			found = found_next;
			found_next = str.find(" ", found);
			found = found_next;
			found++;
			found_next = str.find("\r\n", found);
			ana->sizeCharacter = _atoi64((str.substr(found, found_next - found)).c_str());
			found = found_next;
			found_next = str.find("Connection:", found);
			found = found_next;
			found_next = str.find(" ", found);
			found = found_next;
			found++;
			found_next = str.find("\r\n", found);
			std::string str_type_connection = str.substr(found, found_next - found);
			if (str_type_connection == "Close")
			{
				ana->connectionType = CLOSE;
			}
			else
			{
				ana->connectionType = KEEP_ALIVE;
			}
			found = found_next;
			found_next = str.find("\r\n", found);
			found = found_next;
			found++;
			ana->body = str.substr(found, found_next - found);

			return S_OK;
		}

		HRESULT writeToClientViaServer(int Clientfd, int Serverfd) {

			s32 size;
			char buf[MAX_BUF_SIZE];

			while ((size = recv(Serverfd, buf, MAX_BUF_SIZE, 0)) > 0) {
				//
				buf[size] = '\0';
				PRINT_OUT("[REV] size : %d , data: %s", size, buf);
				sendBufferViaSocket(buf, size, Clientfd);         // writing to client	
				memset(buf, 0, sizeof buf);
				return S_OK;
			}


			/* Error handling */
			if (size == 0) {
				PRINT_OUT(" recv failed: %d\n", WSAGetLastError());
				ERROR_OUT(" Yo..!! socket with server has close ! \n");
				return S_CLOSE;
			}
			ERROR_OUT("Yo..!! Error while recieving from server ! \n");
			return S_FAILED;
		}

		HRESULT getSocketIsBlocking(s32 socket)
		{
			// If iMode = 0, blocking is enabled; 
			// If iMode != 0, non-blocking mode is enabled.
			unsigned long iMode;
			s32 iResult = ioctlsocket(socket, FIONBIO, &iMode);
			if (iResult != NO_ERROR){
				printf("ioctlsocket failed with error: %ld\n", iResult);

				return S_FAILED;
			}

			if (iMode == 0)
			{
				return S_FAILED;
			}
			return S_OK;
		}

		HRESULT HandleRequestConnectWithClient(ClientMessage * msg, struct ParsedRequest *req)
		{
			s32 revSize = 0;
			char buf[MAX_BUF_SIZE];
			HRESULT hr;
			u32 count = 0;
			std::string buff;
			buff.append("HTTP/ ");
			buff.append(req->version);
			buff.append(" 200 Connection established\nProxy-Agent: Test-Proxy-Server\n");
			//buff.append(req->path);
			u32 state = 0;
			do {
				if (state == 0)
				{
					PRINT_OUT("[Connect] state 0 : send Connection established to client...");
					hr = sendBufferViaSocket(buff.c_str(), buff.size(), msg->getsocket_client_id());
					if (hr == S_FAILED)
					{
						ERROR_OUT("[Connect] state 0 : failed with errror %d ", WSAGetLastError());
						return hr;
					}
					state = 1;
					memset(buf, 0, sizeof buf);
					PRINT_OUT("[Connect] state 1 : finish !\n");
				}

				//rev data from client
				if (state == 1)
				{

					PRINT_OUT("[Connect] state 1 : receive data from client...");
					//if (getSocketIsBlocking(msg->getsocket_client_id()) == S_FAILED)
					//	return S_FAILED;

					revSize = recv(msg->getsocket_client_id(), buf, MAX_BUF_SIZE, 0);
					PRINT_OUT("...with size %d" , revSize);
					if (revSize <= 0)
					{
						ERROR_OUT("[Connect] state 1 : failed with errror %d ", WSAGetLastError());
						return hr;
					}
					state = 2;
					PRINT_OUT("[Connect] state 1 : finish !\n");
				}

				//send to server
				if (state == 2)
				{
					PRINT_OUT("[Connect] state 2: send data receive with size %d to server..." , revSize);
					hr = sendBufferViaSocket(buf, revSize, msg->getsocket_server_id());
					if (hr == S_FAILED)
					{
						ERROR_OUT("[Connect] state 2 : failed with errror %d ", WSAGetLastError());
						return hr;
					}
					state = 3;
					memset(buf, 0, sizeof(buf));
					PRINT_OUT("[Connect] state 2 : finish !\n");
				}

				//receive from server 
				if (state == 3)
				{
					do
					{
						PRINT_OUT("[Connect] state 3: rev data from server...");

						revSize = recv(msg->getsocket_server_id(), buf, MAX_BUF_SIZE, 0);
						PRINT_OUT("...with size %d", revSize);
						PRINT_OUT("[Connect] state 3 : have errror %d ", WSAGetLastError());
						if (revSize > 0)
						{
							PRINT_OUT("[Connect] state 3: send to client data rev...");
							hr = sendBufferViaSocket(buf, revSize, msg->getsocket_client_id());         // writing to client
							if (hr == S_FAILED)
							{
								ERROR_OUT("[Connect] state 3 : failed with errror %d ", WSAGetLastError());
								return hr;
							}
							state = 1;
						}
						else if (revSize == 0)
						{
							PRINT_OUT("[Connect][CLOSE] state 3: dont receive any thing from server , return !");
							if (state == 1)
							{
								break;
							}
							else
							{
								return S_CLOSE;
							}
						}
						else
						{
							PRINT_OUT("[Connect][ERROR] state 3: have error with socket server , return !");
							return S_FAILED;
						}
					} while (true);
					PRINT_OUT("[Connect] state 3 : finish ! \n");
				}

				//sent to client
				if (state == 4)
				{
					PRINT_OUT("[Connect] state 4: send to client data rev...");
					hr = sendBufferViaSocket(buf, revSize, msg->getsocket_client_id());         // writing to client	
					if (hr == S_FAILED)
					{
						ERROR_OUT("[Connect] state 4 : failed with errror %d ", WSAGetLastError());
						return hr;
					}
					state = 1;
					memset(buf, 0, sizeof(buf));
					PRINT_OUT("[Connect] state 4 : finish !\n\n\n");
				}

			} while (true);
		}

		HRESULT datafromclient(s32 socket , ClientMessage * msg)
		{
			if (msg->getSSLState() == 1)
			{
				//create a connection with  to handle
				//create
				s32 numberClientMsg = 2;
				struct sockaddr cli_addr;
				for (u32 i = 0; i < numberClientMsg; i++)
				{
					ClientMessage * p = new ClientMessage(msg->getServer());
					p->setsocket_id(msg->getsocket_id());
					p->startRequest([](void * data) -> u32
					{
						ClientMessage * client = (ClientMessage*)data;
						handleRequestThread(client->getsocket_id(), client);
						return 1;
					});

					msg->addChild(p);
				}

				msg->setSSLState(2);
				return S_OK;
			}
			else if (msg->getSSLState() == 2)
			{

				msg->onCheckCLientMsg();

				return S_OK;
			}
			else if (msg->getSSLState() == 3)
			{
				msg->setSSLState(0);
				CLOSE_SOCKET(msg->getsocket_server_id_ssl(), msg);
				msg->deleteAllChild();
				return S_FAILED;
			}


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
					CLOSE_SOCKET(msg->getsocket_server_id() , msg);
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
					CLOSE_SOCKET(msg->getsocket_server_id() , msg);
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
					FREE(request_message, maxbuff);
					return S_FAILED;
				}

				if (strcmp(req->method, "CONNECT") == 0)
				{
					//create a tunnel with client
					msg->setSSLState(1);
					hr = HandleRequestConnectWithClient(msg, req);
					if (hr == S_FAILED)
					{
						CLOSE_SOCKET(msg->getsocket_server_id(), msg);
						FREE(request_message, maxbuff);
						return S_FAILED;
					}
					else if (hr == S_CLOSE)
					{
						// keep the connection tunnel
						msg->setsocket_client_id_ssl(msg->getsocket_client_id());
						msg->setsocket_server_id_ssl(msg->getsocket_server_id());
						//reset the id socket
						msg->setsocket_server_id(-1);
						msg->setsocket_client_id(-1);
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
							CLOSE_SOCKET(msg->getsocket_server_id() , msg);
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
					hr = sendBufferViaSocket(request_message, total_recieved_bits, msg->getsocket_server_id());
					if (hr == S_OK)
					{
						do
						{
							s32 size = 0;
							char buffer[MAX_BUF_SIZE];
							buffer[0] = '\0';
							do
							{
								char * buff_rev = new char[MAX_BUF_SIZE];
								s32 size_rev = recv(msg->getsocket_server_id(), buff_rev, MAX_BUF_SIZE , 0);
								if (size_rev <= 0)
								{
									delete buff_rev;
									break;
								}
								else
								{
									buff_rev[size_rev] = '\0';
									strcat(buffer, buff_rev);
									size += size_rev;

									delete  buff_rev;
								}
							} while (true);

							AnalyticBuffer ana;
							HRESULT hr_buff;
							if (size > 0)
							{
								buffer[size] = '\0';
								PRINT_OUT("[REV] size : %d , data: %s", size, buffer);

								hr_buff = HandleBufferFromServer(&ana , buffer, size);

								sendBufferViaSocket(buffer, size, msg->getsocket_client_id());         // writing to client	
								memset(buffer, 0, sizeof(buffer));
							}	
							else 
							{
								CLOSE_SOCKET(msg->getsocket_server_id(), msg);
								if (hr_buff == S_OK)
								{
									if (ana.connectionType == CLOSE)
									{
										FREE(request_message, maxbuff);
										if (msg->getParent())
										{
											msg->getParent()->setSSLState(3);
										}
										return S_FAILED;
									}
								}

								break;
							}

						} while (true);
					}
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
			CLOSE_SOCKET(newsockfd, ClientMsg);
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

			//socketId = socket(AF_INET, SOCK_SEQPACKET, IPPROTO_TCP);   // create a socket
			socketId = socket(AF_INET, SOCK_STREAM, 0);
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
