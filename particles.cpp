#include <SDL2/SDL.h>
#include <stdio.h>
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>

#define WIDTH 1280
#define HEIGHT 720
#define FRAMERATE 120
#define GRAVCONST 100000

using namespace std;

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

typedef struct
{
	float x, y;
} vec2;

typedef struct
{
	int type;
	int brightness;
	float mass;
	vec2 position;
	vec2 velocity;
	vec2 force;
} particle;

//	SDL variables
SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

//	particle variables
particle * particles;
int particles_size;
int particle_count;

char colors[10][3]; 

//	time variables
clock_t last_time;
clock_t actual_time;
float delta_time;

float time_to_update_fps;

//	input variables
vec2 mouse_pos;
bool input_up = false;
bool input_down = false;
bool input_left = false;
bool input_right = false;
bool input_quit = false;
bool input_lclick = false;
bool input_mclick = false;
bool input_rclick = false;
bool input_rclick_down = false;
bool input_rclick_down_reset = false;

int SDL_RenderFillCircle(SDL_Renderer * renderer, int x, int y, int radius)
{
	int offsetx, offsety, d;
	int status;

	offsetx = 0;
	offsety = radius;
	d = radius -1;
	status = 0;

	while (offsety >= offsetx) {
		status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx, x + offsety, y + offsetx);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety, x + offsetx, y + offsety);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety, x + offsetx, y - offsety);
		status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx, x + offsety, y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2*offsetx) {
			d -= 2*offsetx + 1;
			offsetx +=1;
		}
		else if (d < 2 * (radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	return status;
}

void draw()
{
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = WIDTH;
	rect.h = HEIGHT;
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
	SDL_RenderFillRect(renderer, &rect);

	for(int i = 0; i < particle_count; i++)
	{
		SDL_SetRenderDrawColor(renderer, colors[particles[i].type][0], colors[particles[i].type][1], colors[particles[i].type][2], SDL_ALPHA_OPAQUE);
		rect.x = particles[i].position.x - 2;
		rect.y = particles[i].position.y - 2;
		rect.w = 4;
		rect.h = 4;
		//SDL_RenderFillRect(renderer, &rect);
		SDL_RenderFillCircle(renderer, particles[i].position.x, particles[i].position.y, 4);
		//SDL_SetRenderDrawColor(renderer, 64, 64, 64, SDL_ALPHA_OPAQUE);
		//SDL_RenderDrawLine(renderer, particles[i].position.x, particles[i].position.y, particles[i].position.x + particles[i].velocity.x, particles[i].position.y + particles[i].velocity.y);
	}

	SDL_RenderPresent(renderer);
}

void create_particles(int count, int type)
{
	for(int i = 0; i < count; i++)
	{
		particles[particle_count].type = type;
		particles[particle_count].mass = 1;
		particles[particle_count].position.x = rand() % WIDTH;
		particles[particle_count].position.y = rand() % HEIGHT;
		particles[particle_count].velocity.x = 0;
		particles[particle_count].velocity.y = 0;
		particles[particle_count].force.x = 0;
		particles[particle_count].force.y = 0;
		particle_count++;
	}
}

void update()
{
	int dx = 0;
	int dy = 0;

	for(int i = 0; i < particle_count; i++)
	{
		particles[i].force.x = 0;
		particles[i].force.y = 0;

		for(int j = 0; j < particle_count; j++)
		{
			dx = -(particles[i].position.x - particles[j].position.x);
			dy = -(particles[i].position.y - particles[j].position.y);
			if(i != j && dx * dx + dy * dy > 0.01)
			{
				float multiplier = GRAVCONST / (dx * dx + dy * dy);
				particles[i].force.x += dx * multiplier;
				particles[i].force.y += dy * multiplier;
			}
		}

		/*
		dx = 0;
		dy = (particles[i].position.y);
		if(dx * dx + dy * dy > 0.01)
		{
			float multiplier = GRAVCONST / (dx * dx + dy * dy);
			particles[i].force.x += dx * multiplier;
			particles[i].force.y += dy * multiplier;
		}

		dx = (particles[i].position.x);
		dy = 0;
		if(dx * dx + dy * dy > 0.01)
		{
			float multiplier = GRAVCONST / (dx * dx + dy * dy);
			particles[i].force.x += dx * multiplier;
			particles[i].force.y += dy * multiplier;
		}

		dx = 0;
		dy = (particles[i].position.y - HEIGHT);
		if(dx * dx + dy * dy > 0.01)
		{
			float multiplier = GRAVCONST / (dx * dx + dy * dy);
			particles[i].force.x += dx * multiplier;
			particles[i].force.y += dy * multiplier;
		}

		dx = (particles[i].position.x - WIDTH);
		dy = 0;
		if(dx * dx + dy * dy > 0.01)
		{
			float multiplier = GRAVCONST / (dx * dx + dy * dy);
			particles[i].force.x += dx * multiplier;
			particles[i].force.y += dy * multiplier;
		}
		*/

		if(input_lclick)
		{
			dx = (particles[i].position.x - mouse_pos.x);
			dy = (particles[i].position.y - mouse_pos.y);
			if(dx * dx + dy * dy > 0.01)
			{
				float multiplier = 10 * GRAVCONST / (dx * dx + dy * dy);
				particles[i].force.x += dx * multiplier;
				particles[i].force.y += dy * multiplier;
			}
		}

		if(input_rclick)
		{
			dx = -(particles[i].position.x - mouse_pos.x);
			dy = -(particles[i].position.y - mouse_pos.y);
			if(dx * dx + dy * dy > 0.01)
			{
				float multiplier = 10 * GRAVCONST / (dx * dx + dy * dy);
				particles[i].force.x += dx * multiplier;
				particles[i].force.y += dy * multiplier;
			}
		}

		float air_friction_x = -0.001 * particles[i].velocity.x * particles[i].velocity.x;
		float air_friction_y = -0.001 * particles[i].velocity.y * particles[i].velocity.y;

		particles[i].velocity.x += (particles[i].force.x / particles[i].mass + sgn(particles[i].velocity.x) * air_friction_x) * delta_time;
		particles[i].velocity.y += (particles[i].force.y / particles[i].mass + sgn(particles[i].velocity.y) * air_friction_y) * delta_time;
	}

	for(int i = 0; i < particle_count; i++)
	{
		particles[i].position.x += particles[i].velocity.x * delta_time;
		particles[i].position.y += particles[i].velocity.y * delta_time;
		if(particles[i].position.x < 0)
		{
			particles[i].position.x = 0;
			particles[i].velocity.x *= -1;
		}
		if(particles[i].position.x >= WIDTH)
		{
			particles[i].position.x = WIDTH - 1;
			particles[i].velocity.x *= -1;
		}
		if(particles[i].position.y < 0)
		{
			particles[i].position.y = 0;
			particles[i].velocity.y *= -1;
		}
		if(particles[i].position.y >= HEIGHT)
		{
			particles[i].position.y = HEIGHT - 1;
			particles[i].velocity.y *= -1;
		}
	}
}

void catch_input(){
	SDL_Event event;
	while(SDL_PollEvent(&event)){
		int key_code = event.key.keysym.scancode;
		if(event.type == SDL_QUIT || event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
			input_quit = true;
		}
		else if(event.type == SDL_KEYDOWN){
			if(key_code == SDL_SCANCODE_UP || key_code == SDL_SCANCODE_W) input_up = true;
			else if(key_code == SDL_SCANCODE_DOWN || key_code == SDL_SCANCODE_S) input_down = true;
			else if(key_code == SDL_SCANCODE_LEFT || key_code == SDL_SCANCODE_A) input_left = true;
			else if(key_code == SDL_SCANCODE_RIGHT || key_code == SDL_SCANCODE_D) input_right = true;
		}
		else if(event.type == SDL_KEYUP){
			if(key_code == SDL_SCANCODE_UP || key_code == SDL_SCANCODE_W) input_up = false;
			else if(key_code == SDL_SCANCODE_DOWN || key_code == SDL_SCANCODE_S) input_down = false;
			else if(key_code == SDL_SCANCODE_LEFT || key_code == SDL_SCANCODE_A) input_left = false;
			else if(key_code == SDL_SCANCODE_RIGHT || key_code == SDL_SCANCODE_D) input_right = false;
		}
		else if(event.type == SDL_MOUSEMOTION)
		{
			mouse_pos.x = event.motion.x;
			mouse_pos.y = event.motion.y;
		}
		else if(event.type == SDL_MOUSEBUTTONDOWN)
		{
			if(event.button.button == SDL_BUTTON_LEFT)
				input_lclick = true;
			else if(event.button.button == SDL_BUTTON_MIDDLE)
				input_mclick = true;
			else if(event.button.button == SDL_BUTTON_RIGHT)
			{
				if(input_rclick_down_reset)
					input_rclick_down = true;
				input_rclick = true;
				input_rclick_down_reset = false;
			}
		}
		else if(event.type == SDL_MOUSEBUTTONUP)
		{
			if(event.button.button == SDL_BUTTON_LEFT)
				input_lclick = false;
			else if(event.button.button == SDL_BUTTON_MIDDLE)
				input_mclick = false;
			else if(event.button.button == SDL_BUTTON_RIGHT)
			{
				input_rclick_down_reset = true;
				input_rclick = false;
			}
		}
	}
}

void run()
{
	while(!input_quit)
	{
		catch_input();

		update();
		draw();

		actual_time = clock();
		delta_time = float(actual_time - last_time) / (float) CLOCKS_PER_SEC;
		last_time = actual_time;

		time_to_update_fps -= delta_time;
		if(time_to_update_fps <= 0)
		{
			time_to_update_fps = 0.5;

			string fps_string = to_string((int) (1 / delta_time));
			char fps_chars[fps_string.length() + 1];
			strcpy(fps_chars, fps_string.c_str());

			SDL_SetWindowTitle(window, fps_chars);
		}
	}
}

int main(int argc, char *argv[])
{
	if(argc < 1)
		return 1;

	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
		SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	srand(time(NULL));
	last_time = clock();
	time_to_update_fps = 0.5;

	colors[0][0] = 236;
	colors[0][1] = 28;
	colors[0][2] = 94;

	colors[1][0] = 161;
	colors[1][1] = 28;
	colors[1][2] = 124;

	colors[2][0] = 90;
	colors[2][1] = 33;
	colors[2][2] = 114;

	colors[3][0] = 5;
	colors[3][1] = 28;
	colors[3][2] = 161;

	colors[4][0] = 28;
	colors[4][1] = 118;
	colors[4][2] = 236;

	colors[5][0] = 0;
	colors[5][1] = 0;
	colors[5][2] = 255;

	colors[6][0] = 255;
	colors[6][1] = 0;
	colors[6][2] = 255;

	mouse_pos.x = 0;
	mouse_pos.y = 0;

	particles_size = 0;
	particle_count = 0;

	for(int i = 1; i < argc; i++)
		particles_size += atoi(argv[i]);

	particles = (particle *) malloc(particles_size * sizeof(particle));

	for(int i = 1; i < argc; i++)
		create_particles(atoi(argv[i]), i - 1);

	run();

	SDL_Quit();
	return 0;
}