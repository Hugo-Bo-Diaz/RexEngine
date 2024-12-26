#ifndef RENDERQUAD_SHADER_BASE__H
#define RENDERQUAD_SHADER_BASE__H

const char* RenderQuadVertexShader =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec2 UV;\n"
"out vec2 vTexCoords;\n"
"uniform mat4 modelMatrix;\n"
"uniform mat4 ViewProjection;\n"
"void main()\n"
"{\n"
"	vTexCoords = UV;\n"
"	gl_Position = ViewProjection * modelMatrix * vec4(aPos, 1.0);\n"
"}";
const char* RenderQuadFragmentShader =
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec2 vTexCoords;\n"
"uniform vec4 ourColor;\n"
"uniform int borderOnly;\n"
"uniform float aspect;\n"
"uniform float borderSize;\n"
"void main()\n"
"{\n"
"	if(borderOnly == 0)\n"
"		FragColor = ourColor;\n"
"	else\n"
"	{\n"
"		float minX = borderSize;\n"
"		float maxX = 1-borderSize;\n"
"		float minY = minX * aspect;\n"
"		float maxY = 1-minY;\n"
"		if(vTexCoords.x < minX ||vTexCoords.y < minY ||vTexCoords.x > maxX ||vTexCoords.y > maxY)\n"
"			FragColor = ourColor;\n"
"		else\n"
"			FragColor = vec4(0.0f);\n"
"	}\n"
"}";

#endif // !RENDERQUAD_SHADER_BASE__H