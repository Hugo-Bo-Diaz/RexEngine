#include "OpenGLRenderer.h"
#include "../src/Modules/WindowImpl.h"
#include <SDL/include/SDL_render.h>
#include <Utils/Logger.h>
#include <string>
#include <glm/include/glm/gtc/type_ptr.hpp>
#include <SDL_image/include/SDL_image.h>

#include "SDL_image/include/SDL_image.h"
#pragma comment( lib, "SDL_image/libx86/SDL2_image.lib" )

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
	RexTexture* lRexTexture = new RexTexture();

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
		glGenTextures(1, &lRexTexture->mID);
		glBindTexture(GL_TEXTURE_2D, lRexTexture->mID); 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		SDL_FreeSurface(surface);

		aResultID = lRexTexture->mID;
		mTextures.push_back(lRexTexture);
	}

	return true;
}

bool OpenGL2DRenderer::DestroyTexture(RexTextureID aResultID)
{
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

	//SHADERS
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &lVertexSource, NULL);
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

	std::ifstream lFragmentFile;
	lFragmentFile.open(aPathFragment);
	std::string lFragShader((std::istreambuf_iterator<char>(lFragmentFile)),
		(std::istreambuf_iterator<char>()));
	const GLchar* lFragSource = (const GLchar*)lFragShader.c_str();

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &lFragSource, NULL);
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

	mShaders.push_back(aShader);

	return true;
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

bool OpenGL2DRenderer::DrawSprite(RexTextureID aTexture, const RXRect& aPositionScreen, const RXRect& aPositionTexture, float aRotation, const RXPoint& aCenter, IMGFLIP aFlip)
{
	glm::mat4x4 lTransMat = glm::mat4x4(1.0f);
	lTransMat = glm::translate(lTransMat, glm::vec3(aPositionScreen.x, aPositionScreen.y, 1.0f));
	lTransMat = glm::rotate(lTransMat,aRotation, glm::vec3(0.0f, 0.0f, 1.0f));
	lTransMat = glm::scale(lTransMat,glm::vec3(aPositionScreen.w,aPositionScreen.h,1.0f));

	glBindTexture(GL_TEXTURE_2D, lBaseTex);

	int modelTransLocation = glGetUniformLocation(mCurrentShader, "modelMatrix");
	glUniformMatrix4fv(modelTransLocation, 1, GL_FALSE, glm::value_ptr(lTransMat));

	int viewProjectionLocation = glGetUniformLocation(mCurrentShader, "ViewProjection");
	glUniformMatrix4fv(viewProjectionLocation, 1, GL_FALSE, glm::value_ptr(mViewProjMat));

	int vertexColorLocation = glGetUniformLocation(mCurrentShader, "ourColor");
	glUniform4f(vertexColorLocation, 0.0f, 0.0f, 1.0f, 1.0f);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	return true;
}

void OpenGL2DRenderer::SetViewProjectionMatrix(glm::mat4x4 aMat)
{
	mViewProjMat = aMat;
}

bool OpenGL2DRenderer::Loop(float dt)
{
	SDL_GL_SwapWindow(mSDLWindow);
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	return true;
}

bool OpenGL2DRenderer::InitRenderer(Window::WindowImpl& aWindowFuncts,Window& aWindow)
{
	bool ret = true;
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

	int win_x, win_y;
	aWindow.GetWindowSize(win_x, win_y);

	glViewport(0, 0, win_x, win_y);
	glClearColor(0.0f, 0.5f, 1.0f, 0.0f);

	if (!LoadShader(mVertexLocation.c_str(), mFragLocation.c_str(),mDefaultShader))
	{
		ret = false;
	}
	CreateBuffers();

	LoadTexture("Assets/Sprites/Particles.png", lBaseTex);

	return ret;
}

bool OpenGL2DRenderer::CleanUp()
{
	for (int i = 0; i < mShaders.size(); i++)
	{
		glDeleteProgram(mShaders[i]);
	}

	for (int i = 0; i < mTextures.size(); i++)
	{
		glDeleteTextures(1,&mTextures[i]->mID);
		delete mTextures[i];
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &UVVBO);

	SDL_GL_DeleteContext(SDLGLcontext);
	return true;
}

bool OpenGL2DRenderer::LoadConfig(pugi::xml_node& config_node)
{
	return true;

}

bool OpenGL2DRenderer::CreateConfig(pugi::xml_node& config_node)
{
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
	   0.0f, 0.0f,
	   1.0f, 1.0f,
	   0.0f, 1.0f,

	   0.0f, 0.0f,
	   1.0f, 0.0f,
	   1.0f, 1.0f
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

//void OpenGL2DRenderer::CreateBuffers()
//{
//	//GENERATE BUFFER FOR GEOMETRY
//	glGenVertexArrays(1, &VAO);
//	glGenBuffers(1, &VBO);
//	glBindVertexArray(VAO);
//
//	float vertices[] = {
//		// pos      // tex
//		0.0f, 1.0f, 0.0f, //0.0f, 1.0f,
//		1.0f, 0.0f, 0.0f, //1.0f, 0.0f,
//		0.0f, 0.0f, 0.0f, //0.0f, 0.0f,
//
//		0.0f, 1.0f, 0.0f, //0.0f, 1.0f,
//		1.0f, 1.0f, 0.0f, //1.0f, 1.0f,
//		1.0f, 0.0f, 0.0f //1.0f, 0.0f
//	};
//
//	float UVs[] = {
//		//tex
//	   0.0f, 1.0f,
//	   1.0f, 0.0f,
//	   0.0f, 0.0f,
//
//	   0.0f, 1.0f,
//	   1.0f, 1.0f,
//	   1.0f, 0.0f
//	};
//
//	modelV = new modelvert[6];
//	model = glm::mat4x4(1.0f);
//	model = glm::scale(model, glm::vec3(2, 2, 1));
//	model = glm::translate(model, glm::vec3(-2, 0, 0.0));
//	modelV[0] = modelvert{ {0.0f,1.0f,1.0f},{0.0f,1.0f}, model };
//	modelV[1] = modelvert{ {1.0f,0.0f,1.0f},{1.0f,0.0f}, model };
//	modelV[2] = modelvert{ {0.0f,0.0f,1.0f},{0.0f,0.0f}, model };
//	modelV[3] = modelvert{ {0.0f,1.0f,1.0f},{0.0f,1.0f}, model };
//	modelV[4] = modelvert{ {1.0f,1.0f,1.0f},{1.0f,1.0f}, model };
//	modelV[5] = modelvert{ {1.0f,0.0f,1.0f},{1.0f,0.0f}, model };
//
//
//	std::size_t vec4Size = sizeof(glm::vec4);
//	glBindBuffer(GL_ARRAY_BUFFER, VBO);
//	//ADD GEOMETRY DATA TO THE BUFFER
//	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 5 * 6 * (vec4Size * 4), &modelV[0], GL_STATIC_DRAW);
//
//	int fsize = sizeof(float);
//	int stride = fsize * 5 + vec4Size * 4;
//
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
//	glEnableVertexAttribArray(0);
//
//	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(fsize * 3));
//	glEnableVertexAttribArray(1);
//
//
//	// vertex attributes
//	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(unsigned int)(fsize * 5));
//	glEnableVertexAttribArray(2);
//	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(unsigned int)(fsize * 5 + (1 * vec4Size)));
//	glEnableVertexAttribArray(3);
//	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)(unsigned int)(fsize * 5 + (2 * vec4Size)));
//	glEnableVertexAttribArray(4);
//	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)(unsigned int)(fsize * 5 + (3 * vec4Size)));
//	glEnableVertexAttribArray(5);
//
//	//glVertexAttribDivisor(2, 1);
//	//glVertexAttribDivisor(3, 1);
//	//glVertexAttribDivisor(4, 1);
//	//glVertexAttribDivisor(5, 1);
//}
