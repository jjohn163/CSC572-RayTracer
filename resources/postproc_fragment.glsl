#version 410 core
out vec4 color;
in vec2 vertex_tex;
uniform sampler2D tex;


void main()
{
vec4 tcol = texture(tex, vertex_tex);

color = tcol;
color.a=1;
}
