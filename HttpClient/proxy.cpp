//#include <bits/stdc++.h>
#include <thread>
#include <vector>
//#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string>
//#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <pthread.h>
//#include <sys/wait.h>
//#include <netdb.h>
#include "winsock2.h"
#include "winsock.h"
#include "windows.h"
#include "Ws2tcpip.h"
#include "proxy_parse.h"


void appExit(int d)
{
	exit(d);
}


char* convert_Request_to_string(struct ParsedRequest *req)
{

	/* Set headers */
	ParsedHeader_set(req, "Host", req -> host);
	ParsedHeader_set(req, "Connection", "close");

	int iHeadersLen = ParsedHeader_headersLen(req);

	char *headersBuf;

	headersBuf = (char*) malloc(iHeadersLen + 1);

	if (headersBuf == NULL) {
		fprintf (stderr," Error in memory allocation  of headersBuffer ! \n");
		appExit(1);
	}


	ParsedRequest_unparse_headers(req, headersBuf, iHeadersLen);
	headersBuf[iHeadersLen] = '\0';


	int request_size = strlen(req->method) + strlen(req->path) + strlen(req->version) + iHeadersLen + 4;
	
	char *serverReq;

	serverReq = (char *) malloc(request_size + 1);

	if(serverReq == NULL){
		fprintf (stderr," Error in memory allocation for serverrequest ! \n");
		appExit(1);
	}

	serverReq[0] = '\0';
	strcpy(serverReq, req->method);
	strcat(serverReq, " ");
	strcat(serverReq, req->path);
	strcat(serverReq, " ");
	strcat(serverReq, req->version);
	strcat(serverReq, "\r\n");
	strcat(serverReq, headersBuf);

	free(headersBuf);

	return serverReq;

}

int createserverSocket(char *pcAddress, char *pcPort) {
  struct addrinfo ahints;
  struct addrinfo *paRes;

  int iSockfd;

  /* Get address information for stream socket on input port */
  memset(&ahints, 0, sizeof(ahints));
  ahints.ai_family = AF_UNSPEC;
  ahints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(pcAddress, pcPort, &ahints, &paRes) != 0) {
   		fprintf (stderr," Error in server address format ! \n");
		return -1;
  }

  /* Create and connect */
  if ((iSockfd = socket(paRes->ai_family, paRes->ai_socktype, paRes->ai_protocol)) < 0) {
    	fprintf (stderr," Error in creating socket to server ! \n");
		return -1;
  }
  if (connect(iSockfd, paRes->ai_addr, paRes->ai_addrlen) < 0) {
    	fprintf (stderr," Error in connecting to server ! \n");
		return -1;
	}

  /* Free paRes, which was dynamically allocated by getaddrinfo */
  freeaddrinfo(paRes);

  return iSockfd;
}

void writeToserverSocket(const char* buff_to_server,int sockfd,int buff_length)
{

	std::string temp;

	temp.append(buff_to_server);
	
	int totalsent = 0;

	int senteach;

	while (totalsent < buff_length) {
		if ((senteach = send(sockfd, (const char *) (buff_to_server + totalsent), buff_length - totalsent, 0)) < 0) {
			fprintf (stderr," Error in sending to server ! \n");
			//appExit(1);
			return;
		}
		totalsent += senteach;

	}	

}

void writeToclientSocket(const char* buff_to_server,int sockfd,int buff_length)
{

	std::string temp;

	temp.append(buff_to_server);

	int totalsent = 0;

	int senteach;

	while (totalsent < buff_length) {
		if ((senteach = send(sockfd, (const char *) (buff_to_server + totalsent), buff_length - totalsent, 0)) < 0) {
			fprintf (stderr," Error in sending to server ! \n");
			appExit(1);
		}
		totalsent += senteach;

	}	

}

#define MAX_BUF_SIZE  50000

void sendRequestToServer(int Clientfd, int Serverfd, struct ParsedRequest *req)
{
	int iRecv;
	char buf[MAX_BUF_SIZE];

	while (true) {
		iRecv = recv(Serverfd, buf, MAX_BUF_SIZE, 0);
		if (iRecv >= 0)
		{
			std::string buff;
			buff.append("HTTP/1.1 200 Connection Established\nProxy-agent: Qchien-Proxy/1.1");
			//buff.append(req->path);
			writeToclientSocket(buff.c_str(), Clientfd, buff.size());
			memset(buf, 0, sizeof buf);
			break;
		}
	}

}

void writeToClient (int Clientfd, int Serverfd) {

	int iRecv;
	char buf[MAX_BUF_SIZE];

	while ((iRecv = recv(Serverfd, buf, MAX_BUF_SIZE, 0)) > 0) {
	      writeToclientSocket(buf, Clientfd,iRecv);         // writing to client	    
		memset(buf,0,sizeof buf);	
	}      


	/* Error handling */
	if (iRecv < 0) {
		fprintf (stderr,"Yo..!! Error while recieving from server ! \n");
		//appExit(1);
	}
}


void* datafromclient(void* sockid)
{
	int maxbuff = MAX_BUF_SIZE;
	char buf[MAX_BUF_SIZE];

	int newsockfd = *((int*)sockid);

	char *request_message;  // Get message from URL

	request_message = (char *) malloc(maxbuff);

	if (request_message == NULL) {
		fprintf (stderr," Error in memory allocation ! \n");
		appExit(1);
	}	

	request_message[0] = '\0';

	int total_recieved_bits = 0;

	while (strstr(request_message, "\r\n\r\n") == NULL) {  // determines end of request

	  int recvd = recv(newsockfd, buf, MAX_BUF_SIZE, 0) ;

	  if(recvd < 0 ){
	  	fprintf (stderr," Error while recieving ! \n");
		appExit(1);
	  				
	  }else if(recvd == 0) {
	  		break;
	  } else {

	  	total_recieved_bits += recvd;

	  	/* if total message size greater than our string size,double the string size */

	  	buf[recvd] = '\0';
	  	if (total_recieved_bits > maxbuff) {
			maxbuff *= 2;
			request_message = (char *) realloc(request_message, maxbuff);
			if (request_message == NULL) {
				fprintf (stderr," Error in memory re-allocation ! \n");
				appExit(1);
			}
		}


	  }

	  strcat(request_message, buf);

	}

	if (strlen(request_message) <= 0)
	{
		fprintf(stderr, "ERROR : request_message zero \n");
		return NULL;
	}

	fprintf(stdout, "request message from client \n");
	fprintf(stdout, request_message);

	struct ParsedRequest *req;    // contains parsed request

	req = ParsedRequest_create();

	if (ParsedRequest_parse(req, request_message, strlen(request_message)) < 0) {		
		fprintf (stderr,"Error in request message..\n");
		appExit(0);
	}

	if (req->port == NULL)             // if port is not mentioned in URL, we take default as 80 
		 req->port = (char *) "80";
	

	/*final request to be sent*/
	
	char*  browser_req  = convert_Request_to_string(req);		
		
	int iServerfd;

	iServerfd = createserverSocket(req->host, req->port);
	
	/* To run on IIIT proxy, comment above line and uncomment below line */
			
	//char iiitproxy[] = "172.31.1.4";
	//char iiitport[] = "8080";
	
	// iServerfd = createserverSocket(iiitproxy, iiitport);
	if (iServerfd < 0)
	{
		ParsedRequest_destroy(req);
		closesocket(newsockfd);   // close the sockets

		return NULL;
	}

	if (strcmp(req->method, "CONNECT") == 0)
	{
		writeToserverSocket(browser_req, iServerfd, total_recieved_bits);
		sendRequestToServer(newsockfd, iServerfd, req);
	}
	else
	{
		writeToserverSocket(browser_req, iServerfd, total_recieved_bits);
		writeToClient(newsockfd, iServerfd);

	}
	closesocket(iServerfd);


	//  Doesn't make any sense ..as to send something
	return NULL;

}


void handleRequestThread(int sockfd, struct sockaddr & cli_addr)
{
	/* A browser request starts here */
	int clilen = sizeof(struct sockaddr);
	int newsockfd;
	newsockfd = accept(sockfd, &cli_addr, (socklen_t*)&clilen);
	if (newsockfd < 0) {
		fprintf(stderr, "ERROR! On Accepting Request ! i.e requests limit crossed \n");
	}
	int pid = 0;
	//int pid = fork();
	if (pid == 0) {
		datafromclient((void*)&newsockfd);
		closesocket(newsockfd);
	}
	else {
		closesocket(newsockfd);     // pid =1 parent process
	}
}

int main_proxy(int port) 
{
	int iResult;
	WSADATA wsaData;
	int sockfd, newsockfd;

	struct sockaddr_in serv_addr; 
	struct sockaddr cli_addr;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}



  	sockfd = socket(AF_INET, SOCK_STREAM, 0);   // create a socket

  	if (sockfd<0) {
  		fprintf (stderr,"SORRY! Cannot create a socket ! \n");
  		return 1;
  	}

  	memset(&serv_addr,0,sizeof serv_addr);

	int  portno = port;          // argument through terminal 
 	serv_addr.sin_family = AF_INET;     // ip4 family 
  	serv_addr.sin_addr.s_addr = INADDR_ANY;  // represents for localhost i.e 127.0.0.1 
 	serv_addr.sin_port = htons(portno); 


 	int binded = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

 	if (binded <0 ) {
 		fprintf (stderr,"Error on binding! \n");
  		return 1;
 	}
  	
  	listen(sockfd, 100);  // can have maximum of 100 browser requests


	int maximum_request = 16;
	std::thread th[16];
  	while(1) {
		for (int i = 0; i < maximum_request; i++)
		{
			th[i] = std::thread(handleRequestThread, sockfd, cli_addr);
		}

		for (int i = 0; i < maximum_request; ++i) {
			th[i].join();	
		}
  		
	}

	closesocket(sockfd);


	return 0;
}