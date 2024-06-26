#ifndef WINDOW_IMPL__H
#define WINDOW_IMPL__H

#include "PartImpl.h"
#include "../include/Modules/Window.h"

struct SDL_Window;
struct SDL_Surface;

class Window::WindowImpl : public Part::Part_Impl
{
private:
	friend class Window;

	SDL_Window* window;
	SDL_Surface* screen_surface;
	Window* mPartInst;

	int		width;
	int		height;
	float	scale;

	bool fullscreen = false;
	bool borderless = false;
	bool resizable = false;
	bool fullscreen_window = false;

	float base_window_width = 1024;
	float base_window_height = 576;
public:
	Window::WindowImpl(Window* aWindow) :mPartInst(){};

	void SetWindowInst(Window* aAudio)
	{
		mPartInst = aAudio;
	}

	SDL_Window* GetSDLWindow() { return window; };

protected:
	bool Loop(float dt);
	bool CleanUp();

	bool LoadConfig(pugi::xml_node& config_node);
	bool CreateConfig(pugi::xml_node& config_node);

};

#endif // !AUDIO_IMPL__H