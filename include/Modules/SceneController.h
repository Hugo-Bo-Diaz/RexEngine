#ifndef SCENECONTROLLER__H
#define SCENECONTROLLER__H

#include "PartsDef.h"
#include "Part.h"
#include <vector>
#include <list>
#include <functional>
#include <functional>
#include <typeindex>
#include "../EngineElements/GameObject.h"

#define ROOMS_MAX_X 10
#define ROOMS_MAX_Y 10

#define MAX_WALLS 500

struct ObjectProperty
{
	std::string name;
	std::string str_value;
	float num_value;
	bool bool_value;
};


//do not use this class, it exists to have the factory as a template class in a list
class DLL_EXPORT FactoryBase
{
public:
	FactoryBase() {};
	FactoryBase(const char* nameInMap) {};

	virtual GameObject* CreateInstace() { return nullptr; };
	virtual GameObject* CreateInstace(std::list<ObjectProperty*>&) { return nullptr; };

	virtual std::string GetObjectMapName() { return "ERRORTYPE"; };
	virtual std::type_index GetObjectTypeIndex() { return std::type_index(typeid(this)); };
};

template<typename T>

//a factory to enable the creation of the objects, initialize with the name it will look for in the map
class DLL_EXPORT Factory : public FactoryBase
{
private:
	std::string mNameInMap;
public:
	Factory() {};
	Factory(const char* nameInMap) :mNameInMap(nameInMap) {};

	GameObject* CreateInstace() { return new T(); };
	GameObject* CreateInstace(std::list<ObjectProperty*>& lProps)
	{
		return new T(lProps);
	};

	std::type_index GetObjectTypeIndex() { return std::type_index(typeid(T)); };
	std::string GetObjectMapName() { return mNameInMap; };
};

struct collision
{
	GameObject* object;
};

//module that manages loading maps and objects
class DLL_EXPORT SceneController : public Part
{
public:
	SceneController(EngineAPI&);

	//loads a map found at that file
	void LoadMap(const char* filename);
	//removes all map elements
	void CleanMap();
	//returns the room's size
	void GetRoomSize(int& x, int& y);

	//the function given as an argument will be called every frame
	bool AssignGameLoopFunction(std::function<void()> SceneFunction);
	//the function given as an argument will be called every time a new scene is loaded
	bool AssignLoadFunction(std::function<void()> SceneFunction);


	//removes all objects from the scene
	bool Clearphysics();
	//removes a particular object from the scene
	void DeleteObject(GameObject*);
	//returns the colliders that are in the range specified, using the coordinates recieved as the center
	void GetNearbyWalls(int x, int y, int pxls_range, std::vector<RXRect*>& colliders_near);

	//returns all objects containing the type specified
	std::vector<GameObject*>* GetAllObjectsOfType(std::type_index);

	//returns all collisions with that rectangle
	void GetCollisions(RXRect* rect, std::vector<collision>& collisions);

	//adds a collider
	int AddWall(RXRect& rect);
	//removes a collider
	void DeleteWall(int id);

	//pauses all object's processing
	void PauseObjects();
	//unpauses all object's processing
	void UnPauseObjects();
	//returns true if objects have been paused
	bool isPaused();

	//adds a new object with that type and properties and returns it
	GameObject* AddObject(int x, int y, int w_col, int h_col, std::type_index lType);
	//adds an already existing object into the engine
	void AddObject(GameObject*);

	//returns the count of all objects
	int GetTotalObjectNumber();

	//adds a factory to enable the creation of the objects
	bool AddFactory(FactoryBase* lFactory);

	class SceneControllerImpl;
};

#endif // !SCENECONTROLLER__H

