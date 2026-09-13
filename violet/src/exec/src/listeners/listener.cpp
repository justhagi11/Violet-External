
#include "executor/listeners/listener.h"

void Listener::SetContext(Websocket* server) {
	_server = server;
}
