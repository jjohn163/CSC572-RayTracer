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
layout(rgba32f, binding = 1) uniform image2D img_output;									//output image
layout(rgba32f, binding = 2) uniform image2D normal_depth;
layout(std430, binding = 3) volatile buffer shader_data
{
    vec4 sphere_pos[BUFFERSIZE];
	vec4 sphere_col[BUFFERSIZE];
	float rand_buffer[RAND_BUFFERSIZE];
};


uniform vec3 horizontal;
uniform vec3 vertical;
uniform vec3 LLC;
uniform vec3 eye;
uniform float time;


float rand(ivec2 coords, int offset) {
	return rand_buffer[int(mod(coords.y * RESX + coords.x + time * 17 + offset * 37, RAND_BUFFERSIZE))];
}

void main() 
	{
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);				
	vec4 col=vec4(0);

	//setup ray from camera through current pixel
	
	
	int rand_counter = 0;
	vec3 total_samples_color = vec3(0, 0, 0);
	
	for (int s = 0; s < SPP; s++) {
		float vp = (pixel_coords.y + rand(pixel_coords, rand_counter++)) / RESY;
		float hp = (pixel_coords.x + rand(pixel_coords, rand_counter++)) / RESX;
		vec3 ray_position = eye;
		vec3 ray_direction = LLC + (horizontal * hp) + (vertical * vp) - eye;
		vec3 new_ray_position = ray_position;
		vec3 new_ray_direction = ray_direction;
		int depth = MAX_DEPTH;
		vec3 hit_color = BACKGROUND;
		vec3 color_accumulator = vec3(1, 1, 1);
		float t = 10000000;
		float reflectance = 1.0;
		while (depth > 0) {
			hit_color = BACKGROUND;
			t = 10000000;
			if (depth != MAX_DEPTH) {
				ray_position = new_ray_position;
				ray_direction = normalize(reflect(ray_direction, new_ray_direction));
				vec3 center = ray_position + ray_direction;
				vec3 dir = vec3(1, 1, 1);
				while (length(dir) > 1.0) {
					dir.x = rand(pixel_coords, rand_counter++);
					dir.y = rand(pixel_coords, rand_counter++);
					dir.z = rand(pixel_coords, rand_counter++);
				}
				ray_direction = normalize((center + dir * reflectance) - ray_position);
			}

			//check for hits in the scene with other spheres
			for (int i = 0; i < BUFFERSIZE; i++) {
				vec3 v = ray_position - sphere_pos[i].xyz;
				float a = dot(ray_direction, ray_direction);
				float b = 2.0 * dot(ray_direction, v);
				float c = dot(v, v) - sphere_pos[i].w * sphere_pos[i].w;
				float discriminant = b * b - 4.0 * a * c;
				if (discriminant >= 0) { //ray hit the sphere
					float cur_t = (-b - sqrt(discriminant)) / (2.0 * a);
					if (cur_t < t && cur_t > 0.01) { //hit was closer than any other previous hit
						t = cur_t;
						new_ray_position = ray_position + ray_direction * t;
						new_ray_direction = normalize(new_ray_position - sphere_pos[i].xyz);
						new_ray_position = new_ray_position + new_ray_direction * 0.00001;
						hit_color = sphere_col[i].rgb;
						reflectance = sphere_col[i].w;
					}
				}
			}
			//check for hit with ground plane
			float b = dot(ray_direction, GROUND_NORMAL);
			if (b != 0) {
				float cur_t = (GROUND_HEIGHT - dot(ray_position, GROUND_NORMAL)) / b;
				if (cur_t < t && cur_t > 0.01) { //hit is closer than previous hit
					t = cur_t;
					new_ray_position = ray_position + ray_direction * t + (GROUND_NORMAL * 0.00001);
					new_ray_direction = GROUND_NORMAL;
					hit_color = GROUND_COLOR;
				}
			}
			if (depth == MAX_DEPTH) {
				int location = int(pixel_coords.y * RESX + pixel_coords.x);
				imageStore(normal_depth, pixel_coords, vec4((new_ray_direction + 1.0) / 2.0, t));
			}
			if (t >= 9999999) { //no hit
				depth = 0; //break out of loop
			}
			color_accumulator *= hit_color;
			depth--;
		}
		total_samples_color += color_accumulator;
	}
	
	col.xyz = total_samples_color;

	//Gamma correction
	float scale = 1.0 / SPP;
	col.x = sqrt(scale * col.x);
	col.y = sqrt(scale * col.y);
	col.z = sqrt(scale * col.z);
	
	imageStore(img_output, pixel_coords, col);
	}