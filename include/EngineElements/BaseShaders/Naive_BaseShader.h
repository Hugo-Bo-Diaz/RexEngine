#ifndef NAIVE_SHADER_BASE__H
#define NAIVE_SHADER_BASE__H

const char* NaiveVertexShader = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec2 UV;\n"
"out vec2 vTexCoords;\n"
"uniform mat4 modelMatrix;\n"
"uniform mat4 ViewProjection;\n"
"void main()\n"
"{\n"
"vTexCoords = UV;\n"
"gl_Position = ViewProjection * modelMatrix * vec4(aPos, 1.0);\n"
"}";
const char* NaiveFragmentShader =
"#version 330 core\n"
"in vec2 vTexCoords;\n"
"out vec4 FragColor;\n"
"uniform vec4 ourColor;\n"
"uniform sampler2D ourTexture;\n"
"void main()\n"
"{\n"
"FragColor = texture(ourTexture, vTexCoords);\n"
"//FragColor = vec4(vTexCoords.xy,0.0f,1.0f);\n"
"}";

#endif // !NAIVE_SHADER_BASE__H