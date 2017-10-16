#include "NetworkMgr.h"
#include "NetHttpClient.h"

int main()
{
	NetMgr->init();
	NetMgr->getHttpClient()->RequestUrl("https://google.com.vn", netlib::network::HttpRequest::Type::GET, true);

	NetMgr->getHttpClient()->setCallBack([](std::vector<char>* strBuffer) {
		if (strBuffer != nullptr)
		{
			std::string str(strBuffer->begin() , strBuffer->end());

		}
	});

	do
	{
		NetMgr->mainLoop(1.f / 60.f);

	} while (true);

	return 1;
}