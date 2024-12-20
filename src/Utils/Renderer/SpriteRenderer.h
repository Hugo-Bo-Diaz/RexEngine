//#ifndef SPRITE_RENDERER_BASE__H
//#define SPRITE_RENDERER_BASE__H
//
//#include "pugiXML/src/pugixml.hpp"
//#include "RXRect.h"
//#include "RXPoint.h"
//#include <Modules/Window.h>
//#include <../src/Modules/WindowImpl.h>
//#include <../src/Modules/RenderImpl.h>
//#include <glm/include/glm/glm.hpp>
//
//
//typedef unsigned int RexTextureID;
//struct RexTexture {
//	RexTextureID mID;
//	std::string mPath;
//	unsigned int mBufferIndex;
//	bool operator==(const RexTextureID& t)
//	{
//		if (mID == t)
//		{
//			return true;
//		}
//		else
//		{
//			return false;
//		}
//	}
//
//	bool operator==(const char* t)
//	{
//		if (mPath == t)
//		{
//			return true;
//		}
//		else
//		{
//			return false;
//		}
//	}
//
//};
//typedef unsigned int RexShader;
//
//enum IMGFLIP
//{
//	NONE,
//	VERTICAL,
//	HORIZONTAL,
//	BOTH
//};
//
//class SpriteRenderer 
//{
//public:
//	friend class Render;
//	friend class RenderImpl;
//
//	virtual bool LoadTexture(const char* aPath, RexTextureID& aResultID) { return false; };
//	virtual bool DestroyTexture(RexTextureID aResultID) { return false; };
//
//	virtual bool LoadShader(const char* aPathVertex, const char* aPathFragment, RexShader& aShader) { return false; };
//	virtual bool SetShader(RexShader aShader) { return false; };
//	virtual void SetDefaultShader() {};
//
//	virtual bool DrawSprite(RexTextureID aTexture, const RXRect& aPositionScreen, const RXRect& aPositionTexture, float aRotation = 0, const RXPoint& aCenter = { 0,0 }, IMGFLIP aFlip = NONE) { return false; };
//
//	virtual void SetViewProjectionMatrix(glm::mat4x4 aMat) {};
//	virtual bool UpdateRender() { return true; };
//	//TO DO
//	//bool DrawSpriteInstanced() { return false; };
//
//protected:
//	virtual bool Init(Window::WindowImpl& aWindowFuncts, Window& aWindow) { return true; };
//	virtual bool CleanUp() { return true; };
//
//	virtual bool LoadConfig(pugi::xml_node& config_node) { return true; };
//	virtual bool CreateConfig(pugi::xml_node& config_node) { return true; };
//
//	std::vector<RexShader> mShaders;
//	std::vector<RexTexture*> mTextures;
//};
//
//#endif // !AUDIO_IMPL__H