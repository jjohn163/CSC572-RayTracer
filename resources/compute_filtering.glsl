#version 450 
#define EPS 0.001
#define RESX 640.0
#define RESY 480.0
#define BUFFERSIZE 3
#define RAND_BUFFERSIZE 10000139
#define BACKGROUND vec3(0.6,0.6,1.0)
#define GROUND_NORMAL vec3(0,1,0)
#define GROUND_HEIGHT -2.0
#define GROUND_COLOR vec3(0.5,1.0,0.5)
#define MAX_DEPTH 20
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

bool in_bounds(vec2 coords) {
	return coords.x >= 0 && coords.x < RESX && coords.y >= 0 && coords.y < RESY;
}

void main() 
	{
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	vec4 col;
	int location = int(pixel_coords.y * RESX + pixel_coords.x);
	if (mode == 0) { //display the original output
		col = imageLoad(img_input, pixel_coords);
	}
	else if (mode == 1) { //display the denoised output
		float weight = 0;
		col = vec4(0, 0, 0, 1);
		vec3 cur_normal = imageLoad(normal_depth, pixel_coords).xyz * 2.0 - 1;
		float cur_depth = imageLoad(normal_depth, pixel_coords).w;
		float cur_lum = col.r * 0.2126 + col.g * 0.7152 + col.b * 0.0722; //calculation for luminance
		for (int x = -2; x <= 2; x++) {
			for (int y = -2; y <= 2; y++) {
				//in bounds and is not the center
				if (in_bounds(vec2(pixel_coords.x + x, pixel_coords.y + y)) && (x != 0 || y != 0)) {
					ivec2 neighbor = ivec2(pixel_coords.x + x, pixel_coords.y + y);
					vec4 neighbor_color = imageLoad(img_output, neighbor);
					vec3 neighbor_normal = imageLoad(normal_depth, neighbor).xyz * 2.0 - 1;
					float neighbor_depth = imageLoad(normal_depth, neighbor).w;
					float neighbor_lum = neighbor_color.r * 0.2126 + neighbor_color.g * 0.7152 + neighbor_color.b * 0.0722;
					float wN = dot(cur_normal, neighbor_normal);
					float wZ = min(0, 0.1 - abs(cur_depth - neighbor_depth));
					float wL = 1.0 - abs(cur_lum - neighbor_lum);
					float neighbor_weight = (wN + wZ + wL);
					if (neighbor_weight > 0.1) {
						col += neighbor_color * neighbor_weight;
						weight += neighbor_weight;
					}
					
				}
			}
		}
		col /= weight;
	}
	else if (mode == 2) { //display the blurred output
		float weight = 0;
		col = vec4(0, 0, 0, 1);
		for (int x = -2; x <= 2; x++) {
			for (int y = -2; y <= 2; y++) {
				if (in_bounds(vec2(pixel_coords.x + x, pixel_coords.y + y))) {
					ivec2 neighbor = ivec2(pixel_coords.x + x, pixel_coords.y + y);
					vec4 neighbor_color = imageLoad(img_output, neighbor);
					col += neighbor_color;
					weight += 1;
				}
			}
		}
		col /= weight;
	}
	else if (mode == 3) {//display the depth
		float depth = 1.0 - min(1.0, imageLoad(normal_depth, pixel_coords).w / 20.0);
		col = vec4(depth, depth, depth, 1);
	}
	else if (mode == 4) { //display the normals
		col = vec4(imageLoad(normal_depth, pixel_coords).xyz, 1.0);
	}
	else { //display the luminance
		col = imageLoad(img_input, pixel_coords);
		float cur_lum = col.r * 0.2126 + col.g * 0.7152 + col.b * 0.0722;
		col = vec4(cur_lum, cur_lum, cur_lum, col.w);
	}

	
	imageStore(img_input, pixel_coords, col);
}