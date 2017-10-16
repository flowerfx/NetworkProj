/*
chien todo: socketclient (chien.truong@gogame.net)

handle socket/websocket to connect to server
*/
#ifndef __SOCKET_CLIENT_H__
#define __SOCKET_CLIENT_H__

#include "platform/CCPlatformMacros.h"
#include "network/WebSocket.h"


NS_CC_BEGIN


class CC_DLL SocketClient : public network::WebSocket::Delegate
{
public:
	SocketClient();
	virtual ~SocketClient();

	void init(const char * server_ip, const char * server_port);
	void init(const char * server_domain);

	void closeSocket();

	/*
	* override function with websocket
	*/

	virtual void onOpen(network::WebSocket* ws)override;
	virtual void onMessage(network::WebSocket* ws, const network::WebSocket::Data& data)override;
	virtual void onClose(network::WebSocket* ws)override;
	virtual void onError(network::WebSocket* ws, const network::WebSocket::ErrorCode& error)override;

	void onUpdate(float dt);


protected:
	network::WebSocket* _socket;
	bool _isConnect;

	void _send(const unsigned char* binaryMsg, unsigned int len);
};


NS_CC_END

#endif // __SOCKET_CLIENT_H__