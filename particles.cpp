#include <SDL2/SDL.h>
#include <stdio.h>
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>

#define WIDTH 1280
#define HEIGHT 720
#define FRAMERATE 120
#define FRICTION 0.05
#define GRAVCONST 10000
#define WALL_GRAVCONST 50000
#define CLICK_GRAVCONST 1000000

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
	vec2 position;
	vec2 velocity;
	vec2 force;
} particle;

//	SDL variables
SDL_Window *window = SDL_CreateWindow("Arthificial life from a chaotic particle system", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE);
SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

//	particle variables
particle * particles;
int particles_size;
int particle_count;

float multipliers[][4] = {{ 1,-2, 1, 1},
						  { 2, 1, 1, 1},
						  {-1,-1,-1,-1},
						  {-1,-1,-1,-1}};

char colors[4][3];

//	time variables
clock_t last_time;
clock_t actual_time;
float delta_time;

float time_to_update_fps;

//	input variables
vec2 mouse_pos;
bool filtering = false;
bool input_filter[] = {false, false, false, false};
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
	filtering = input_filter[0] || input_filter[1] || input_filter[2] || input_filter[3];

	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = WIDTH;
	rect.h = HEIGHT;
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 64);
	SDL_RenderFillRect(renderer, &rect);

	for(int i = 0; i < particle_count; i++)
	{
		if(filtering && !input_filter[particles[i].type])
			SDL_SetRenderDrawColor(renderer, 10, 10, 10, SDL_ALPHA_OPAQUE);
		else
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
	float dx = 0;
	float dy = 0;
	float distance;

	for(int i = 0; i < particle_count; i++)
	{

		particles[i].force.x = 0;
		particles[i].force.y = 0;

		for(int j = 0; j < particle_count; j++)
		{
			dx = particles[j].position.x - particles[i].position.x;
			dy = particles[j].position.y - particles[i].position.y;
			if(dx * dx + dy * dy > 0)
				distance = sqrt(dx * dx + dy * dy);
			else
				distance = 1;
			if(i != j && distance > 8 && distance < 200)
			{
				float multiplier = multipliers[particles[i].type][particles[j].type];
				float force = GRAVCONST / distance;
				particles[i].force.x += dx / distance * force * multiplier;
				particles[i].force.y += dy / distance * force * multiplier;
			}
		}
		
		dx = 0;
		dy = particles[i].position.y;
		distance = sqrt(dx * dx + dy * dy);
		if(distance > 1 && distance < 100)
		{
			float force = WALL_GRAVCONST / distance;
			particles[i].force.x += dx / distance * force;
			particles[i].force.y += dy / distance * force;
		}
		
		dx = particles[i].position.x;
		dy = 0;
		distance = sqrt(dx * dx + dy * dy);
		if(distance > 1 && distance < 100)
		{
			float force = WALL_GRAVCONST / distance;
			particles[i].force.x += dx / distance * force;
			particles[i].force.y += dy / distance * force;
		}
		
		dx = 0;
		dy = particles[i].position.y - HEIGHT;
		distance = sqrt(dx * dx + dy * dy);
		if(distance > 1 && distance < 100)
		{
			float force = WALL_GRAVCONST / distance;
			particles[i].force.x += dx / distance * force;
			particles[i].force.y += dy / distance * force;
		}
		
		dx = particles[i].position.x - WIDTH;
		dy = 0;
		distance = sqrt(dx * dx + dy * dy);
		if(distance > 1 && distance < 100)
		{
			float force = WALL_GRAVCONST / distance;
			particles[i].force.x += dx / distance * force;
			particles[i].force.y += dy / distance * force;
		}
	

		if(input_lclick)
		{
			dx = particles[i].position.x - mouse_pos.x;
			dy = particles[i].position.y - mouse_pos.y;
			distance = dx * dx + dy * dy;
			if(distance > 1)
			{
				float force = CLICK_GRAVCONST / distance;
				particles[i].force.x += dx * force;
				particles[i].force.y += dy * force;
			}
		}

		if(input_rclick)
		{
			dx = mouse_pos.x - particles[i].position.x;
			dy = mouse_pos.y - particles[i].position.y;
			distance = dx * dx + dy * dy;
			if(distance > 1)
			{
				float force = CLICK_GRAVCONST / distance;
				particles[i].force.x += dx * force;
				particles[i].force.y += dy * force;
			}
		}

		float air_friction_x = FRICTION * particles[i].velocity.x * particles[i].velocity.x;
		float air_friction_y = FRICTION * particles[i].velocity.y * particles[i].velocity.y;

		particles[i].velocity.x += (particles[i].force.x - sgn(particles[i].velocity.x) * air_friction_x) * delta_time;
		particles[i].velocity.y += (particles[i].force.y - sgn(particles[i].velocity.y) * air_friction_y) * delta_time;
	}

	for(int i = 0; i < particle_count; i++)
	{
		particles[i].position.x += particles[i].velocity.x * delta_time;
		particles[i].position.y += particles[i].velocity.y * delta_time;
		if(particles[i].position.x < 6)
		{
			particles[i].position.x = 6;
			particles[i].velocity.x *= -1;
		}
		if(particles[i].position.x >= WIDTH - 6)
		{
			particles[i].position.x = WIDTH - 6 - 1;
			particles[i].velocity.x *= -1;
		}
		if(particles[i].position.y < 6)
		{
			particles[i].position.y = 6;
			particles[i].velocity.y *= -1;
		}
		if(particles[i].position.y >= HEIGHT - 6)
		{
			particles[i].position.y = HEIGHT - 6 - 1;
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
			else if(key_code == SDL_SCANCODE_1) input_filter[0] = true;
			else if(key_code == SDL_SCANCODE_2) input_filter[1] = true;
			else if(key_code == SDL_SCANCODE_3) input_filter[2] = true;
			else if(key_code == SDL_SCANCODE_4) input_filter[3] = true;
		}
		else if(event.type == SDL_KEYUP){
			if(key_code == SDL_SCANCODE_UP || key_code == SDL_SCANCODE_W) input_up = false;
			else if(key_code == SDL_SCANCODE_DOWN || key_code == SDL_SCANCODE_S) input_down = false;
			else if(key_code == SDL_SCANCODE_LEFT || key_code == SDL_SCANCODE_A) input_left = false;
			else if(key_code == SDL_SCANCODE_RIGHT || key_code == SDL_SCANCODE_D) input_right = false;
			else if(key_code == SDL_SCANCODE_1) input_filter[0] = false;
			else if(key_code == SDL_SCANCODE_2) input_filter[1] = false;
			else if(key_code == SDL_SCANCODE_3) input_filter[2] = false;
			else if(key_code == SDL_SCANCODE_4) input_filter[3] = false;
		}
		else if(event.type == SDL_MOUSEMOTION)
		{
			mouse_pos.x = event.motion.x;
			mouse_pos.y = event.motion.y;
		}
		else if(event.type == SDL_MOUSEBUTTONDOWN)
		{
			if(event.button.button == SDL_BUTTON_LEFT);
				//input_lclick = true;
			else if(event.button.button == SDL_BUTTON_MIDDLE)
				input_mclick = true;
			else if(event.button.button == SDL_BUTTON_RIGHT)
			{
				if(input_rclick_down_reset)
					input_rclick_down = true;
				//input_rclick = true;
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

	colors[0][0] = 255;
	colors[0][1] = 220;
	colors[0][2] = 118;

	colors[1][0] = 248;
	colors[1][1] = 101;
	colors[1][2] = 93;

	colors[2][0] = 193;
	colors[2][1] = 30;
	colors[2][2] = 181;

	colors[3][0] = 73;
	colors[3][1] = 21;
	colors[3][2] = 190;

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