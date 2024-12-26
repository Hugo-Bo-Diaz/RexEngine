#include "RXpch.h"
#include "Application.h"

#include "Modules/SceneController.h"
#include "Modules/Render.h"
#include "Modules/Audio.h"
#include "Modules/Camera.h"
#include "Modules/ProgressTracker.h"
#include "Modules/Window.h"
#include "Modules/Audio.h"
#include "Utils/Logger.h"
#include "EngineElements/GameObject.h"

#include "Modules/Gui.h"

#include "SceneControllerImpl.h"
#include "RenderImpl.h"

#include "Utils/Utils.h"

SceneController::SceneController(EngineAPI& aAPI):Part("SceneController",aAPI)
{
	mPartFuncts = new SceneControllerImpl(this);
}

#pragma region IMPLEMENTATION

bool SceneController::SceneControllerImpl::Init()
{
	bool ret = true;

	for (int i = 0; i < MAX_WALLS; ++i)
	{
		walls[i] = nullptr;
	}

	return ret;
}

bool SceneController::SceneControllerImpl::Loop(float dt)
{
	//OBJECTS UPDATE
	bool ret = true;

	if (!is_paused)
	{
		for (std::list<GameObject*>::iterator it = objects.begin(); it != objects.end(); it++)
		{
			if ((*it)->active)
			{
				if (!(*it)->Loop(dt))
				{
					ret = false;
				}
			}
		}
	}

	for (std::list<GameObject*>::iterator it = objects.begin(); it != objects.end(); it++)
	{
		if (!(*it)->Render())
		{
			ret = false;
		}
	}
	//delete the current list
	for (std::unordered_set<GameObject*>::iterator it = to_delete.begin(); it != to_delete.end(); it++)
	{
		(*it)->Destroy();
		delete(*it);
		objects.erase(std::find(objects.begin(), objects.end(), *it));

	}
	to_delete.clear();

	//SCENE UPDATE
	if (SceneFunction != nullptr)
	{
		SceneFunction();
	}

	if (lMapToLoad != "")
	{
		LoadMapExecute(lMapToLoad.c_str());
		lMapToLoad = "";
	}

	for (std::vector<background_texture*>::iterator it = active_backgrounds.begin(); it != active_backgrounds.end(); it++)
	{
		mPartInst->mApp.GetImplementation<Render, Render::RenderImpl>()->RenderMapBackground((*it)->texture, (*it)->depth, (*it)->repeat_y, (*it)->parallax_x, (*it)->parallax_y);
	}

	for (std::vector<layer*>::iterator it = layers.begin(); it != layers.end(); ++it)
	{
		mPartInst->mApp.GetImplementation<Render,Render::RenderImpl>()->RenderMapLayer(*it);
	}

	return ret;
}

bool SceneController::SceneControllerImpl::CleanUp()
{
	bool ret = true;
	mPartInst->CleanMap();
	mPartInst->Clearphysics();

	for (std::list<FactoryBase*>::iterator it = mFactories.begin(); it != mFactories.end(); it++)
	{
		delete* it;
	}
	return ret;
}

bool SceneController::SceneControllerImpl::LoadTilesets(pugi::xml_node & node, const char* aMapFolder)
{
	tileset* set = new tileset(
	node.attribute("firstgid").as_int(),
	node.attribute("tilewidth").as_int(),
	node.attribute("tileheight").as_int(),
	node.attribute("columns").as_int(),
	node.attribute("tilecount").as_int());


	pugi::xml_node imagenode = node.child("image");
	std::string base_folder = aMapFolder;
	base_folder += imagenode.attribute("source").as_string();

	//load this texture
	bool lResult = mPartInst->mApp.GetModule<::Render>().LoadTexture(base_folder.c_str(), set->texture);
	tilesets.push_back(set);

	return lResult;
}

bool SceneController::SceneControllerImpl::LoadBackgroundImage(pugi::xml_node& node, const char* aMapFolder)
{
	int depth;
	float parallax_x, parallax_y;
	bool repeat_y;

	pugi::xml_node iterator;
	pugi::xml_node properties_node = node.child("properties");
	for (iterator = properties_node.first_child(); iterator; iterator = iterator.next_sibling())
	{
		std::string temp = iterator.attribute("name").as_string();
		if (temp == "depth")
		{
			depth = iterator.attribute("value").as_int(20);
		}
		else if (temp == "parallax_x")
		{
			parallax_x = iterator.attribute("value").as_float(1);
		}
		else if (temp == "parallax_y")
		{
			parallax_y = iterator.attribute("value").as_float(1);
		}
		else if (temp == "repeat_y")
		{
			repeat_y = iterator.attribute("value").as_bool(true);
		}
	}

	pugi::xml_node imagenode = node.child("image");
	const char* path = imagenode.attribute("source").as_string();

	std::string base_folder = aMapFolder;
	base_folder += path;

	//load this texture
	RexTextureID lTexture;
	bool lResult = mPartInst->mApp.GetModule<::Render>().LoadTexture(base_folder.c_str(),lTexture);

	background_texture* back = new background_texture(lTexture, parallax_x, parallax_y, depth, path, repeat_y);
	active_backgrounds.push_back(back);

	return true;
}

bool SceneController::SceneControllerImpl::LoadMapExecute(const char* filename)
{
	mPartInst->CleanMap();
	mPartInst->Clearphysics();
	mPartInst->mApp.GetImplementation<Render,Render::RenderImpl>()->ClearParticles();

	std::stringstream lStr;
	lStr << "Loading map from: " << filename;
	Logger::Console_log(LogLevel::LOG_INFO, lStr.str().c_str());

	pugi::xml_document	map_file;
	pugi::xml_node map_node;
	pugi::xml_parse_result result = map_file.load_file(filename);

	if (result == NULL)
	{
		std::string errstr = "couldn't find map ";
		errstr += filename;
		Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
		return false;
	}

	//start!
	map_node = map_file.child("map");
	pugi::xml_node iterator;

	std::string lMapFolder = GetDirectoryFromPath(filename);

	pugi::xml_node layer_node = map_node.first_child();
	for (iterator = layer_node; iterator; iterator = iterator.next_sibling())
	{
		std::string iterator_name = iterator.name();
		if (iterator_name == "tileset")
		{
			LoadTilesets(iterator, lMapFolder.c_str());
		}
		if(iterator_name == "imagelayer")
		{
			LoadBackgroundImage(iterator, lMapFolder.c_str());
		}
		if (iterator_name == "objectgroup")
		{
			bool isWallLayer = false;
			pugi::xml_node lProperties = iterator.child("properties");

			pugi::xml_node iterator_prop;
			for (iterator_prop = lProperties.first_child(); iterator_prop; iterator_prop = iterator_prop.next_sibling())
			{
				pugi::xml_attribute lName = iterator_prop.attribute("name");
				if (std::strcmp(lName.value(), "isWallLayer") == 0)
				{
					isWallLayer = iterator_prop.attribute("value").as_bool(false);
				}
			}
			if(isWallLayer)
			{
				LoadWalls(iterator);
			}
			else
			{
				LoadObjects(iterator);
			}
		}
		if (iterator_name == "layer")
		{
			LoadTiles(iterator);
		}
		if (iterator_name == "properties")
		{
			LoadMapProperties(iterator);
		}
	}

	if (LoadFunction != nullptr)
	{
		LoadFunction();
	}

	return true;
}

void SceneController::SceneControllerImpl::LoadMapProperties(pugi::xml_node & node)
{
	pugi::xml_node iterator;
	pugi::xml_node property_node = node.first_child();
	for (iterator = property_node; iterator; iterator = iterator.next_sibling())
	{
		std::string name = iterator.attribute("name").as_string();
		if (name == "music")
		{
			//App->aud->PlayMusic(iterator.attribute("value").as_int(1),500);
		}
	}
}


bool SceneController::SceneControllerImpl::LoadBackground(pugi::xml_node& imagelayer_node)
{
	pugi::xml_node image_node = imagelayer_node.first_child();
	
	//find in map properties the active backgrounds id
	std::string temp = image_node.attribute("source").as_string();

	for (std::vector<background_texture*>::iterator it = backgrounds.begin(); it != backgrounds.end(); it++)
	{
		if (temp == (*it)->path)
		{
			active_backgrounds.push_back(*it);
		}
	}
	return true;
}

bool SceneController::SceneControllerImpl::LoadWalls(pugi::xml_node& objectgroup_node)
{
	pugi::xml_node object_iterator;
	for (object_iterator = objectgroup_node.child("object"); object_iterator; object_iterator = object_iterator.next_sibling())
	{
		RXRect newwall;
		newwall.x = object_iterator.attribute("x").as_int();
		newwall.y = object_iterator.attribute("y").as_int();
		newwall.w = object_iterator.attribute("width").as_int();
		newwall.h = object_iterator.attribute("height").as_int();
		mPartInst->AddWall(newwall);
	}

	return true;
}

bool SceneController::SceneControllerImpl::LoadObjects(pugi::xml_node& objectgroup_node)
{
	pugi::xml_node object_iterator;
	for (object_iterator = objectgroup_node.child("object"); object_iterator; object_iterator = object_iterator.next_sibling())
	{
		int x = object_iterator.attribute("x").as_int();
		int y = object_iterator.attribute("y").as_int();// -object_iterator.attribute("height").as_int();//tile height inside tiled
		int w = 0;
		int h = 0;
		std::string type = object_iterator.attribute("type").as_string();

		GameObject*ret = nullptr;

		w = object_iterator.attribute("width").as_int();
		h = object_iterator.attribute("height").as_int();

		//std::map<std::string, float> lProperties;

		std::list<ObjectProperty*> lProperties;

		pugi::xml_node properties_node = object_iterator.child("properties");
		pugi::xml_node iterator;

		for (iterator = properties_node.first_child(); iterator; iterator = iterator.next_sibling())
		{
			ObjectProperty* lObjProp = new ObjectProperty();

			lObjProp->name = iterator.attribute("name").as_string();
			std::string type = iterator.attribute("type").as_string();
			float value = 0;
			if (strcmp(type.c_str(), "bool") == 0)
			{
				lObjProp->bool_value = iterator.attribute("value").as_bool();
			}
			else if (strcmp(type.c_str(), "float") == 0)
			{
				lObjProp->num_value = iterator.attribute("value").as_float();
			}
			else if (strcmp(type.c_str(), "int") == 0)
			{
				lObjProp->num_value = iterator.attribute("value").as_int();
			}
			else if (strcmp(iterator.attribute("value").as_string(""), "") != 0)
			{
				lObjProp->str_value = iterator.attribute("value").as_string("");
			}

			lProperties.push_back(lObjProp);
		}

		auto lID = GetFactory(type.c_str());
		
		if (lID != nullptr)
		{
			ret = (*lID).CreateInstace(lProperties);
			ret->mType = lID->GetObjectTypeIndex();
			ret->Engine = new EngineAPI(mPartInst->mApp);
			ret->collider.x = x;
			ret->collider.y = y;
			ret->collider.w = w;
			ret->collider.h = h;

			ret->Init();

			objects.push_back(ret);
		}
	}

	return true;
}

bool SceneController::SceneControllerImpl::LoadTiles(pugi::xml_node & tile_node)
{
	//load the main map properties
	int width = tile_node.attribute("width").as_int();
	int height = tile_node.attribute("height").as_int();
	int size = width * height;

	room_w = width;
	room_h = height;

	//read from properties

	pugi::xml_node properties_node = tile_node.child("properties");
	pugi::xml_node iterator;

	int depth = 20;
	int parallax_x = 1;
	int parallax_y = 1;
	tileset* tileset_of_layer = (*tilesets.begin());

	for (iterator = properties_node.first_child(); iterator; iterator = iterator.next_sibling())
	{
		std::string temp = iterator.attribute("name").as_string();
		if (temp == "depth")
		{
			depth = iterator.attribute("value").as_int(20);
		}
		else if (temp == "parallax_x")
		{
			parallax_x = iterator.attribute("value").as_float(1);
		}
		else if (temp == "parallax_y")
		{
			parallax_y = iterator.attribute("value").as_float(1);
		}
		else if (temp == "tileset")
		{
			int tileset = iterator.attribute("value").as_int(0);
			tileset_of_layer = tilesets[tileset];
		}
	}

	uint* data = new uint[size];
	pugi::xml_node data_node = tile_node.child("data").first_child();

	for (uint i = 0; i<size; i++)
	{
		data[i] = data_node.attribute("gid").as_uint(-1);
		if (data[i] != -1)
		{
			data[i] -= tileset_of_layer->firstgid;
		}

		data_node = data_node.next_sibling();
	}

	layer* new_layer = new layer(tileset_of_layer, data, width, height, parallax_x, parallax_y, depth, size);
	layers.push_back(new_layer);

	return true;
}

void SceneController::SceneControllerImpl::RenderDebug()
{
	for (int i = 0; i < MAX_WALLS; ++i)
	{
		if (walls[i] != nullptr)
		{
			RXRect lRect = { walls[i]->x,walls[i]->y,walls[i]->w,walls[i]->h };
			mPartInst->mApp.GetModule<::Render>().RenderRect(lRect, RXColor{ 0, 0, 255, 75 }, true, RenderQueue::RENDER_DEBUG, 0);
		}
	}

	for (std::list<GameObject*>::iterator it = objects.begin(); it != objects.end(); it++)
	{
		(*it)->RenderDebug();
		mPartInst->mApp.GetModule<::Render>().RenderRect((*it)->collider, RXColor { 0, 255, 0, 75 }, true, RenderQueue::RENDER_DEBUG, 0);
	}
}

FactoryBase* SceneController::SceneControllerImpl::GetFactory(const char* aNameInMap)
{
	for (std::list<FactoryBase*>::iterator it = mFactories.begin(); it != mFactories.end(); it++)
	{
		if (strcmp((*it)->GetObjectMapName().c_str(), aNameInMap) == 0)
		{
			return (*it);
		}
	}
	return nullptr;
}
FactoryBase* SceneController::SceneControllerImpl::GetFactory(std::type_index& aType)
{
	for (std::list<FactoryBase*>::iterator it = mFactories.begin(); it != mFactories.end(); it++)
	{
		if ((*it)->GetObjectTypeIndex() == aType)
		{
			return (*it);
		}
	}
	return nullptr;
}

#pragma endregion

#pragma region PUBLIC API

void SceneController::LoadMap(const char* filename)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	std::stringstream lStr;
	lStr << "Change map to: " << filename;
	Logger::Console_log(LogLevel::LOG_INFO, lStr.str().c_str());

	lImpl->lMapToLoad = filename;
}

bool SceneController::AssignGameLoopFunction(std::function<void()> aSceneFunction)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return false;
	}

	if (aSceneFunction != nullptr)
	{
		lImpl->SceneFunction = aSceneFunction;
		return true;
	}
	return false;
}

bool SceneController::AssignLoadFunction(std::function<void()> aLoadFunction)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return false;
	}

	if (aLoadFunction != nullptr)
	{
		lImpl->LoadFunction = aLoadFunction;
		return true;
	}
	return false;
}

void SceneController::CleanMap()
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	for (std::vector<layer*>::iterator it = lImpl->layers.begin(); it != lImpl->layers.end(); it++)
	{
		//delete (*it)->data;
		delete *it;
	}
	lImpl->layers.clear();


	for (std::vector<background_texture*>::iterator it = lImpl->active_backgrounds.begin(); it != lImpl->active_backgrounds.end(); it++)
	{
		//delete (*it)->data;
		delete* it;
	}

	lImpl->active_backgrounds.clear();
	
	for (std::vector<tileset*>::iterator it = lImpl->tilesets.begin(); it != lImpl->tilesets.end(); it++)
	{
		//delete (*it)->data;
		delete* it;
	}
	lImpl->tilesets.clear();
}

void SceneController::GetRoomSize(int& x, int& y)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	x = lImpl->room_w;
	y = lImpl->room_h;
}

void SceneController::GetNearbyWalls(int x, int y, int pxls_range, std::vector<RXRect*>& colliders_near)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	RXRect* toleration_area = new RXRect();
	*toleration_area = { x - pxls_range, y - pxls_range, pxls_range * 2, pxls_range * 2 };

	for (int i = 0; i < MAX_WALLS; ++i)
	{
		if (RXRectCollision(lImpl->walls[i], toleration_area))
		{
			colliders_near.push_back(lImpl->walls[i]);
		}
	}
}
std::vector<GameObject*>* SceneController::GetAllObjectsOfType(std::type_index info)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return nullptr;
	}

	std::vector<GameObject*>* ret = new std::vector<GameObject*>();

	for (std::list<GameObject*>::iterator it = lImpl->objects.begin(); it != lImpl->objects.end(); it++)
	{
		if ((*it)->mType == info)
		{
			ret->push_back(*it);
		}
	}
	return ret;
}

void SceneController::GetCollisions(RXRect* obj, std::vector<collision>& collisions)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	for (std::list<GameObject*>::iterator it = lImpl->objects.begin(); it != lImpl->objects.end(); it++)
	{
		if (RXRectCollision(&(*it)->collider, obj))
		{
			collision col;
			col.object = *it;
			collisions.push_back(col);
		}
	}
}

GameObject* SceneController::AddObject(int x, int y, int w_col, int h_col, std::type_index lType)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return nullptr;
	}

	return lImpl->AddObject(x, y, w_col, h_col, lType,mApp);
}

GameObject* SceneController::SceneControllerImpl::AddObject(int x, int y, int w_col, int h_col, std::type_index lType, EngineAPI& aApp)
{
	auto lID = GetFactory(lType);

	std::list<ObjectProperty*> lPropList;

	GameObject* r = nullptr;
	if (lID != nullptr)
	{
		r = (*lID).CreateInstace();
		if (r != nullptr)
		{
			r->mType = lID->GetObjectTypeIndex();
			r->collider = { 0,0,0,0 };

			r->Engine = new EngineAPI(aApp);
			r->collider.x = x;
			r->collider.y = y;
			r->collider.w = w_col;
			r->collider.h = h_col;

			r->Init();

			objects.push_back(r);
		}
		else
		{
			std::stringstream str;
			str << "Attempted to create: " << lType.name() << " as GameObject, Invalid operation, please make sure that the class inherits from GameObject!";
			Logger::Console_log(LogLevel::LOG_ERROR, str.str().c_str());
		}
	}
	else
	{
		std::stringstream str;
		str << "Attempted to create: " << lType.name() << " as GameObject, Invalid operation, Factory not registered!";
		Logger::Console_log(LogLevel::LOG_ERROR, str.str().c_str());
	}
	return r;
}

int SceneController::GetTotalObjectNumber()
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return 0;
	}
	return lImpl->objects.size();
}

void SceneController::AddObject(GameObject* lToAdd)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	if (lToAdd != nullptr)
		lImpl->objects.push_back(lToAdd);
}

int SceneController::AddWall(RXRect& rect)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return -1;
	}

	int i = 0;
	bool exit = false;

	while (i < MAX_WALLS && !exit)
	{
		if (lImpl->walls[i] == nullptr)
		{
			exit = true;

		}
		else
		{
			++i;
		}
	}

	RXRect* wall = new RXRect();
	wall->x = rect.x;
	wall->y = rect.y;
	wall->w = rect.w;
	wall->h = rect.h;

	lImpl->walls[i] = wall;

	return i;
}

void SceneController::DeleteWall(int id)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	delete lImpl->walls[id];
	lImpl->walls[id] = nullptr;
}

bool SceneController::AddFactory(FactoryBase* aFactory)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return false;
	}

	lImpl->mFactories.push_back(aFactory);
	return true;
}

bool SceneController::Clearphysics()
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return false;
	}

	Logger::Console_log(LogLevel::LOG_INFO, "Clearing UI physics");
	bool ret = true;

	for (int i = 0; i < MAX_WALLS; ++i)
	{
		if (lImpl->walls[i] != nullptr)
		{
			delete lImpl->walls[i];
			lImpl->walls[i] = nullptr;
		}
	}

	for (std::list<GameObject*>::iterator it = lImpl->objects.begin(); it != lImpl->objects.end(); it++)
	{
		(*it)->Destroy();
		delete (*it)->Engine;
		delete* it;
	}
	lImpl->objects.clear();

	return ret;
}

void SceneController::DeleteObject(GameObject* _to_delete)
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	if (std::find(lImpl->to_delete.begin(), lImpl->to_delete.end(), _to_delete) == lImpl->to_delete.end())
	{
		lImpl->to_delete.insert(_to_delete);
	}
}

bool SceneController::isPaused()
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return true;
	}

	return lImpl->is_paused;
}

void SceneController::PauseObjects()
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	lImpl->is_paused = true;
}

void SceneController::UnPauseObjects()
{
	SceneControllerImpl* lImpl = dynamic_cast<SceneControllerImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	lImpl->is_paused = false;
}

#pragma endregion