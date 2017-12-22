
#include "Network\ProxyServer.h"

int main()
{
	App::Network::ProxyServer * proxy_server = new App::Network::ProxyServer(8888 , App::Network::IPv4(127,0,0,1));

	HRESULT hr = proxy_server->init();
	if (hr == S_OK)
	{
		while (true)
		{
			hr = proxy_server->onHandleRequestFromClient();
			if (hr == S_FAILED)
			{
				WARN_OUT("Proxy server have shut down!");
				break;
			}
		}
	}


	return 1;


}