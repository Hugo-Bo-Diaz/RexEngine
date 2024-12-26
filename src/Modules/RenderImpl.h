#ifndef RENDER_IMPL__H
#define RENDER_IMPL__H

#include "../include/Modules/Render.h"
#include "WindowImpl.h"
#include "PartImpl.h"
#include "SDL/include/SDL.h"
#include "SceneControllerImpl.h"
#include <glad/include/glad/glad.h>
#include "glm/include/glm/common.hpp"
#include "glm/include/glm/glm.hpp"


class ParticleEmitter;

enum IMGFLIP
{
	NONE,
	VERTICAL,
	HORIZONTAL,
	BOTH
};

struct RexFont
{
	RexFontID mFontID;
	std::string name;
	RexTextureID font_texture;

	std::map<char, RXRect*> lMapping;

	int size;
	int char_per_row;
	RXColor color;

	RexFont(const char* aName, RexTextureID aTexture, const RXColor& aColor, int aSize)
		:name(aName), font_texture(aTexture), color(aColor), size(aSize)
	{}
};

struct RexTexture {
	RexTextureID mID;
	std::string mPath;

	int mTextureWidth;
	int mTextureHeight;

	bool operator==(const RexTextureID& t)
	{
		if (mID == t)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool operator==(const char* t)
	{
		if (mPath == t)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

};

class Render::RenderImpl : public Part::Part_Impl
{
public:
	Render::RenderImpl(Render* aRender) :mPartInst(aRender){
	};

	void SetRenderInst(Render* aRender)
	{
		mPartInst = aRender;
	}

	//INTERNAL RENDER FUNCTIONS
	void RenderMapLayer(layer* layer);
	void RenderMapBackground(RexTextureID aTexID, int depth, bool repeat_y, float parallax_factor_x = 1, float parallax_factor_y = 1);
	void RenderParticleEmitter(ParticleEmitter* emitter, RenderQueue aRenderQueue);

	//FUNCTIONS TO INHERIT FOR RENDERERS
	virtual bool RenderTexture(RexTextureID aTexture, const RXRect& aPositionScreen, const RXRect& aPositionTexture, float aRotation = 0, const RXPoint& aCenter = { 0,0 }, IMGFLIP aFlip = NONE) { return false; };
	virtual bool RenderSquare(const RXColor& aColor, const RXRect& aRectangle, int depth, bool filled = true) { return false; };

	virtual bool LoadTexture(const char* aPath, RexTextureID& aResultID) { return false; };
	virtual bool DestroyTexture(RexTextureID aResultID) { return false; };
	RexTexture* GetTexture(RexTextureID aTexture);

	virtual bool LoadShader(const char* aPathVertex, const char* aPathFragment, RexShaderID& aShader) { return false; };
	virtual bool SetShader(RexShaderID aShader) { return false; };
	virtual void SetDefaultShader() {};

	virtual void SetViewProjectionMatrix(glm::mat4x4 aMat) {};

	virtual bool LoadFontXML(const char* path, const RXColor& aColor, int size, RexFontID& aID) { return 0; };
	virtual bool LoadFontTTF(const char*, const RXColor& aColor, int size, RexFontID& aID) { return 0; };
	RexFont* GetFont(RexFontID aFontID);

	void ClearParticles();

protected:
	bool Init();
	bool Loop(float dt);

	virtual bool InitRenderer(Window::WindowImpl& aWindowFuncts, Window& aWindow) = 0;
	virtual bool CleanUp() = 0;
	virtual bool UpdateRender(float dt) { return true; };

	virtual bool LoadConfig(pugi::xml_node& config_node) = 0;
	virtual bool CreateConfig(pugi::xml_node& config_node) = 0;

	std::vector<RexShaderID> mShaders;
	std::vector<RexTexture*> mTextures;
	std::vector<RexFont*> mFonts;

	RXColor		backgroundColor;
	const std::string mSupportedChars = ":abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890/.,'";

private:
	std::priority_queue<BlitItem*, std::vector<BlitItem*>, Comparer>* GetQueue(RenderQueue aQueue);

	float		mDrawCallsLastFrame;

	std::priority_queue<BlitItem*, std::vector<BlitItem*>, Comparer> allQueue;
	std::priority_queue<BlitItem*, std::vector<BlitItem*>, Comparer> uiQueue;
	std::priority_queue<BlitItem*, std::vector<BlitItem*>, Comparer> debugQueue;

	std::unordered_set<ParticleEmitter*> to_delete;
	std::list<ParticleEmitter*> particles;
	bool UpdateParticles(float dt);

	//float* vertices; 
	friend class Render;
	Render* mPartInst;
};


class BlitItemText : public BlitItem
{
public:
	BlitItemText(const char* aText, RexFont* aFontUsed)
		: font_used(aFontUsed) {
		mText = aText;
	};
	~BlitItemText() {
		mText.clear();
	}

	std::string mText;
	RexFont* font_used;
	void Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow);
};

class BlitTexture : public BlitItem
{
public:
	BlitTexture(RexTextureID aTex,const RXRect& aOnImage,float aScale_x, float aScale_y, float aParallax_x, float aParallax_y)
		: tex(aTex), on_image(aOnImage), scale_x(aScale_x), scale_y(aScale_y), parallax_x(aParallax_x), parallax_y(aParallax_y) {};

	~BlitTexture() {
	}

	RexTextureID tex;
	RXRect on_image;
	float parallax_x;
	float parallax_y;
	float scale_x;
	float scale_y;

	void Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow);
};

class BlitLayer : public BlitItem
{
public:
	BlitLayer(layer* aLayer) : mLayer(aLayer) {};

	~BlitLayer() {
	}

	layer* mLayer;
	void Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow);

	RXRect GetImageRectFromId(tileset* t, int id);

};


class BlitBackground : public BlitTexture
{
public:
	BlitBackground(RexTextureID aTexID, int aDepth, bool aRepeat_y, float aParallax_factor_x, float aParallax_factor_y)
		: BlitTexture(aTexID, RXRect{ 0,0,0,0 }, 1.0f, 1.0f, aParallax_factor_x, aParallax_factor_y), repeat_y(aRepeat_y) {
		depth = aDepth;
	};
	bool repeat_y;
	void Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow);
};

class BlitParticles : public BlitItem
{
public:
	BlitParticles(ParticleEmitter* aEmmitter) : lEmmitter(aEmmitter) {};

	ParticleEmitter* lEmmitter;

	void Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow);
};

class BlitRect : public BlitItem
{
public:
	BlitRect(const RXRect& aRect, bool aFilled) : w(aRect.w), h(aRect.h), filled(aFilled) {
		x = aRect.x;
		y = aRect.y;
	}

	int w;
	int h;

	bool filled;
	void Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow);
};

class BlitTrail : public BlitItem
{
public:
	BlitTrail(SDL_Point* aPoint, int aAmount, int aDepth) : points(aPoint), amount(aAmount) { depth = aDepth; }

	SDL_Point* points;
	int amount;
	void Blit(Render::RenderImpl& aRender, Camera& camera, Window& aWindow);
};

#endif