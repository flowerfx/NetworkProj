
#include "Network\ProxyServer.h"

int main()
{
	App::Network::ProxyServer * proxy_server = new App::Network::ProxyServer(8888 , App::Network::IPv4::LocalHostIP());

	HRESULT res = S_OK;
	res = proxy_server->init();
	if (res == S_OK)
	{

	}


	return 1;


}