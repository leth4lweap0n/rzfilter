#include "Config/GlobalCoreConfig.h"
#include "Core/CrashHandler.h"
#include "Core/EventLoop.h"
#include "Core/Log.h"
#include "Database/DbConnectionPool.h"
#include "GlobalConfig.h"
#include "LibRzuInit.h"

#include "Console/ConsoleSession.h"
#include "NetSession/BanManager.h"
#include "NetSession/ServersManager.h"
#include "NetSession/SessionServer.h"

#include "AuthClientSession.h"
#include "FilterManager.h"
#include "GameClientSessionManager.h"

void runServers(Log* trafficLogger);

void onTerminate(void* instance) {
	ServersManager* serverManager = (ServersManager*) instance;

	if(serverManager)
		serverManager->forceStop();
}

int main(int argc, char** argv) {
	LibRzuScopedUse useLibRzu;
	GlobalConfig::init();
	BanManager::registerConfig();

	ConfigInfo::get()->init(argc, argv);

	Log mainLogger(GlobalCoreConfig::get()->log.enable,
	               GlobalCoreConfig::get()->log.level,
	               GlobalCoreConfig::get()->log.consoleLevel,
	               GlobalCoreConfig::get()->log.dir,
	               GlobalCoreConfig::get()->log.file,
	               GlobalCoreConfig::get()->log.maxQueueSize);
	Log::setDefaultLogger(&mainLogger);

	Log trafficLogger(CONFIG_GET()->trafficDump.enable,
	                  CONFIG_GET()->trafficDump.level,
	                  CONFIG_GET()->trafficDump.consoleLevel,
	                  CONFIG_GET()->trafficDump.dir,
	                  CONFIG_GET()->trafficDump.file,
	                  GlobalCoreConfig::get()->log.maxQueueSize);

	ConfigInfo::get()->dump();

	int clientEpic, gameEpic;
	clientEpic = CONFIG_GET()->client.epic;
	gameEpic = CONFIG_GET()->server.epic;
	Object::logStatic(Object::LL_Info,
	                  "main",
	                  "Client epic: %x.%x.%x (0x%06X), Server epic: %x.%x.%x (0x%06X)\n",
	                  clientEpic >> 16,
	                  (clientEpic >> 8) & 0xFF,
	                  clientEpic & 0xFF,
	                  clientEpic,
	                  gameEpic >> 16,
	                  (gameEpic >> 8) & 0xFF,
	                  gameEpic & 0xFF,
	                  gameEpic);
	Object::logStatic(Object::LL_Info,
	                  "main",
	                  "Target auth: %s:%d\n",
	                  CONFIG_GET()->server.ip.get().c_str(),
	                  CONFIG_GET()->server.port.get());

	FilterManager::init();
	runServers(&trafficLogger);

	// Make valgrind happy
	EventLoop::getInstance()->deleteObjects();

	return 0;
}

void runServers(Log* trafficLogger) {
	ServersManager serverManager;
	std::unique_ptr<FilterManager> filterManager;
	std::unique_ptr<FilterManager> converterFilterManager;

	if(CONFIG_GET()->filter.filterModuleEnable)
		filterManager.reset(new FilterManager(CONFIG_GET()->filter.filterModuleName));
	if(CONFIG_GET()->filter.converterFilterModuleEnable)
		converterFilterManager.reset(new FilterManager(CONFIG_GET()->filter.converterFilterModuleName));

	GameClientSessionManager gameClientSessionManager(filterManager.get(), converterFilterManager.get());
	BanManager clientBanManager;

	AuthClientSession::InputParameters parameters = {
	    &gameClientSessionManager, filterManager.get(), converterFilterManager.get()};

	SessionServerWithParameter<AuthClientSession, AuthClientSession::InputParameters> clientSessionServer(
	    parameters,
	    CONFIG_GET()->client.listener.listenIp,
	    CONFIG_GET()->client.listener.port,
	    &CONFIG_GET()->client.listener.idleTimeout,
	    trafficLogger,
	    &clientBanManager);

	serverManager.addServer("clients", &clientSessionServer, &CONFIG_GET()->client.listener.autoStart);
	serverManager.addServer("gameClientSessionManager", &gameClientSessionManager, nullptr);
	ConsoleServer consoleServer(&serverManager);

	serverManager.start();

	CrashHandler::setTerminateCallback(&onTerminate, &serverManager);

	EventLoop::getInstance()->run(UV_RUN_DEFAULT);

	CrashHandler::setTerminateCallback(nullptr, nullptr);
}
