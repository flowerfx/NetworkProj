#include "NetworkMgr.h"
#include "NetHttpClient.h"
int main_proxy(int port);
int main()
{
	return main_proxy(8888);

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