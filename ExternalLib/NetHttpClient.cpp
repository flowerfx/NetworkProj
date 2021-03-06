﻿#include "NetHttpClient.h"
#include "base/ccUTF8.h"
NS_CC_BEGIN


NetHttpClient::NetHttpClient()
{
	_callback = nullptr;
}

NetHttpClient::~NetHttpClient()
{
	_callback = nullptr;
}

void NetHttpClient::RequestUrl(const char * url, network::HttpRequest::Type type, bool isImmediate)
{
	network::HttpRequest * request = new (std::nothrow) network::HttpRequest();
	request->setUrl(url);
	request->setRequestType(type);
	request->setResponseCallback(CC_CALLBACK_2(NetHttpClient::onHttpRequestCompleted, this));
	if (isImmediate)
	{
		request->setTag("GET immediate test1");
		network::HttpClient::getInstance()->sendImmediate(request);
	}
	else
	{
		request->setTag("GET test1");
		network::HttpClient::getInstance()->send(request);
	}
	request->release();
}

void NetHttpClient::onUpdate(float dt)
{

}

void NetHttpClient::setCallBack(const std::function<void(std::vector<char>*)> & cb)
{
	this->_callback = cb;
}

//Http Response Callback
void NetHttpClient::onHttpRequestCompleted(netlib::network::HttpClient *sender, netlib::network::HttpResponse *response)
{
	if (!response)
	{
		_callback(nullptr);
		return;
	}

	// You can get original request type from: response->request->reqType
	if (0 != strlen(response->getHttpRequest()->getTag()))
	{
		log("[NetHttpClient] : %s completed", response->getHttpRequest()->getTag());
	}

	long statusCode = response->getResponseCode();
	char statusString[64] = {};
	sprintf(statusString, "[NetHttpClient] : HTTP Status Code: %ld, tag = %s", statusCode, response->getHttpRequest()->getTag());
	log("[NetHttpClient] : response code: %ld", statusCode);

	if (!response->isSucceed())
	{
		log("[NetHttpClient] : response failed");
		log("[NetHttpClient] : error buffer: %s", response->getErrorBuffer());
		_callback(nullptr);
		return;
	}

	// dump data
	_callback(response->getResponseData());
}


NS_CC_END
