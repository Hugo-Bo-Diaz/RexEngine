#include "OpenGLRenderer.h"
#include "../src/Modules/WindowImpl.h"
#include <Utils/Logger.h>
#include "Utils/Utils.h"
#include <string>
#include "../../../include/EngineElements/BaseShaders/Naive_BaseShader.h"
#include "../../../include/EngineElements/BaseShaders/RenderQuad_BaseShader.h"
#include "../../../include/EngineElements/BaseShaders/Instanced_BaseShader.h"

#include <glm/include/glm/gtc/type_ptr.hpp>
#include <SDL/include/SDL_render.h>
#include "SDL_image/include/SDL_image.h"
#include "SDL_ttf\include\SDL_ttf.h"
#pragma comment( lib, "SDL_image/libx86/SDL2_image.lib" )
#pragma comment( lib, "SDL_ttf/libx86/SDL2_ttf.lib" )

bool OpenGL2DRenderer::InitRenderer(Window::WindowImpl& aWindowFuncts, Window& aWindow)
{
	bool ret = true;

	SDL_GL_LoadLibrary(NULL);

	// Request an OpenGL 4.5 context (should be core)
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	// Also request a depth buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);


	mSDLWindow = aWindowFuncts.GetSDLWindow();
	SDLGLcontext = SDL_GL_CreateContext(mSDLWindow);

	int flags = IMG_INIT_PNG | IMG_INIT_JPG;
	int init = IMG_Init(flags);

	if ((init & flags) != flags)
	{
		std::string lStr = "Could not initialize Image lib. IMG_Init: ";
		lStr += IMG_GetError();
		Logger::Console_log(LogLevel::LOG_ERROR, lStr.c_str());
		ret = false;
	}

	SDL_GL_MakeCurrent(mSDLWindow, SDLGLcontext);
	Logger::Console_log(LogLevel::LOG_INFO, "OpenGL loaded");
	gladLoadGLLoader(SDL_GL_GetProcAddress);

	SDL_GL_SetSwapInterval(1);

	std::stringstream ss;
	ss << "Vendor:";
	ss << glGetString(GL_VENDOR);
	Logger::Console_log(LogLevel::LOG_INFO, ss.str().c_str());
	ss.str("");
	ss << "Renderer:";
	ss << glGetString(GL_RENDERER);
	Logger::Console_log(LogLevel::LOG_INFO, ss.str().c_str());
	ss.str("");
	ss << "Version:";
	ss << glGetString(GL_VERSION);
	Logger::Console_log(LogLevel::LOG_INFO, ss.str().c_str());

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	int win_x, win_y;
	aWindow.GetWindowSize(win_x, win_y);

	glViewport(0, 0, win_x, win_y);
	glClearColor(0.0f, 0.5f, 1.0f, 0.0f);

	if (!LoadShader(mVertexLocation.c_str(), mFragLocation.c_str(), mDefaultShader))
	{
		ret = false;
	}
	CreateBuffers();
	CreateBaseShaders();

	if (TTF_Init() == -1)
	{
		printf("SDL_ttf could not initialize! SDL_ttf Error: %s", TTF_GetError());
	}

	return ret;
}

bool OpenGL2DRenderer::LoadConfig(pugi::xml_node& config_node)
{
	//TODO OPENGL
	return true;

}

bool OpenGL2DRenderer::CreateConfig(pugi::xml_node& config_node)
{
	//TODO OPENGL
	return true;

}

void OpenGL2DRenderer::CreateBuffers()
{
	//GENERATE BUFFER FOR GEOMETRY
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	float vertices[] = {
		// pos      // tex
		0.0f, 1.0f, 0.0f, //0.0f, 1.0f,
		1.0f, 0.0f, 0.0f, //1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, //0.0f, 0.0f,

		0.0f, 1.0f, 0.0f, //0.0f, 1.0f,
		1.0f, 1.0f, 0.0f, //1.0f, 1.0f,
		1.0f, 0.0f, 0.0f //1.0f, 0.0f
	};

	float UVs[] = {
		//tex
	   0.0f, 1.0f,
	   1.0f, 0.0f,
	   0.0f, 0.0f,

	   0.0f, 1.0f,
	   1.0f, 1.0f,
	   1.0f, 0.0f
	};

	int fsize = sizeof(float);
	int stride = fsize * 3;

	//ADD GEOMETRY DATA TO THE BUFFER
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 6, &vertices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);


	glGenBuffers(1, &UVVBO);
	glBindBuffer(GL_ARRAY_BUFFER, UVVBO);
	glBufferData(GL_ARRAY_BUFFER, 2 * 6 * sizeof(float), &UVs[0], GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(0));
}

void OpenGL2DRenderer::CreateBaseShaders()
{
	CompileShader(NaiveVertexShader, NaiveFragmentShader, mDefaultShader);
	CompileShader(RenderQuadVertexShader, RenderQuadFragmentShader, mQuadShader);

	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			int c = ((((i & 0x8) == 0) ^ (((j & 0x8)) == 0))) * 255;
			checkImage[i][j][0] = (GLubyte)c;
			checkImage[i][j][1] = (GLubyte)c;
			checkImage[i][j][2] = (GLubyte)c;
			checkImage[i][j][3] = (GLubyte)255;
		}
	}

	glGenTextures(1, &lBaseTex);
	glBindTexture(GL_TEXTURE_2D, lBaseTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16,
		0, GL_RGBA, GL_UNSIGNED_BYTE, checkImage);
}

bool OpenGL2DRenderer::UpdateRender(float dt)
{
	SDL_GL_SwapWindow(mSDLWindow);
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	return true;
}

void OpenGL2DRenderer::SetViewProjectionMatrix(glm::mat4x4 aMat)
{
	mViewProjMat = aMat;
}

bool OpenGL2DRenderer::CleanUp()
{
	for (int i = 0; i < mShaders.size(); i++)
	{
		glDeleteProgram(mShaders[i]);
	}

	for (int i = 0; i < mTextures.size(); i++)
	{
		glDeleteTextures(1, &mTextures[i]->mID);
		delete mTextures[i];
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &UVVBO);

	SDL_GL_DeleteContext(SDLGLcontext);
	return true;
}

bool OpenGL2DRenderer::LoadTexture(const char* aPath, RexTextureID& aResultID)
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
		RexTextureOpenGL* lRexTexture = new RexTextureOpenGL();
		lRexTexture->mID = mTextures.size();
		lRexTexture->mPath = aPath;
		lRexTexture->mTextureWidth = surface->w;
		lRexTexture->mTextureHeight = surface->h;
		GenerateTextureBufferFromSurface(surface, lRexTexture->mBufferIndex);
		SDL_FreeSurface(surface);

		aResultID = lRexTexture->mID;
		mTextures.push_back(lRexTexture);
	}

	return true;
}

bool OpenGL2DRenderer::DestroyTexture(RexTextureID aResultID)
{
	//TODO OPENGL
	return true;

}

bool OpenGL2DRenderer::LoadShader(const char* aPathVertex, const char* aPathFragment, RexShaderID& aShader)
{
	unsigned int fragmentShader;
	unsigned int vertexShader;

	std::ifstream lVertexFile;
	lVertexFile.open(aPathVertex);
	std::string lVertexShader((std::istreambuf_iterator<char>(lVertexFile)),
		(std::istreambuf_iterator<char>()));
	const GLchar* lVertexSource = (const GLchar*)lVertexShader.c_str();

	std::ifstream lFragmentFile;
	lFragmentFile.open(aPathFragment);
	std::string lFragShader((std::istreambuf_iterator<char>(lFragmentFile)),
		(std::istreambuf_iterator<char>()));
	const GLchar* lFragSource = (const GLchar*)lFragShader.c_str();

	if (CompileShader(lVertexSource, lFragSource, aShader))
	{
		mShaders.push_back(aShader);
	}


	return true;
}

bool OpenGL2DRenderer::CompileShader(const char* aVertexSource, const char* aFragmentSource, RexShaderID& aShader)
{
	unsigned int fragmentShader;
	unsigned int vertexShader;

	//COMPILE VERTEX
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &aVertexSource, NULL);
	glCompileShader(vertexShader);
	// check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		return false;
	}

	//COMPILE FRAGMENT
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &aFragmentSource, NULL);
	glCompileShader(fragmentShader);
	// check for shader compile errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		return false;
	}

	// link shaders
	aShader = glCreateProgram();
	glAttachShader(aShader, vertexShader);
	glAttachShader(aShader, fragmentShader);
	glLinkProgram(aShader);

	// check for linking errors
	glGetProgramiv(aShader, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(aShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		return false;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

}

bool OpenGL2DRenderer::SetShader(RexShaderID aShader)
{
	mCurrentShader = aShader;
	glUseProgram(aShader);
	return true;

}

void OpenGL2DRenderer::SetDefaultShader()
{
	SetShader(mDefaultShader);
}

bool OpenGL2DRenderer::LoadFontXML(const char* path, const RXColor& aColor, int size, RexFontID& aID)
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

	RexTextureOpenGL* lRexTexture = new RexTextureOpenGL();
	lRexTexture->mID = mTextures.size();
	lRexTexture->mPath = path;
	lRexTexture->mTextureWidth = surface->w;
	lRexTexture->mTextureHeight = surface->h;
	GenerateTextureBufferFromSurface(surface, lRexTexture->mBufferIndex);
	SDL_FreeSurface(surface);

	lfont->font_texture = lRexTexture->mID;
	lfont->mFontID = mFonts.size();
	aID = lfont->mFontID;
	mFonts.push_back(lfont);

	return true;
}

bool OpenGL2DRenderer::LoadFontTTF(const char* aPath, const RXColor& aColor, int size, RexFontID& aID)
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

	RexTextureOpenGL* lRexTexture = new RexTextureOpenGL();
	lRexTexture->mID = mTextures.size();
	lRexTexture->mPath = aPath;
	lRexTexture->mTextureWidth = totalSurface->w;
	lRexTexture->mTextureHeight = totalSurface->h;
	GenerateTextureBufferFromSurface(totalSurface, lRexTexture->mBufferIndex);
	SDL_FreeSurface(totalSurface);

	lResult->font_texture = lRexTexture->mID;

	mTextures.push_back(lRexTexture);

	lResult->mFontID = mFonts.size();
	aID = lResult->mFontID;
	mFonts.push_back(lResult);

	return true;
}

bool OpenGL2DRenderer::GenerateTextureBufferFromSurface(const SDL_Surface* aSurface, unsigned int& aBufferResultID)
{

	glGenTextures(1, &aBufferResultID);
	glBindTexture(GL_TEXTURE_2D, aBufferResultID);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, aSurface->w, aSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, aSurface->pixels);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, aSurface->w, aSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, aSurface->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	return true;
}

bool OpenGL2DRenderer::RenderTexture(RexTextureID aTexture, const RXRect& aPositionScreen, const RXRect& aPositionTexture, float aRotation, const RXPoint& aCenter, IMGFLIP aFlip)
{
	glm::mat4x4 lTransMat = glm::mat4x4(1.0f);
	lTransMat = glm::translate(lTransMat, glm::vec3(aPositionScreen.x, aPositionScreen.y, 1.0f));
	lTransMat = glm::rotate(lTransMat, aRotation, glm::vec3(0.0f, 0.0f, 1.0f));
	lTransMat = glm::scale(lTransMat, glm::vec3(aPositionScreen.w, aPositionScreen.h, 1.0f));

	RexTextureOpenGL* lTexture = (RexTextureOpenGL*)GetTexture(aTexture);

	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;

	if (lTexture != nullptr)
	{
		glBindTexture(GL_TEXTURE_2D, lTexture->mBufferIndex);
		left = (float)aPositionTexture.x / (float)lTexture->mTextureWidth;
		right = (float)(aPositionTexture.x + aPositionTexture.w) / (float)lTexture->mTextureWidth;
		bottom = (float)aPositionTexture.y / (float)lTexture->mTextureHeight;
		top = (float)(aPositionTexture.y + aPositionTexture.h) / (float)lTexture->mTextureHeight;
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, lBaseTex);
	}

	float UVs[] = {
		//tex
	   left, top,
	   right, bottom,
	   left, bottom,

	   left, top,
	   right, top,
	   right, bottom
	};


	glBindBuffer(GL_ARRAY_BUFFER, UVVBO);
	void* lBufferLocation = glMapBufferRange(GL_ARRAY_BUFFER, 0, 2 * 6 * sizeof(float), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	memcpy(lBufferLocation, UVs, 2 * 6 * sizeof(float));
	glUnmapBuffer(GL_ARRAY_BUFFER);

	int modelTransLocation = glGetUniformLocation(mCurrentShader, "modelMatrix");
	glUniformMatrix4fv(modelTransLocation, 1, GL_FALSE, glm::value_ptr(lTransMat));

	int viewProjectionLocation = glGetUniformLocation(mCurrentShader, "ViewProjection");
	glUniformMatrix4fv(viewProjectionLocation, 1, GL_FALSE, glm::value_ptr(mViewProjMat));

	int vertexColorLocation = glGetUniformLocation(mCurrentShader, "ourColor");
	glUniform4f(vertexColorLocation, 0.0f, 0.0f, 1.0f, 1.0f);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	return true;
}

bool OpenGL2DRenderer::RenderSquare(const RXColor& aColor, const RXRect& aRectangle, int depth, bool filled)
{
	RexShaderID lCurrSh = mCurrentShader;
	SetShader(mQuadShader);

	glm::mat4x4 lTransMat = glm::mat4x4(1.0f);
	lTransMat = glm::translate(lTransMat, glm::vec3(aRectangle.x, aRectangle.y, 1.0f));
	lTransMat = glm::scale(lTransMat, glm::vec3(aRectangle.w, aRectangle.h, 1.0f));


	float left = 0.0f;
	float right = 1.0f;
	float bottom = 0.0f;
	float top = 1.0f;

	float UVs[] = {
		//tex
	   left, top,
	   right, bottom,
	   left, bottom,

	   left, top,
	   right, top,
	   right, bottom
	};

	glBindBuffer(GL_ARRAY_BUFFER, UVVBO);
	void* lBufferLocation = glMapBufferRange(GL_ARRAY_BUFFER, 0, 2 * 6 * sizeof(float), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
	memcpy(lBufferLocation, UVs, 2 * 6 * sizeof(float));
	glUnmapBuffer(GL_ARRAY_BUFFER);

	int modelTransLocation = glGetUniformLocation(mCurrentShader, "modelMatrix");
	glUniformMatrix4fv(modelTransLocation, 1, GL_FALSE, glm::value_ptr(lTransMat));

	int viewProjectionLocation = glGetUniformLocation(mCurrentShader, "ViewProjection");
	glUniformMatrix4fv(viewProjectionLocation, 1, GL_FALSE, glm::value_ptr(mViewProjMat));

	int borderOnlyLocation = glGetUniformLocation(mCurrentShader, "borderOnly");
	glUniform1i(borderOnlyLocation, !filled);

	int aspectLocation = glGetUniformLocation(mCurrentShader, "aspect");
	glUniform1f(aspectLocation, (float)aRectangle.w / (float)aRectangle.h);

	int borderSizeLocation = glGetUniformLocation(mCurrentShader, "borderSize");
	glUniform1f(borderSizeLocation, (200.0f / (float)aRectangle.w) / 100.0f);

	float r = (float)aColor.r / 255.0f;
	float g = (float)aColor.g / 255.0f;
	float b = (float)aColor.b / 255.0f;
	float a = (float)aColor.a / 255.0f;

	int vertexColorLocation = glGetUniformLocation(mCurrentShader, "ourColor");
	glUniform4f(vertexColorLocation, r, g, b, a);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	SetShader(lCurrSh);

	return true;
}
