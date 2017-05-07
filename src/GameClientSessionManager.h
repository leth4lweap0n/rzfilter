#ifndef GAMECLIENTSESSIONMANAGER_H
#define GAMECLIENTSESSIONMANAGER_H

#include "NetSession/SessionServer.h"
#include "GameClientSession.h"
#include <Config/ConfigParamVal.h>
#include "NetSession/StartableObject.h"
#include <memory>

class GameClientSessionManager : public Object, public StartableObject
{
	DECLARE_CLASS(GameClientSessionManager)

public:
	GameClientSessionManager();
	~GameClientSessionManager();

	uint16_t ensureListening(std::string listenIp, uint16_t listenPort, std::string serverIp, uint16_t serverPort, Log *trafficLogger);

	virtual bool start();
	virtual void stop();
	virtual bool isStarted();

private:
	struct ServerFilter {
		GameClientSession::Parameter serverParameters;
		cval<std::string> listenIp;
		cval<int> listenPort;
		SessionServerWithParameter<GameClientSession, GameClientSession::Parameter*> sessionServer;

		ServerFilter(Log* trafficLogger)
		    : sessionServer(&serverParameters,
		                    listenIp, listenPort, nullptr, trafficLogger)
		{}

	private:
		ServerFilter(const ServerFilter &);
		ServerFilter& operator=(const ServerFilter&);
	};

	std::unordered_map<std::string, std::unique_ptr<ServerFilter>> gameClientServers;
	bool started;

	GameClientSessionManager(const GameClientSessionManager&);
	GameClientSessionManager& operator=(const GameClientSessionManager&);
};

#endif // GAMECLIENTSESSIONMANAGER_H