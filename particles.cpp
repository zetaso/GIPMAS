#include <SDL2/SDL.h>
#include <stdio.h>
#include <iostream>
#include <cmath>
#include <ctime>
#include <cstring>

#define WIDTH 1920.0
#define HEIGHT 1017.0
#define UNIT 192.0
#define FRAMERATE 120.0
#define TIME_MULTIPLIER 1.0

#define FRICTION 0.1
#define MAX_SPEED 2.5
#define MAX_FORCE 25.0

#define FIXMULTIPLIER 0.5
#define GRAVCONST 0.25
#define WALL_GRAVCONST 2
//#define CLICK_GRAVCONST 20.0

#define PARTICLE_RADIUS 4.0

#define VISION_RADIUS 1
#define COLLISION_RADIUS 0.15
#define WALL_COLLISION_RADIUS 10

using namespace std;

float entropy;

bool random = false;

float sign(float v)
{
	return v >= 0 ? 1 : -1;
}

typedef struct
{
	float x, y;
} vec2;

typedef struct
{
	int type;
	int color[3];
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
int type_count;
int *particle_count_by_type;

float UNIT_WIDTH = WIDTH / (float) UNIT;
float UNIT_HEIGHT = HEIGHT / (float) UNIT;

float multipliers[][4] = {{0.46, 1.96, 1.14, 1.62},
							{-1.36, -0.96, 0.2, 1.6},
							{1.78, 1.44, -1.36, 0.76},
							{0.18, -1.04, 0.08, -1.38}};

//float multipliers[][4] = {{ 3, 9,-7,10},
//						  {10, 5, 9,-10},
//						  { 4, 3,-3,-2},
//						  { 5,-7,-1, 3}};

//float multipliers[][4] = {{ 1,-1, 1, 1},	//GRUMOS INESTABLES (distance)
//						  { 1, 1,-1,-1},
//						  {-1,-1,-1,-1},
//						  { 1, 1,-1, 0}};

char colors[4][3];

//	time variables
clock_t last_time;
clock_t actual_time;
float delta_time;

float time_to_new_matrix = 0.0;
float new_matrix_interval = 10.0;

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
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
	SDL_RenderFillRect(renderer, &rect);

	for(int i = 0; i < particle_count; i++)
	{
		if(filtering && !input_filter[particles[i].type])
			SDL_SetRenderDrawColor(renderer, 10, 10, 10, SDL_ALPHA_OPAQUE);
		else
			SDL_SetRenderDrawColor(renderer, particles[i].color[0], particles[i].color[1], particles[i].color[2], SDL_ALPHA_OPAQUE);
		rect.x = particles[i].position.x  * UNIT - 8;
		rect.y = particles[i].position.y * UNIT - 8;
		rect.w = 16;
		rect.h = 16;
		//SDL_RenderFillRect(renderer, &rect);
		SDL_RenderFillCircle(renderer, particles[i].position.x * UNIT, particles[i].position.y * UNIT, PARTICLE_RADIUS);
		//SDL_SetRenderDrawColor(renderer, 0, 255, 255, SDL_ALPHA_OPAQUE);
		//SDL_RenderDrawLine(renderer, particles[i].position.x * UNIT, particles[i].position.y * UNIT, particles[i].position.x * UNIT + particles[i].velocity.x * UNIT, particles[i].position.y * UNIT + particles[i].velocity.y * UNIT);
		//SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
		//SDL_RenderDrawLine(renderer, particles[i].position.x * UNIT, particles[i].position.y * UNIT, particles[i].position.x * UNIT + particles[i].force.x * UNIT, particles[i].position.y * UNIT + particles[i].force.y * UNIT);
	}

	SDL_RenderPresent(renderer);
}

void reset_particles()
{
	for(int i = 0; i < particle_count; i++)
	{
		particles[i].position.x = rand() % (int) WIDTH / UNIT;
		particles[i].position.y = rand() % (int) HEIGHT / UNIT;
		particles[i].velocity.x = 0;
		particles[i].velocity.y = 0;
		particles[i].force.x = 0;
		particles[i].force.y = 0;
	}
}

void create_particles(int count, int type)
{
	for(int i = 0; i < count; i++)
	{
		particles[particle_count].type = type;
		particles[particle_count].position.x = rand() % (int) WIDTH / UNIT;
		particles[particle_count].position.y = rand() % (int) HEIGHT / UNIT;
		particles[particle_count].velocity.x = 0;
		particles[particle_count].velocity.y = 0;
		particles[particle_count].force.x = 0;
		particles[particle_count].force.y = 0;
		particles[particle_count].color[0] = colors[type][0];
		particles[particle_count].color[1] = colors[type][1];
		particles[particle_count].color[2] = colors[type][2];
		particle_count++;
	}
}

void update()
{
	float dx = 0;
	float dy = 0;
	float distance = 0;
	float sqrDistance = 0;
	float multiplier = 1;
	float force = 0;
	entropy = 0;

	for(int i = 0; i < particle_count; i++)
	{
		particles[i].force.x = 0;
		particles[i].force.y = 0;

		for(int j = 0; j < particle_count; j++)
		{
			if(i == j)
				continue;

			dx = (particles[j].position.x - particles[i].position.x);
			dy = (particles[j].position.y - particles[i].position.y);
			distance = sqrt(dx * dx + dy * dy);

			//float radio = ((PARTICLE_RADIUS - 2) / UNIT);
			//if(distance < (2.0 * radio) && (particles[i].position.x != particles[j].position.x || particles[i].position.y != particles[j].position.y))
			//{
			//	vec2 punto_medio;
			//	punto_medio.x = (particles[i].position.x + particles[j].position.x) / 2.0;
			//	punto_medio.y = (particles[i].position.y + particles[j].position.y) / 2.0;
			//	
			//	vec2 normal;
			//	normal.x = particles[j].position.x - particles[i].position.x;
			//	normal.y = particles[j].position.y - particles[i].position.y;
			//	
			//	float normal_mag = sqrt(normal.x * normal.x + normal.y * normal.y);
			//	normal.x /= normal_mag;
			//	normal.y /= normal_mag;
			//	
			//	particles[j].position.x = punto_medio.x + normal.x * radio;
			//	particles[j].position.y = punto_medio.y + normal.y * radio;
			//	
			//	particles[i].position.x = punto_medio.x - normal.x * radio;
			//	particles[i].position.y = punto_medio.y - normal.y * radio;
//
			//	particles[i].velocity.x *= (1 - FRICTION * 5);
			//	particles[i].velocity.y *= (1 - FRICTION * 5);
			//}
			
			if(distance > 0 && distance < VISION_RADIUS && distance >= COLLISION_RADIUS)
			{
				//float n = 2.0 * abs(distance - 0.5 * (VISION_RADIUS + 0));
				//float d = VISION_RADIUS - 0;
				//float f = GRAVCONST * multipliers[particles[i].type][particles[j].type];
				//force = f * (1.0 - n / d);
				//particles[i].force.x += dx / distance * force;
				//particles[i].force.y += dy / distance * force;
				multiplier = multipliers[particles[i].type][particles[j].type];
				force = GRAVCONST / distance;
				particles[i].force.x += dx / distance * force * multiplier;
				particles[i].force.y += dy / distance * force * multiplier;
			}
			else if(distance > 0 && distance < COLLISION_RADIUS)
			{
				multiplier = -1;
				force = FIXMULTIPLIER * GRAVCONST / distance;
				//force = FIXCONST;
				particles[i].force.x += dx / distance * force * multiplier;
				particles[i].force.y += dy / distance * force * multiplier;
			}
		}
		
		dx = 0;
		dy = particles[i].position.y;
		distance = sqrt(dx * dx + dy * dy);
		sqrDistance = distance * distance;
		if(distance > 0 && distance < WALL_COLLISION_RADIUS)
		{
			force = WALL_GRAVCONST / distance;
			particles[i].force.x += dx / distance * force;
			particles[i].force.y += dy / distance * force;
		}
		
		dx = particles[i].position.x;
		dy = 0;
		distance = sqrt(dx * dx + dy * dy);
		sqrDistance = distance * distance;
		if(distance > 0 && distance < WALL_COLLISION_RADIUS)
		{
			force = WALL_GRAVCONST / distance;
			particles[i].force.x += dx / distance * force;
			particles[i].force.y += dy / distance * force;
		}
		
		dx = 0;
		dy = particles[i].position.y - UNIT_HEIGHT;
		distance = sqrt(dx * dx + dy * dy);
		sqrDistance = distance * distance;
		if(distance > 0 && distance < WALL_COLLISION_RADIUS)
		{
			force = WALL_GRAVCONST / distance;
			particles[i].force.x += dx / distance * force;
			particles[i].force.y += dy / distance * force;
		}
		
		dx = particles[i].position.x - UNIT_WIDTH;
		dy = 0;
		distance = sqrt(dx * dx + dy * dy);
		sqrDistance = distance * distance;
		if(distance > 0 && distance < WALL_COLLISION_RADIUS)
		{
			force = WALL_GRAVCONST / distance;
			particles[i].force.x += dx / distance * force;
			particles[i].force.y += dy / distance * force;
		}
	

		//if(input_lclick)
		//{
		//	dx = particles[i].position.x - mouse_pos.x;
		//	dy = particles[i].position.y - mouse_pos.y;
		//	distance = dx * dx + dy * dy;
		//	if(distance > 1)
		//	{
		//		force = CLICK_GRAVCONST / distance;
		//		particles[i].force.x += dx * force;
		//		particles[i].force.y += dy * force;
		//	}
		//}
		//
		//if(input_rclick)
		//{
		//	dx = mouse_pos.x - particles[i].position.x;
		//	dy = mouse_pos.y - particles[i].position.y;
		//	distance = dx * dx + dy * dy;
		//	if(distance > 1)
		//	{
		//		force = CLICK_GRAVCONST / distance;
		//		particles[i].force.x += dx * force;
		//		particles[i].force.y += dy * force;
		//	}
		//}


		particles[i].velocity.x += particles[i].force.x * delta_time * TIME_MULTIPLIER;
		particles[i].velocity.y += particles[i].force.y * delta_time * TIME_MULTIPLIER;
		float speed = sqrt(particles[i].velocity.x * particles[i].velocity.x + particles[i].velocity.y * particles[i].velocity.y);
		float new_speed = speed * (1 - FRICTION);
		if(speed > 0)
		{
			particles[i].velocity.x = particles[i].velocity.x / speed * new_speed;
			particles[i].velocity.y = particles[i].velocity.y / speed * new_speed;
		}
		if(speed > MAX_SPEED)
		{
			particles[i].velocity.x = particles[i].velocity.x / speed * MAX_SPEED;
			particles[i].velocity.y = particles[i].velocity.y / speed * MAX_SPEED;
		}

		entropy += sqrt(particles[i].velocity.x * particles[i].velocity.x + particles[i].velocity.y * particles[i].velocity.y);
	}

	for(int i = 0; i < particle_count; i++)
	{
		particles[i].position.x += particles[i].velocity.x * delta_time * TIME_MULTIPLIER;
		particles[i].position.y += particles[i].velocity.y * delta_time * TIME_MULTIPLIER;

		if(particles[i].position.x < PARTICLE_RADIUS / UNIT)
		{
			particles[i].position.x = PARTICLE_RADIUS / UNIT;
			particles[i].velocity.x *= -0.25;
			//particles[i].velocity.x = 0;
		}
		if(particles[i].position.x > UNIT_WIDTH - PARTICLE_RADIUS / UNIT)
		{
			particles[i].position.x = UNIT_WIDTH - PARTICLE_RADIUS / UNIT;
			particles[i].velocity.x *= -0.25;
			//particles[i].velocity.x = 0;
		}
		if(particles[i].position.y < PARTICLE_RADIUS / UNIT)
		{
			particles[i].position.y = PARTICLE_RADIUS / UNIT;
			particles[i].velocity.y *= -0.25;
			//particles[i].velocity.y = 0;
		}
		if(particles[i].position.y > UNIT_HEIGHT - PARTICLE_RADIUS / UNIT)
		{
			particles[i].position.y = UNIT_HEIGHT - PARTICLE_RADIUS / UNIT;
			particles[i].velocity.y *= -0.25;
			//particles[i].velocity.y = 0;
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

		if(random)
		{
			time_to_new_matrix -= delta_time;
			if(time_to_new_matrix <= 0)
			{
				time_to_new_matrix = new_matrix_interval;
				reset_particles();

				for (int i = 0; i < 4; ++i)
				{
					for (int j = 0; j < 4; ++j)
					{
						multipliers[i][j] = (rand() % 101) / 50.0 * (rand() % 2 == 0 ? 1 : -1);
						if(j != 3) cout << multipliers[i][j] << ", ";
					}
					cout << endl;
				}
				cout << endl;
			}
		}

		time_to_update_fps -= delta_time;
		if(time_to_update_fps <= 0)
		{
			time_to_update_fps = 0.5;

			string fps_string = to_string(entropy);
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

	if(random)
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				multipliers[i][j] = (rand() % 101) / 50.0 * (rand() % 2 == 0 ? 1 : -1);
				if(j != 3) cout << multipliers[i][j] << ", ";
			}
			cout << endl;
		}

	for(int i = 1; i < argc; i++)
		particles_size += atoi(argv[i]);

	particles = (particle *) malloc(particles_size * sizeof(particle));

	particle_count_by_type = (int*) malloc((argc - 1) * sizeof(int));
	type_count = argc - 1;
	for(int i = 0; i < type_count; i++)
	{
		particle_count_by_type[i] = atoi(argv[i + 1]);
		create_particles(particle_count_by_type[i], i);
	}

	run();

	SDL_Quit();
	return 0;
}