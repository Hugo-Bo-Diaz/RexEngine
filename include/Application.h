#ifndef APPLICATION__H
#define APPLICATION__H

#include <string>
#include <list>
#include "Modules/PartsDef.h"
#include "Utils/Timer.h"

#define BASEFPS 60

enum ApplicationState
{
	CREATE,
	LOOP,
	CLEAN,
	QUIT,
	EXIT
};

class EngineAPI;

class DLL_EXPORT Application
{
public:

	//thread blocking call until the engine has finished running
	void Run();

	//creates the application and loads its configuration
	Application(const char* aConfigFile, bool& aResult);

	bool Init();

	//other properties
	float GetLastUpdateTime() { return dt; }

	EngineAPI* mAPI;
	friend class EngineAPI;

private:
	//THIS SET OF FUNCTIONS IS CALLED AUTOMATICALLY, DO NOT CALL
	float dt;
	float fps_cap = 60;
	float last_frame_ms;
	Timer update_timer;

	void LoadConfig(const char* filename);
	std::string mConfigFile;
	std::list<Part*> parts;

	bool Loop();
	bool CleanUp();
};

#endif // !APPLICATION__H