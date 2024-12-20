#ifndef AUDIO_IMPL__H
#define AUDIO_IMPL__H

#include <vector>
#include <glad/include/glad/glad.h>
#include <SDL/include/SDL_video.h>
#include "../src/Modules/RenderImpl.h"


class OpenGL2DRenderer : public Render::RenderImpl
{
public:
	OpenGL2DRenderer(Render* aRender) :Render::RenderImpl(aRender) {};

	bool LoadTexture(const char* aPath, RexTextureID& aResultID);
	bool DestroyTexture(RexTextureID aResultID);

	bool LoadShader(const char* aPathVertex, const char* aPathFragment, RexShaderID& aShader);
	bool SetShader(RexShaderID aShader);
	void SetDefaultShader();

	bool DrawSprite(RexTextureID aTexture, const RXRect& aPositionScreen, const RXRect& aPositionTexture, float aRotation = 0, const RXPoint& aCenter = { 0,0 }, IMGFLIP aFlip = NONE);
	
	void SetViewProjectionMatrix(glm::mat4x4 aMat);

	//TO DO
	//bool DrawSpriteInstanced() { return false; };

	//BASE GEOMETRY BUFFERS
	unsigned int VBO, VAO;
	unsigned int UVVBO;

	unsigned int VBOInstanced, VAOInstanced;

	SDL_GLContext SDLGLcontext;
	SDL_Window* mSDLWindow;

	std::string mVertexLocation = "Shaders/naive.vert";
	std::string mFragLocation = "Shaders/naive.frag";
	RexShaderID mDefaultShader;

	RexTextureID lBaseTex;

	RexShaderID mCurrentShader;
	glm::mat4x4 mViewProjMat;

protected:
	bool InitRenderer(Window::WindowImpl& aWindowFuncts, Window& aWindow) ;
	bool Loop(float dt);
	bool CleanUp();

	bool LoadConfig(pugi::xml_node& config_node);
	bool CreateConfig(pugi::xml_node& config_node);

private:
	void CreateBuffers();

};

#endif // !AUDIO_IMPL__H