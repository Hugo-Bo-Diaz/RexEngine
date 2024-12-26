#include "SDLRenderer.h"
#include "../src/Modules/WindowImpl.h"
#include <Utils/Logger.h>
#include "Utils/Utils.h"
#include <string>

#include <glm/include/glm/gtc/type_ptr.hpp>
#include <SDL/include/SDL_render.h>
#include "SDL_image/include/SDL_image.h"
#include "SDL_ttf\include\SDL_ttf.h"
#pragma comment( lib, "SDL_image/libx86/SDL2_image.lib" )
#pragma comment( lib, "SDL_ttf/libx86/SDL2_ttf.lib" )

bool SDLRenderer::InitRenderer(Window::WindowImpl& aWindowFuncts, Window& aWindow)
{
	bool ret = true;

	Uint32 flags = SDL_RENDERER_ACCELERATED;

	mSDLWindow = aWindowFuncts.GetSDLWindow();

	Logger::Console_log(LogLevel::LOG_INFO, "Create SDL rendering context");
	mSDLRenderer = SDL_CreateRenderer(mSDLWindow, -1, flags);
	SDL_SetRenderDrawBlendMode(mSDLRenderer, SDL_BLENDMODE_BLEND);

	int IMGflags = IMG_INIT_PNG | IMG_INIT_JPG;
	int init = IMG_Init(IMGflags);

	if ((init & flags) != flags)
	{
		std::string lStr = "Could not initialize Image lib. IMG_Init: ";
		lStr += IMG_GetError();
		Logger::Console_log(LogLevel::LOG_ERROR, lStr.c_str());
		ret = false;
	}

	int win_x, win_y;
	aWindow.GetWindowSize(win_x, win_y);

	if (TTF_Init() == -1)
	{
		printf("SDL_ttf could not initialize! SDL_ttf Error: %s", TTF_GetError());
	}

	return ret;
}

bool SDLRenderer::LoadConfig(pugi::xml_node& config_node)
{
	//TODO OPENGL
	return true;

}

bool SDLRenderer::CreateConfig(pugi::xml_node& config_node)
{
	//TODO OPENGL
	return true;

}

bool SDLRenderer::UpdateRender(float dt)
{
	SDL_SetRenderDrawColor(mSDLRenderer, background.r, background.g, background.g, background.a);
	SDL_RenderPresent(mSDLRenderer);
	SDL_RenderClear(mSDLRenderer);

	return true;
}

bool SDLRenderer::CleanUp()
{
//TO DO CLEAN UP FONTS AND TEXTURES
	return true;
}

bool SDLRenderer::LoadTexture(const char* aPath, RexTextureID& aResultID)
{
	for (int i = 0; i < mTextures.size(); i++)
	{
		if (std::strcmp(mTextures[i]->mPath.c_str(), aPath) == 0)
		{
			aResultID = mTextures[i]->mID;
			return true;
		}
	}

	std::stringstream lStr;
	lStr << "Loading texture from: " << aPath;
	Logger::Console_log(LogLevel::LOG_INFO, lStr.str().c_str());

	SDL_Surface* surface = IMG_Load(aPath);

	if (surface == NULL)
	{
		std::string lStr = "Could not load surface with path: ";
		lStr += aPath;
		lStr += " IMG_Init: ";
		lStr += IMG_GetError();
		Logger::Console_log(LogLevel::LOG_ERROR, lStr.c_str());
	}
	else
	{
		RexTextureSDL* lRexTexture = new RexTextureSDL();
		lRexTexture->mID = mTextures.size();
		lRexTexture->mPath = aPath;
		lRexTexture->mTextureWidth = surface->w;
		lRexTexture->mTextureHeight = surface->h;
		lRexTexture->mTexture = SDL_CreateTextureFromSurface(mSDLRenderer, surface);
		if (lRexTexture->mTexture == NULL)
		{
			Logger::Console_log(LogLevel::LOG_ERROR, "couldn't make texture from surface");
		}

		SDL_FreeSurface(surface);

		aResultID = lRexTexture->mID;
		mTextures.push_back(lRexTexture);
	}

	return true;
}

bool SDLRenderer::DestroyTexture(RexTextureID aResultID)
{
	//TODO OPENGL
	return true;

}

bool SDLRenderer::LoadFontXML(const char* path, const RXColor& aColor, int size, RexFontID& aID)
{
	pugi::xml_document	font_file;
	pugi::xml_node font_node;
	pugi::xml_parse_result result = font_file.load_file(path);

	if (result.status != pugi::xml_parse_status::status_ok)
	{
		return -1;
	}

	font_node = font_file.child("font");

	const char* lImagePath = font_node.child_value("image_path");

	SDL_Surface* surface = NULL;
	if (FileExists(lImagePath))
	{
		surface = IMG_Load(lImagePath);
	}
	else
	{
		std::string lDir = GetDirectoryFromPath(path);
		std::string lCompletePath = lDir + lImagePath;
		surface = IMG_Load(lCompletePath.c_str());
	}

	SDL_Texture* texture = NULL;

	if (surface == NULL)
	{
		std::string lStr = "Could not load surface with path:";
		lStr += path;
		lStr += " IMG_Load: ";
		lStr += IMG_GetError();
		Logger::Console_log(LogLevel::LOG_ERROR, lStr.c_str());
		return -1;
	}

	//texture = SDL_CreateTextureFromSurface(mPartInst->mApp.GetModule<::Render>().GetSDL_Renderer(), surface);
	if (texture == NULL)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "couldn't make texture from surface");
	}
	SDL_FreeSurface(surface);

	pugi::xml_node properties_node = font_node.child("rectangles");
	pugi::xml_node iterator;
	//TextureID lTex = mPartInst->mApp.GetImplementation<Textures, Textures::TexturesImpl>()->AddTexture(texture, path);

	RexFont* lfont = new RexFont(path, 0, aColor, size);

	for (iterator = properties_node.first_child(); iterator; iterator = iterator.next_sibling())
	{
		char c = iterator.child_value("char")[0];
		RXRect* lRect = new RXRect();

		pugi::xml_node lRectangle = iterator.child("rectangle");

		lRect->x = std::stoi(lRectangle.child_value("x"));
		lRect->y = std::stoi(lRectangle.child_value("y"));
		lRect->w = std::stoi(lRectangle.child_value("w"));
		lRect->h = std::stoi(lRectangle.child_value("h"));

		lfont->lMapping.insert(std::make_pair(c, lRect));
	}

	RexTextureSDL* lRexTexture = new RexTextureSDL();
	lRexTexture->mID = mTextures.size();
	lRexTexture->mPath = path;
	lRexTexture->mTextureWidth = surface->w;
	lRexTexture->mTextureHeight = surface->h;
	lRexTexture->mTexture = SDL_CreateTextureFromSurface(mSDLRenderer, surface);
	if (lRexTexture->mTexture == NULL)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "couldn't make texture from surface");
	}
	SDL_FreeSurface(surface);

	lfont->font_texture = lRexTexture->mID;
	lfont->mFontID = mFonts.size();
	aID = lfont->mFontID;
	mFonts.push_back(lfont);

	return true;
}

bool SDLRenderer::LoadFontTTF(const char* aPath, const RXColor& aColor, int size, RexFontID& aID)
{
	TTF_Font* lFont = TTF_OpenFont(aPath, size);

	if (lFont == nullptr)
	{
		std::stringstream lStream;
		lStream << "Could not load the font: " << aPath << ", error: " << TTF_GetError();
		Logger::Console_log(LogLevel::LOG_ERROR, lStream.str().c_str());
		return false;
	}

	SDL_Surface* totalSurface = SDL_CreateRGBSurface(0, 512, 512, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
	//SDL_Surface* totalSurface = SDL_CreateRGBSurface(0, 512, 512, 32, 0, 0, 0, 0);
	RexFont* lResult = new RexFont(aPath, NULL, aColor, size);

	int cursor_x = 0, cursor_y = 0;
	for (int i = 0; i < mSupportedChars.size(); i++)
	{
		int size_char_x, size_char_y;
		std::string lChar;
		lChar += mSupportedChars[i];
		TTF_SizeText(lFont, lChar.c_str(), &size_char_x, &size_char_y);

		RXRect* lCharMapping = new RXRect();
		lCharMapping->x = cursor_x;
		lCharMapping->y = cursor_y;
		lCharMapping->w = size_char_x;
		lCharMapping->h = size_char_y;

		SDL_Surface* lCharSurf = TTF_RenderText_Solid(lFont, lChar.c_str(), { aColor.r,aColor.g,aColor.b,aColor.a });
		cursor_x += size_char_x;

		if (cursor_x + size > 512)
		{
			cursor_y += 2 * size;
			cursor_x = 0;
		}

		if (lCharSurf == NULL)
		{
			std::stringstream lStream;
			lStream << "Could not create the font character for font: " << aPath << " character: " << mSupportedChars[i];
			Logger::Console_log(LogLevel::LOG_ERROR, lStream.str().c_str());
			return false;
		}
		lResult->lMapping.insert(std::make_pair(mSupportedChars[i], lCharMapping));

		SDL_Rect forBlit = { lCharMapping->x,lCharMapping->y,lCharMapping->w,lCharMapping->h };
		SDL_UpperBlit(lCharSurf, new SDL_Rect{ 0,0,size_char_x,size_char_y }, totalSurface, &forBlit);
		SDL_FreeSurface(lCharSurf);
	}

	TTF_CloseFont(lFont);

	RexTextureSDL* lRexTexture = new RexTextureSDL();
	lRexTexture->mID = mTextures.size();
	lRexTexture->mPath = aPath;
	lRexTexture->mTextureWidth = totalSurface->w;
	lRexTexture->mTextureHeight = totalSurface->h;
	lRexTexture->mTexture = SDL_CreateTextureFromSurface(mSDLRenderer, totalSurface);
	if (lRexTexture->mTexture == NULL)
	{
		Logger::Console_log(LogLevel::LOG_ERROR, "couldn't make texture from surface");
	}
	SDL_FreeSurface(totalSurface);

	lResult->font_texture = lRexTexture->mID;

	mTextures.push_back(lRexTexture);

	lResult->mFontID = mFonts.size();
	aID = lResult->mFontID;
	mFonts.push_back(lResult);

	return true;
}

bool SDLRenderer::RenderTexture(RexTextureID aTexture, const RXRect& aPositionScreen, const RXRect& aPositionTexture, float aRotation, const RXPoint& aCenter, IMGFLIP aFlip)
{
	RexTextureSDL* RXTEX = (RexTextureSDL*)GetTexture(aTexture);

	SDL_Rect onTex = { aPositionTexture.x,aPositionTexture.y, aPositionTexture.w, aPositionTexture.h };
	SDL_Rect onScreen = { aPositionScreen.x,aPositionScreen.y, aPositionScreen.w, aPositionScreen.h };

	SDL_Point center = { aCenter.x,aCenter.y };

	SDL_RendererFlip lFlip = SDL_FLIP_NONE;
	switch (aFlip)
	{
	case VERTICAL:
		lFlip = SDL_FLIP_VERTICAL;
		break;
	case HORIZONTAL:
		lFlip = SDL_FLIP_HORIZONTAL;
		break;
	case BOTH:
		break;
	default:
		break;
	}

	if (SDL_RenderCopyEx(mSDLRenderer, RXTEX->mTexture, &onTex, &onScreen, aRotation, &center, lFlip) != 0)
	{
		std::string errstr = "Cannot blit to screen. SDL_RenderCopy error: ";
		errstr += SDL_GetError();
		Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
		return false;
	}
	return true;
}

bool SDLRenderer::RenderSquare(const RXColor& aColor, const RXRect& aRectangle, int depth, bool filled)
{
	SDL_Rect onScreen = { aRectangle.x,aRectangle.y, aRectangle.w, aRectangle.h };
	SDL_Color lColor = { aColor.r,aColor.g,aColor.b,aColor.a };

	SDL_SetRenderDrawBlendMode(mSDLRenderer, SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor(mSDLRenderer, aColor.r, aColor.g, aColor.b, aColor.a);

	int result = 0;

	if (filled)
	{
		result = SDL_RenderFillRect(mSDLRenderer, &onScreen);
	}
	else
	{
		result = SDL_RenderDrawRect(mSDLRenderer, &onScreen);
	}

	if (result != 0)
	{
		std::string errstr = "Cannot draw quad to screen. SDL_RenderFillRect error: ";
		errstr += SDL_GetError();
		Logger::Console_log(LogLevel::LOG_ERROR, errstr.c_str());
	}

	return true;
}
