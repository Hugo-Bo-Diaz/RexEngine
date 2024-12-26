#ifndef OPENGL_RENDERER_IMPL__H
#define OPENGL_RENDERER_IMPL__H

#include <vector>
#include <glad/include/glad/glad.h>
#include <SDL/include/SDL_video.h>
#include "../src/Modules/RenderImpl.h"

struct RexTextureOpenGL : RexTexture {

	unsigned int mBufferIndex;
};

class OpenGL2DRenderer : public Render::RenderImpl
{
public:
	OpenGL2DRenderer(Render* aRender) : Render::RenderImpl(aRender) {};

	bool LoadTexture(const char* aPath, RexTextureID& aResultID);
	bool DestroyTexture(RexTextureID aResultID);

	bool LoadShader(const char* aPathVertex, const char* aPathFragment, RexShaderID& aShader);
	bool CompileShader(const char* aVertexSource, const char* aSourceFragment, RexShaderID& aShader);
	bool SetShader(RexShaderID aShader);
	void SetDefaultShader();

	bool RenderTexture(RexTextureID aTexture, const RXRect& aPositionScreen, const RXRect& aPositionTexture, float aRotation = 0, const RXPoint& aCenter = { 0,0 }, IMGFLIP aFlip = NONE);
	bool RenderSquare(const RXColor& aColor, const RXRect& aRectangle, int depth, bool filled = true);

	void SetViewProjectionMatrix(glm::mat4x4 aMat);

	bool LoadFontXML(const char* path, const RXColor& aColor, int size, RexFontID& aID);
	bool LoadFontTTF(const char*, const RXColor& aColor, int size, RexFontID& aID);

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
	RexShaderID mQuadShader;

	GLuint lBaseTex;

	RexShaderID mCurrentShader;
	glm::mat4x4 mViewProjMat;

protected:
	bool InitRenderer(Window::WindowImpl& aWindowFuncts, Window& aWindow) ;
	bool UpdateRender(float dt);
	bool CleanUp();

	bool LoadConfig(pugi::xml_node& config_node);
	bool CreateConfig(pugi::xml_node& config_node);

private:
	void CreateBuffers();
	void CreateBaseShaders();
	bool GenerateTextureBufferFromSurface(const SDL_Surface* aSurface, unsigned int& aResultID);

	GLubyte checkImage[16][16][4];
};

#endif // !OPENGL_RENDERER_IMPL__H