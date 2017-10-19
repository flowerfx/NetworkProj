#include "ClientMessage.h"

namespace App
{
	namespace Network
	{
		ClientMessage::ClientMessage() :
			socket_client_id(-1),
			socket_server_id(-1),
			_thread(nullptr)
		{

		}

		ClientMessage::~ClientMessage()
		{
			DELETE(_thread);
		}


	}
}
