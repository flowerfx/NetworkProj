#include "SocketClient.h"
#include "NetworkMgr.h"
#include "base/ccUTF8.h"
NS_CC_BEGIN

SocketClient::SocketClient()
{
	_socket = nullptr;
	_isConnect = false;
}

SocketClient::~SocketClient()
{
	CC_SAFE_DELETE(_socket);
}

void SocketClient::init(const char * server_ip, const char * server_port)
{
	CC_SAFE_DELETE(_socket);
	_socket = new network::WebSocket();
	if (!_socket->init(*this, FORMAT("%s:%s", server_ip,server_port).c_str()))
	{
		CC_SAFE_DELETE(_socket);
	}
	_isConnect = false;
}

void SocketClient::init(const char * server_domain)
{

}

void SocketClient::closeSocket()
{
	_isConnect = false;
	CC_SAFE_DELETE(_socket);
}

void SocketClient::onOpen(network::WebSocket* ws)
{
	CCLOG("[SocketClient] Socket Open");
	_isConnect = true;
}

void SocketClient::onClose(network::WebSocket* ws)
{
	CCLOG("[SocketClient] Socket Close");
	_isConnect = false;
	//p_buff_stream = nullptr;
}

void SocketClient::onError(network::WebSocket* ws, const network::WebSocket::ErrorCode& error)
{
	CCLOG("[SocketClient] Socket Error , code : %d" , (int)error);
}

void SocketClient::onMessage(network::WebSocket* ws, const network::WebSocket::Data& data)
{
	CCLOG("[SocketClient] Socket Message");


	CCLOG("[SocketClient] return msg: %s", (unsigned char*)data.bytes);

	//dua vao buffer
	//p_buff_stream->pushData((unsigned char*)data.bytes, data.len);

}

void SocketClient::_send(const unsigned char* binaryMsg, unsigned int len)
{
	CCASSERT(_socket, "[SocketClient] socket cannot be null");
	_socket->send(binaryMsg, len);
}

void SocketClient::onUpdate(float dt)
{

}


NS_CC_END