#version 450 
#define EPS 0.001
#define RESX 640.0
#define RESY 480.0
#define BUFFERSIZE 2
#define RAND_BUFFERSIZE 4 * 640 * 480
#define BACKGROUND vec3(0.6,0.6,1.0)
#define GROUND_NORMAL vec3(0,1,0)
#define GROUND_HEIGHT -2.0
#define GROUND_COLOR vec3(0.5,0.5,0.5)
#define MAX_DEPTH 5
#define SPP 3
layout(local_size_x = 1, local_size_y = 1) in;											//local group of shaders
layout(rgba32f, binding = 0) uniform image2D img_input;									//input image
layout(rgba32f, binding = 1) uniform image2D img_output;
layout(rgba32f, binding = 2) uniform image2D normal_depth;//output image
layout(std430, binding = 3) volatile buffer shader_data
{
	vec4 sphere_pos[BUFFERSIZE];
	vec4 sphere_col[BUFFERSIZE];
	float rand_buffer[RAND_BUFFERSIZE];
};

uniform int mode;

void main() 
	{
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	vec4 col;
	int location = int(pixel_coords.y * RESX + pixel_coords.x);
	if (mode == 0) {
		col = imageLoad(img_input, pixel_coords);
	}
	else if (mode == 1) { //display the normals
		col = vec4(imageLoad(normal_depth, pixel_coords).xyz, 1.0);
	}
	else {//display the depth
		float depth = 1.0 - min(1.0, imageLoad(normal_depth, pixel_coords).w / 20.0);
		col = vec4(depth, depth, depth, 1);
	}
	
	imageStore(img_input, pixel_coords, col);
	}