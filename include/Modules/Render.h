#ifndef RENDER__H
#define RENDER__H

#include"PartsDef.h"
#include "SceneController.h"

#include "RXRect.h"
#include "RXColor.h"
#include "RXPoint.h"
#include "Part.h"
#include "EngineElements/Animation.h"
#include "EngineElements/ParticleEmitter.h"

#define XMLFONTEXTENSION "xml"
#define TTFFONTEXTENSION "ttf"

enum RenderQueue
{
	RENDER_GAME = 0,
	RENDER_UI = 1,
	RENDER_DEBUG = 2,

	RENDER_MAX = 3
};

typedef unsigned int RexTextureID;
typedef unsigned int RexFontID;
typedef unsigned int RexShaderID;


//module that handles graphic processing
class DLL_EXPORT Render : public Part
{
public:
	Render(EngineAPI& aAPI, const char* aRenderType);

	//renders a texture
	void RenderTexture(RexTextureID aTexID, int x, int y,const RXRect& rect_on_image, int aDepth, RenderQueue aQueue = RenderQueue::RENDER_GAME, float angle = 0, float aScale_x = 1.0f, float aScale_y = 1.0f, float parallax_factor_x = 1, float parallax_factor_y = 1, int center_x = -1,int center_y = -1);
	//renders an animation
	void RenderAnimation(Animation& aAnimation, int x, int y, int aDepth = 0, RenderQueue aQueue = RenderQueue::RENDER_GAME, float angle = 0, float aScale_x = 1.0f, float aScale_y = 1.0f, float parallax_factor_x = 1, float parallax_factor_y = 1, int center_x = -1, int center_y = -1);

	//renders a text
	void RenderText(const char* text, RexFontID font, int x, int y, int depth, const RXColor& aColor,RenderQueue aQueue,bool ignore_camera = false);
	//renders a rectangle
	void RenderRect(const RXRect& area, const RXColor& aColor, bool filled,RenderQueue aQueue, int depth, bool ignore_camera = false);
	//renders a trail of points
	void RenderTrail(RXPoint* point_array, int amount, RenderQueue aQueue,bool aIgnoreCamera,int depth, uint8_t r = 255, uint8_t g = 255, uint8_t b= 255);

	//returns the amount of draw calls that were done last frame
	long GetDrawCallsLastFrame();
	//adds one to the draw calls on this frame
	void CountDrawCall();



	//loads a texutre into memory and returns a handle in the parameter
	bool LoadTexture(const char* aPath, RexTextureID& aResultID);
	//frees a texture from memory
	bool DestroyTexture(RexTextureID aResultID);

	//loads a shader into memory and returns a handle in the parameter
	bool LoadShader(const char* aPathVertex, const char* aPathFragment, RexShaderID& aShader);
	//sets the shader to be used
	bool SetShader(RexShaderID aShader);
	//sets the shader to the default render shader
	void SetDefaultShader();
	//Destroys the shader referenced
	bool DestroyShader(RexShaderID aResultID);

	//
	bool LoadFont(const char* aPath, const RXColor& aColor, int size, RexFontID& aFontID);
	void GetTextSize(RexFontID aFontID, const char* string, int& w, int& y);

	//creates a particle emitter and adds it to the engine
	ParticleEmitter* AddParticleEmitter(particle_preset* particle_preset, float x, float y, float lifespan = -1, int depth = 0);
	//removes a particle emitter and deletes its memory
	void RemoveParticleEmitter(ParticleEmitter* _to_delete);


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

class Comparer
{
public:
	bool operator() (BlitItem* itemA, BlitItem* itemB)
	{
		return itemB->depth > itemA->depth;
	}
};


#endif // !RENDER__H
