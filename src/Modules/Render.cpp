#include "RXpch.h"
#include "Modules/Render.h"
#include "Application.h"
#include "Modules/Window.h"
#include "Modules/Camera.h"
#include "EngineElements/ParticleEmitter.h"
#include "Utils/Logger.h"

#include "RenderImpl.h"
#include "CameraImpl.h"
#include "WindowImpl.h"
#include "EngineAPI.h"

#include "Renderers/OpenGLRenderer.h"
#include "Renderers/SDLRenderer.h"

#include <glm/include/glm/gtc/type_ptr.hpp>

Render::Render(EngineAPI& aAPI, const char* aRenderType) :Part("Render",aAPI)
{ 
	if (strcmp("SDL", aRenderType) == 0)
	{
		mPartFuncts = new SDLRenderer(this);
	}
	else if (strcmp("OpenGL", aRenderType) == 0)
	{
		mPartFuncts = new OpenGL2DRenderer(this);
	}
	else
	{
		Logger::Console_log(LogLevel::LOG_ERROR,"Could not find the Render type, please check the configuration file");
		mPartFuncts = nullptr;
	}
}

#pragma region IMPLEMENTATION

bool Render::RenderImpl::LoadConfig(pugi::xml_node& config_node)
{
	pugi::xml_node& lRenderType = config_node.child("RenderEngine");

	bool ret = true;

	pugi::xml_node& color_node = config_node.child("bkg_color");

	backgroundColor.r = color_node.attribute("r").as_float(0);
	backgroundColor.g = color_node.attribute("g").as_float(0);
	backgroundColor.b = color_node.attribute("b").as_float(0);
	backgroundColor.a = color_node.attribute("a").as_float(0);

	// load flags
	Uint32 flags = SDL_RENDERER_ACCELERATED;

	if (config_node.child("vsync").attribute("on").as_bool(true))//if config vsync
	{
		flags |= SDL_RENDERER_PRESENTVSYNC;
		Logger::Console_log(LogLevel::LOG_INFO, "Using vsync");
	}
	//SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	Logger::Console_log(LogLevel::LOG_INFO, "Create SDL rendering context");

	return ret;
}

bool Render::RenderImpl::CreateConfig(pugi::xml_node& config_node)
{
	bool ret = true;

	pugi::xml_node & color_node = config_node.append_child("bkg_color");
	color_node.append_attribute("r") = 0.0;
	color_node.append_attribute("g") = 0.0;
	color_node.append_attribute("b") = 0.0;
	color_node.append_attribute("a") = 0.0;

	//vsync will be on by default
	config_node.append_child("vsync").append_attribute("on") = true;
	return ret;
}


std::priority_queue<BlitItem*, std::vector<BlitItem*>, Comparer>* Render::RenderImpl::GetQueue(RenderQueue aQueue)
{
	switch (aQueue)
	{
	case RENDER_GAME:
		return &allQueue;
		break;
	case RENDER_UI:
		return &uiQueue;
		break;
	case RENDER_DEBUG:
		return &debugQueue;
		break;
	default:
		break;
	}

}

RexFont* Render::RenderImpl::GetFont(RexFontID aFontID)
{
	for (std::vector<RexFont*>::iterator it = mFonts.begin(); it != mFonts.end(); it++)
	{
		if ((*it)->mFontID == aFontID)
		{
			return ((*it));
		}
	}
	return nullptr;
}

bool Render::RenderImpl::Init()
{
	bool ret = true;
	ret = InitRenderer(*mPartInst->mApp.GetImplementation<Window, Window::WindowImpl>(), mPartInst->mApp.GetModule<Window>());

	return ret;
}

bool Render::RenderImpl::Loop(float dt)
{
	bool ret = true;
	//SDL_RenderClear(renderer);

	UpdateParticles(dt);

	mDrawCallsLastFrame += dt/1000;
	if (mDrawCallsLastFrame > 1.0f)
		mDrawCallsLastFrame = 0;

	glm::mat4x4 lViewProjection = mPartInst->mApp.GetImplementation<Camera, Camera::CameraImpl>()->GetOrthoMatrix() * mPartInst->mApp.GetImplementation<Camera, Camera::CameraImpl>()->GetViewMatrix();

	SetViewProjectionMatrix(lViewProjection);
	SetDefaultShader();

	for (int i = 0; i < RenderQueue::RENDER_MAX; i++)
	{
		std::priority_queue<BlitItem*, std::vector<BlitItem*>, Comparer>* lQueue = GetQueue((RenderQueue)i);
		while (!lQueue->empty())
		{
			BlitItem* lNextItem = lQueue->top();
			lQueue->pop();

			lNextItem->Blit(*this, mPartInst->mApp.GetModule<Camera>(), mPartInst->mApp.GetModule<Window>());
			delete lNextItem;
		}
	}

	UpdateRender(dt);
	return ret;
}


void Render::RenderImpl::RenderMapLayer(layer* layer)
{
	BlitLayer* it = new BlitLayer(layer);
	it->depth = layer->depth;
	allQueue.push(it);
}

void Render::RenderImpl::RenderParticleEmitter(ParticleEmitter* layer, RenderQueue aRenderQueue)
{
	BlitParticles* it = new BlitParticles(layer);
	it->depth = layer->depth;
	GetQueue(aRenderQueue)->push(it);
}

RexTexture* Render::RenderImpl::GetTexture(RexTextureID aTexture)
{
	for (std::vector<RexTexture*>::iterator it = mTextures.begin(); it != mTextures.end(); it++)
	{
		if ((*it)->mID == aTexture)
		{
			return ((*it));
		}
	}
	return nullptr;
}

void Render::RenderImpl::RenderMapBackground(RexTextureID aTexID, int depth, bool repeat_y, float parallax_factor_x, float parallax_factor_y)
{
	BlitBackground* it = new BlitBackground(aTexID, depth, repeat_y, parallax_factor_x, parallax_factor_y);
	//order the elements
	allQueue.push(it);
}

#pragma endregion

#pragma region PUBLIC API

long Render::GetDrawCallsLastFrame() 
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
	}
	return lImpl->mDrawCallsLastFrame;
}

void Render::CountDrawCall()
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
	}
	lImpl->mDrawCallsLastFrame++;
}

bool Render::LoadTexture(const char* aPath, RexTextureID& aResultID)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
	}

	return lImpl->LoadTexture(aPath, aResultID);
}

bool Render::DestroyTexture(RexTextureID aResultID)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
	}

	return lImpl->DestroyTexture(aResultID);
}

bool Render::LoadShader(const char* aPathVertex, const char* aPathFragment, RexShaderID& aShader)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
	}

	return lImpl->LoadShader(aPathVertex,aPathFragment, aShader);
}

bool Render::SetShader(RexShaderID aShader)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
	}

	return lImpl->SetShader(aShader);
}

void Render::SetDefaultShader()
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
	}

	lImpl->SetDefaultShader();
}

bool Render::DestroyShader(RexShaderID aResultID)
{
	return false;
}

bool Render::LoadFont(const char* aPath, const RXColor& aColor, int size, RexFontID& aFontID)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return 0;
	}

	for (std::vector<RexFont*>::iterator it = lImpl->mFonts.begin(); it != lImpl->mFonts.end(); it++)
	{
		if (std::strcmp((*it)->name.c_str(), aPath) == 0 && (*it)->size == size)
		{
			aFontID = (*it)->mFontID;
			return true;
		}
	}

	std::string lFullPath = aPath;
	std::string lExtension = lFullPath.substr(lFullPath.find_last_of(".") + 1).c_str();

	bool lResult = false;

	if (strcmp(lExtension.c_str(), XMLFONTEXTENSION) == 0) {
		lResult = lImpl->LoadFontXML(aPath, aColor, size, aFontID);
	}
	else if (strcmp(lExtension.c_str(), TTFFONTEXTENSION) == 0) {
		lResult = lImpl->LoadFontTTF(aPath, aColor, size, aFontID);
	}
	else
	{
		std::stringstream lStream;
		lStream << "Could not load font from :" << aPath << " - incorrect format";
		Logger::Console_log(LogLevel::LOG_ERROR, lStream.str().c_str());
	}

	return lResult;
}

void Render::GetTextSize(RexFontID aFontID, const char* string, int& w, int& y)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	RexFont* lFont = lImpl->GetFont(aFontID);

	if (lFont == nullptr)
		return;

	w = 0;
	y = 0;
	for (size_t i = 0; string[i] != '\0'; i++)
	{
		RXRect* lMap = lFont->lMapping[string[i]];
		if (lMap == nullptr)
		{
			continue;
		}

		y = std::max(lMap->h, y);
		w += lMap->w;
	}
}

void Render::RenderTexture(RexTextureID aTexID, int x, int y,const RXRect& rect_on_image, int depth, RenderQueue aQueue, float angle, float aScale_x, float aScale_y, float parallax_factor_x, float parallax_factor_y, int center_x,int center_y)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	BlitTexture* it = new BlitTexture(aTexID, rect_on_image, aScale_x, aScale_y, parallax_factor_x, parallax_factor_y);
	it->SetPosition(x, y);
	it->SetCenter(center_x, center_y);
	it->depth = depth;
	it->ignore_camera = aQueue == RenderQueue::RENDER_UI;
	it->angle = angle;

	lImpl->GetQueue(aQueue)->push(it);
}


ParticleEmitter* Render::AddParticleEmitter(particle_preset* particle_preset, float x, float y, float lifespan, int depth)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return 0;
	}

	ParticleEmitter* emit = new ParticleEmitter(particle_preset, lifespan, x, y, depth);
	lImpl->particles.push_back(emit);
	return emit;
}

void Render::RemoveParticleEmitter(ParticleEmitter* _to_delete)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	lImpl->to_delete.insert(_to_delete);
}

bool Render::RenderImpl::UpdateParticles(float dt)
{
	for (std::list<ParticleEmitter*>::iterator it = particles.begin(); it != particles.end(); it++)
	{
		if (!(*it)->Loop(dt))
		{
			to_delete.insert(*it);
		}
	}

	for (std::unordered_set<ParticleEmitter*>::iterator it = to_delete.begin(); it != to_delete.end(); it++)
	{
		delete(*it);
		particles.erase(std::find(particles.begin(), particles.end(), *it));
	}
	to_delete.clear();

	for (std::list<ParticleEmitter*>::iterator it = particles.begin(); it != particles.end(); it++)
	{
		mPartInst->mApp.GetImplementation<Render, Render::RenderImpl>()->RenderParticleEmitter(*it, RenderQueue::RENDER_GAME);
	}

	return true;
}

void Render::RenderImpl::ClearParticles()
{
	Logger::Console_log(LogLevel::LOG_INFO, "Clearing Particles");
	for (std::list<ParticleEmitter*>::iterator it = particles.begin(); it != particles.end(); it++)
	{
		to_delete.insert(*it);
	}
}

void Render::RenderAnimation(Animation& aAnimation, int x, int y, int aDepth, RenderQueue aQueue, float angle, float aScale_x, float aScale_y, float parallax_factor_x, float parallax_factor_y, int center_x, int center_y)
{
	if (aAnimation.GetAmountOfFrames() == 0)
		return;

	RenderTexture(aAnimation.mTexture, x, y, aAnimation.GetCurrentFrame(), aDepth, aQueue, angle, aScale_x, aScale_y, parallax_factor_x, parallax_factor_y, center_x, center_y);
}

void Render::RenderText(const char* text, RexFontID font, int x, int y, int depth, const RXColor& aColor, RenderQueue aQueue, bool ignore_camera)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}
	RexFont* lFont = mApp.GetImplementation<Render, Render::RenderImpl>()->GetFont(font);
	if(lFont == nullptr)
	{
		return;
	}

	BlitItemText* it = new BlitItemText(text, lFont);
	it->ignore_camera = aQueue == RenderQueue::RENDER_UI || ignore_camera;
	it->SetPosition(x, y);
	it->color = { aColor.r, aColor.g, aColor.b, aColor.a };
	it->depth = depth;
	lImpl->GetQueue(aQueue)->push(it);
}

void Render::RenderRect(const RXRect& area, const RXColor& aColor, bool filled, RenderQueue aQueue,int depth , bool ignore_camera)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	BlitRect* it = new BlitRect(area, filled);
	it->color = aColor;
	it->ignore_camera = aQueue == RenderQueue::RENDER_UI || ignore_camera;
	it->depth = depth;

	lImpl->GetQueue(aQueue)->push(it);
}

void Render::RenderTrail(RXPoint* point_array, int amount, RenderQueue aQueue,bool aIgnoreCamera,int depth, uint8_t r, uint8_t g, uint8_t b)
{
	RenderImpl* lImpl = dynamic_cast<RenderImpl*>(mPartFuncts);
	if (!lImpl)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "Wrong format on the implementation class");
		return;
	}

	SDL_Point* lPoints = new SDL_Point[amount];

	for (int i = 0; i < amount; ++i)
	{
		lPoints[i].x = point_array[i].x;
		lPoints[i].y = point_array[i].y;
	}

	BlitTrail* it = new BlitTrail(lPoints,amount,depth);
	it->color = RXColor{ r,g,b ,255};
	it->ignore_camera = aQueue == RenderQueue::RENDER_UI || aIgnoreCamera;
	lImpl->GetQueue(aQueue)->push(it);
}

void BlitTexture::Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow)
{
	float scale = aWindow.GetScale();

	SDL_Rect rect;

	if (!ignore_camera)
	{
		rect.x = (float)x * scale + camera.GetCameraXoffset() * parallax_x;
		rect.y = (float)y * scale + camera.GetCameraYoffset() * parallax_y;
	}
	else
	{
		rect.x = (float)x * scale;
		rect.y = (float)y * scale;
	}

	rect.w = on_image.w * scale * scale_x;
	rect.h = on_image.h * scale * scale_y;

	RXPoint p = { center_x,center_y };
	
	RXRect lRect = { rect.x,rect.y,rect.w,rect.h };
	if (!camera.isOnScreen(lRect, false))
		return;

	//aRender.CountDrawCall();
	//if (SDL_RenderCopyEx(aRender.GetSDL_Renderer(), tex, &on_image, &rect, angle, &p, SDL_FLIP_NONE) != 0)
	if (aRender.RenderTexture(tex, lRect, on_image, angle, p, IMGFLIP::NONE) == false)
	{
		std::string errstr = "Cannot blit to screen. SDL_RenderCopy error: ";
		errstr += SDL_GetError();
		Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
		//ret = false;
	}
}

void BlitLayer::Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow)
{
	float scale = aWindow.GetScale();

	float para_x = mLayer->parallax_x;
	float para_y = mLayer->parallax_y;
	int depth = mLayer->depth;

	tileset* t = mLayer->tileset_of_layer;//(*tilesets.begin());

	for (int _y = 0; _y < mLayer->height; ++_y)
	{
		for (int _x = 0; _x < mLayer->width; ++_x)
		{
			int i = _y * mLayer->width + _x;
			//tileset* t = GetTilesetFromId((*it)->data[i]);
			float worldcoords_x = scale * _x * t->tile_width + camera.GetCameraXoffset();
			float worldcoords_y = scale * _y * t->tile_height + camera.GetCameraYoffset();

			if (mLayer->data[i] == -1)
				continue;

			//App->ren->BlitMapTile(t->texture, _x, _y, GetImageRectFromId((*it)->data[i]), depth, para_x, para_y);

			RXRect on_scn;
			on_scn.x = worldcoords_x;
			on_scn.y = worldcoords_y;
			on_scn.w = 48 * scale;
			on_scn.h = 48 * scale;

			if (!camera.isOnScreen(on_scn, false))
				continue;

			//aRender.CountDrawCall();
			//if (SDL_RenderCopyEx(aRender.GetSDL_Renderer(), tex, &GetImageRectFromId(mLayer->tileset_of_layer, mLayer->data[i]), &on_scn, 0, NULL, SDL_FLIP_NONE) != 0)
			if (aRender.RenderTexture(t->texture, on_scn, GetImageRectFromId(mLayer->tileset_of_layer, mLayer->data[i])) == false)
			{
				std::string errstr = "Cannot blit to screen. SDL_RenderCopy error: ";
				errstr += SDL_GetError();
				Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
			}
		}
	}

}

RXRect BlitLayer::GetImageRectFromId(tileset* t, int id)
{

	RXRect rect;
	rect.w = t->tile_width;
	rect.h = t->tile_height;
	rect.x = ((rect.w) * (id % t->columns));
	rect.y = ((rect.h) * (id / t->columns));

	return rect;
}

void BlitBackground::Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow)
{
	RexTexture* lTexture = aRender.GetTexture(tex);
	if (lTexture == nullptr)
		return;

	float scale = aWindow.GetScale();
	//calculate camera tile

	int cam_tile_x;
	int cam_tile_y;
	int j = 0;
	bool exit = false;

	float cam_posx, cam_posy;
	camera.GetCameraPosition(cam_posx, cam_posy);
	while (!exit)
	{
		if (cam_posx * parallax_x >= (j - 1) * lTexture->mTextureWidth && cam_posx * parallax_x < j * lTexture->mTextureWidth)
		{
			exit = true;
			cam_tile_x = j;
		}
		j++;
	}
	j = 0;
	exit = false;
	while (!exit)
	{
		if (cam_posy * parallax_y >= (j - 1) * lTexture->mTextureHeight && cam_posy * parallax_y < j * lTexture->mTextureHeight)
		{
			exit = true;
			cam_tile_y = j;
		}
		j++;
	}

	int rel_x = cam_posx - cam_tile_x * lTexture->mTextureWidth;
	int rel_y = cam_posy - cam_tile_y * lTexture->mTextureHeight;

	float cam_w, cam_h;
	camera.GetCameraSize(cam_w, cam_h);

	float images_that_fit_in_cam_x = cam_w / lTexture->mTextureWidth;
	float images_that_fit_in_cam_y = cam_h / lTexture->mTextureHeight;

	for (int i = 0; i <= images_that_fit_in_cam_x; ++i)
	{
		for (int j = 0; j <= images_that_fit_in_cam_y; ++j)
		{
			RXRect rect;//DUAL LOOP THIS
			RXRect UV;//DUAL LOOP THIS

			int pos_x = ((cam_tile_x - 1 + i) * lTexture->mTextureWidth) * scale;
			int pos_y = ((cam_tile_y - 1 + j) * lTexture->mTextureHeight) * scale;

			rect.x = pos_x + camera.GetCameraXoffset() * scale * parallax_x;
			rect.y = pos_y + camera.GetCameraYoffset() * scale * parallax_y;

			if (!repeat_y)
			{
				rect.y = 0;// App->scn->room_h * 48 - App->cam->height;
			}

			rect.w = lTexture->mTextureWidth;
			rect.h = lTexture->mTextureHeight;

			rect.w *= scale;
			rect.h *= scale;

			UV = { 0,0,rect.w,rect.h };

			//aRender.CountDrawCall();
			//if (SDL_RenderCopyEx(aRender.GetSDL_Renderer(), tex, NULL, &rect, 0, NULL, SDL_FLIP_NONE) != 0)
			// TO DO OPENGL: RENDER BACKGROUNDS
			if (aRender.RenderTexture(tex, rect,UV) == false)
			{
				std::string errstr = "Cannot blit to screen. SDL_RenderCopy error: ";
				errstr += SDL_GetError();
				Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
			}

			if (!repeat_y)
			{
				break;
			}
		}
	}
}

void BlitRect::Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow)
{
	float scale = aWindow.GetScale();

	SDL_Rect temp;

	if (!ignore_camera)
	{
		temp = { x + (int)camera.GetCameraXoffset(),y + (int)camera.GetCameraYoffset() ,w,h };
	}
	else
	{
		temp = { x,y,w,h };
	}

	temp.x *= scale;
	temp.y *= scale;
	temp.w *= scale;
	temp.h *= scale;

	RXRect lRect = { temp.x,temp.y,temp.w,temp.h };
	if (!camera.isOnScreen(lRect, false))
		return;

	//SDL_SetRenderDrawBlendMode(aRender.GetSDL_Renderer(), SDL_BLENDMODE_BLEND);
	//SDL_SetRenderDrawColor(aRender.GetSDL_Renderer(), color.r, color.g, color.b, color.a);
	//SDL_SetRenderDrawColor(lRender->renderer, color.r, color.g, color.b, 255);

	//aRender.CountDrawCall();
	//int result = (filled) ? SDL_RenderFillRect(aRender.GetSDL_Renderer(), &temp) : SDL_RenderDrawRect(aRender.GetSDL_Renderer(), &temp);
	bool lResult = aRender.RenderSquare(color, lRect, depth, filled);
	if (!lResult)
	{
		std::string errstr = "Cannot draw quad to screen. SDL_RenderFillRect error: ";
		errstr += SDL_GetError();
		Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
	}
}

void BlitTrail::Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow)
{
	float scale = aWindow.GetScale();

	if (!ignore_camera)
	{
		for (int i = 0; i < amount; ++i)
		{
			points[i].x += camera.GetCameraXoffset();
			points[i].y += camera.GetCameraYoffset();
		}
	}

	//SDL_SetRenderDrawBlendMode(aRender.GetSDL_Renderer(), SDL_BLENDMODE_BLEND);
	//SDL_SetRenderDrawColor(aRender.GetSDL_Renderer(), color.r, color.g, color.b, 255);// it's a debug feature so it'll have max visibility
	//int result = SDL_RenderDrawLines(aRender.GetSDL_Renderer(), points, amount);

	delete points;

	//TO DO OPENGL: RENDER THE TRAIL
	//if (result != 0)
	//{
	//	std::string errstr = "Cannot draw trail to screen. SDL_RenderFillRect error: ";
	//	errstr += SDL_GetError();
	//	Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
	//}
}

void BlitParticles::Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow)
{
	float scale = aWindow.GetScale();
	
	for (int i = 0; i < MAX_PARTICLES; ++i)
	{
		if (lEmmitter->particles[i] != nullptr)
		{
			SDL_Rect rect;
			rect.x = lEmmitter->particles[i]->target_on_screen.x * scale + camera.GetCameraXoffset();
			rect.y = lEmmitter->particles[i]->target_on_screen.y * scale + camera.GetCameraYoffset();

			rect.w = lEmmitter->particles[i]->target_on_screen.w * lEmmitter->particles[i]->current_scale;
			rect.h = lEmmitter->particles[i]->target_on_screen.h * lEmmitter->particles[i]->current_scale;

			rect.x -= rect.w / 2;
			rect.y -= rect.h / 2;

			rect.w *= scale;
			rect.h *= scale;

			RXRect* lRecFromEmitter = lEmmitter->particles[i]->area_in_texture;
			SDL_Rect lRectInText = {lRecFromEmitter->x, lRecFromEmitter->y, lRecFromEmitter->w, lRecFromEmitter->h};

			RXRect lRect = { rect.x,rect.y,rect.w,rect.h };
			if (!camera.isOnScreen(lRect, false))
				continue;

			//aRender.CountDrawCall();
			//if (SDL_RenderCopyEx(aRender.GetSDL_Renderer(), tex, &lRectInText, &rect, lEmmitter->particles[i]->angle, NULL, SDL_FLIP_NONE) != 0)
			//TO DO OPENGL: ADD CENTER TO THE CENTER OF QUAD
			if (aRender.RenderTexture( lEmmitter->preset_for_emitter->texture_name, lRect,*lRecFromEmitter, lEmmitter->particles[i]->angle) == false)
			{
				std::string errstr = "Cannot blit to screen. SDL_RenderCopy error: ";
				errstr += SDL_GetError();
				Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
			}
		}
	}
}

void BlitItemText::Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow)
{
	// variable to store token obtained from the original
	int length_so_far = 0;
	int yLevel = 0;

	if (font_used == nullptr)
		return;

	for (int i = 0; i < mText.size(); ++i)
	{
		if(mText[i] == '\n')
		{
			yLevel += font_used->size*1.5;
			length_so_far = 0;
		}
		else if (mText[i] == ' ')
		{
			length_so_far += font_used->size/2;
		}
		else if (font_used->lMapping.count(mText[i]))
		{
			RXRect* mappedRect = font_used->lMapping[mText[i]];
			RXRect on_screen = RXRect{ x + length_so_far,y + yLevel, mappedRect->w, mappedRect->h };

			length_so_far += on_screen.w;

			RXRect lRect = { on_screen.x,on_screen.y,on_screen.w,on_screen.h };
			if (!camera.isOnScreen(lRect,false))
				continue;

			//aRender.CountDrawCall();
			// TO DO OPENGL: TEXT PROCESSING AND RENDERING
			if (aRender.RenderTexture(font_used->font_texture, on_screen, *mappedRect) == false)
			{
				std::string errstr = "Cannot blit to screen. SDL_RenderCopy error: ";
				errstr += SDL_GetError();
				Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
			}
		}
	}
}

#pragma endregion