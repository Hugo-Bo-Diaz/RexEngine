#ifndef SDL_RENDERER_IMPL__H
#define SDL_RENDERER_IMPL__H

#include <vector>
#include <SDL/include/SDL_video.h>
#include "../src/Modules/RenderImpl.h"

struct RexTextureSDL : RexTexture {

	SDL_Texture* mTexture;
};


class SDLRenderer : public Render::RenderImpl
{
public:
	SDLRenderer(Render* aRender) : Render::RenderImpl(aRender) {};

	bool LoadTexture(const char* aPath, RexTextureID& aResultID);
	bool DestroyTexture(RexTextureID aResultID);

	bool RenderTexture(RexTextureID aTexture, const RXRect& aPositionScreen, const RXRect& aPositionTexture, float aRotation = 0, const RXPoint& aCenter = { 0,0 }, IMGFLIP aFlip = NONE);
	bool RenderSquare(const RXColor& aColor, const RXRect& aRectangle, int depth, bool filled = true);

	bool LoadFontXML(const char* path, const RXColor& aColor, int size, RexFontID& aID);
	bool LoadFontTTF(const char*, const RXColor& aColor, int size, RexFontID& aID);

	//TO DO
	//bool DrawSpriteInstanced() { return false; };

protected:
	bool InitRenderer(Window::WindowImpl& aWindowFuncts, Window& aWindow) ;
	bool UpdateRender(float dt);
	bool CleanUp();

	bool LoadConfig(pugi::xml_node& config_node);
	bool CreateConfig(pugi::xml_node& config_node);

private:
	SDL_Renderer* mSDLRenderer;
	SDL_Window* mSDLWindow;

	SDL_Color background;
};

#endif // !SDL_RENDERER_IMPL__H