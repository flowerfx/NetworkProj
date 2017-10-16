/*
chien todo: httpclient (chien.truong@gogame.net)

handle http server connect
*/
#ifndef __NET_HTTP_CLIENT_H__
#define __NET_HTTP_CLIENT_H__

#include "platform/CCPlatformMacros.h"
#include "network/HttpClient.h"


NS_CC_BEGIN


class CC_DLL NetHttpClient : public netlib::Ref
{
public:
	NetHttpClient();
	virtual ~NetHttpClient();

	void RequestUrl(const char * url, network::HttpRequest::Type type = network::HttpRequest::Type::GET, bool isImmediate = true);

	void onUpdate(float dt);

	//Http Response Callback
	void onHttpRequestCompleted(netlib::network::HttpClient *sender, netlib::network::HttpResponse *response);

private:


};


NS_CC_END

#endif // __NET_HTTP_CLIENT_H__
