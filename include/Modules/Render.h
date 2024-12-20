#ifndef RENDER__H
#define RENDER__H

#include"PartsDef.h"
#include "Textures.h"
#include "Text.h"
#include "SceneController.h"

#include "RXRect.h"
#include "RXColor.h"
#include "RXPoint.h"
#include "Part.h"
#include "EngineElements/Animation.h"

enum RenderQueue
{
	RENDER_GAME = 0,
	RENDER_UI = 1,
	RENDER_DEBUG = 2,

	RENDER_MAX = 3
};




class Comparer
{
public:
	bool operator() (BlitItem* itemA, BlitItem* itemB)
	{
		return itemB->depth > itemA->depth;
	}
};

typedef unsigned int RexTextureID;
typedef unsigned int RexShaderID;

struct SDL_Renderer;

//module that handles graphic processing
class DLL_EXPORT Render : public Part
{
public:
	Render(EngineAPI& aAPI);

	//renders a texture
	void RenderTexture(TextureID aTexID, int x, int y,const RXRect& rect_on_image, int aDepth, RenderQueue aQueue = RenderQueue::RENDER_GAME, float angle = 0, float aScale_x = 1.0f, float aScale_y = 1.0f, float parallax_factor_x = 1, float parallax_factor_y = 1, int center_x = -1,int center_y = -1);
	//renders an animation
	void RenderAnimation(Animation& aAnimation, int x, int y, int aDepth = 0, RenderQueue aQueue = RenderQueue::RENDER_GAME, float angle = 0, float aScale_x = 1.0f, float aScale_y = 1.0f, float parallax_factor_x = 1, float parallax_factor_y = 1, int center_x = -1, int center_y = -1);

	//renders a text
	void RenderText(const char* text, FontID font, int x, int y, int depth, const RXColor& aColor,RenderQueue aQueue,bool ignore_camera = false);
	//renders a rectangle
	void RenderRect(const RXRect& area, const RXColor& aColor, bool filled,RenderQueue aQueue, int depth, bool ignore_camera = false);
	//renders a trail of points
	void RenderTrail(RXPoint* point_array, int amount, RenderQueue aQueue,bool aIgnoreCamera,int depth, uint8_t r = 255, uint8_t g = 255, uint8_t b= 255);

	//returns the amount of draw calls that were done last frame
	long GetDrawCallsLastFrame();
	//adds one to the draw calls on this frame
	void CountDrawCall();



	//loads a texutre into memory and returns a handle in the parameter
	bool LoadTexture(const char* aPath, RexTextureID& aResultID) { return false; };
	//frees a texture from memory
	bool DestroyTexture(RexTextureID aResultID) { return false; };

	//loads a shader into memory and returns a handle in the parameter
	bool LoadShader(const char* aPathVertex, const char* aPathFragment, RexShaderID& aShader) { return false; };
	//sets the shader to be used
	bool SetShader(RexShaderID aShader) { return false; };
	//sets the shader to the default render shader
	void SetDefaultShader() {};
	//Destroys the shader referenced
	bool DestroyTexture(RexShaderID aResultID) { return false; };

	//

	bool UpdateRender() { return true; };


	class RenderImpl;
};

class BlitItem
{
public:
	//position
	int x;
	int y;
	int depth;

	RXColor color;

	//rotation point and angle
	//SDL_Point img_center;
	int center_x;
	int center_y;
	float angle;

	bool ignore_camera = false;

	virtual void Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow) = 0;

	virtual ~BlitItem() {};

	void SetPosition(int aX, int aY)
	{
		x = aX;
		y = aY;
	}

	void SetCenter(int aCenterX, int aCenterY)
	{
		center_x = aCenterX;
		center_y = aCenterY;
	}

	bool operator<(const BlitItem& rhs) const
	{
		return rhs.depth > depth;
	}
};

#endif // !RENDER__H
