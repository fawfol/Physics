/*   =================================================================================
                  Aerodynamic Terminal Simulator
      ===============================================================================
	 Description:
	 simple 2D aerodynamic simulation inside a terminal window and uses the ncurses
	 library to render air particls flowing around a user-selectable shape
	
	 realistic maybe particle deflection to better visualize concepts like drag 
	 and pressure zones
	 
	Features:
	 Air particles flowing from left to right
	 collision and deflection physics
	 Adjustable air speed and density and shi
 
	How to Compile:
  	 You need to have the ncurses library installed. On Debian/Ubuntu, you can
         install it with: sudo apt-get install libncurses5-dev

	Compile the code using gcc:
	 gcc -o aero_sim aero_sim.c -lncurses -lm

	Run with ./aero_sim
 

    =================================================================================
 */

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_PARTICLES 10000
#define INITIAL_DENSITY 0.25 
#define INITIAL_SPEED 0.5

typedef enum {
    SHAPE_SQUARE,
    SHAPE_RECTANGLE,
    SHAPE_TRIANGLE,
    SHAPE_CIRCLE
} ShapeType;

typedef struct {
    float x, y; 
    float vx, vy; 
} Particle;

Particle particles[MAX_PARTICLES];
int num_particles;
int screen_width, screen_height;
float air_speed = INITIAL_SPEED;
float air_density = INITIAL_DENSITY;
ShapeType current_shape = SHAPE_SQUARE;

int shape_x, shape_y;
int shape_w, shape_h;

void init_simulation();
void reset_particle(int i);
int is_inside_shape(int x, int y);
void update_simulation();
void draw_frame();
void draw_shape();
void show_menu();

int main() {

    initscr();
    noecho();
    cbreak();
    curs_set(0);
    nodelay(stdscr, TRUE); 
    srand(time(NULL));

    init_simulation();

    while (1) {
        int ch = getch();
        if (ch == 'q') {
            break; 
        }
        if (ch == 'm') {
            show_menu(); 
        }

        update_simulation();
        draw_frame();
        usleep(16000); 
    }

    endwin();
    return 0;
}

void init_simulation() {
    getmaxyx(stdscr, screen_height, screen_width);

    shape_w = 10;
    shape_h = 10;
    shape_x = screen_width / 2 - shape_w / 2;
    shape_y = screen_height / 2 - shape_h / 2;

    num_particles = (int)(MAX_PARTICLES * air_density);
    if (num_particles > MAX_PARTICLES) num_particles = MAX_PARTICLES;

    for (int i = 0; i < num_particles; i++) {
        particles[i].x = (float)(rand() % screen_width);
        particles[i].y = (float)(rand() % screen_height);
        particles[i].vx = air_speed;
        particles[i].vy = 0;
    }
}

void reset_particle(int i) {
    particles[i].x = 0;
    particles[i].y = (float)(rand() % screen_height);

    particles[i].vx = air_speed + ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
    particles[i].vy = 0; 
}

int is_inside_shape(int x, int y) {

    if (x < shape_x || x >= shape_x + shape_w || y < shape_y || y >= shape_y + shape_h) {
        return 0;
    }

    switch (current_shape) {
        case SHAPE_SQUARE:
        case SHAPE_RECTANGLE:
            return 1; 
        case SHAPE_TRIANGLE: {

            float normalized_x = (float)(x - shape_x) / shape_w;
            float half_height_at_x = (shape_h / 2.0f) * normalized_x;
            float center_y = shape_y + shape_h / 2.0f;
            if (fabs(y - center_y) < half_height_at_x) {
                return 1;
            }
            break;
        }
        case SHAPE_CIRCLE: {

            float center_x = shape_x + shape_w / 2.0f;
            float center_y = shape_y + shape_h / 2.0f;
            float radius = shape_w / 2.0f;
            float dx = x - center_x;
            float dy = y - center_y;

            float aspect_ratio = 2.0f;
            if ((dx * dx) + ((dy * dy) * aspect_ratio) < (radius * radius)) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

void update_simulation() {
    for (int i = 0; i < num_particles; i++) {
        int prev_x = (int)roundf(particles[i].x);

        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;

        particles[i].vy *= 0.95f;

        if (particles[i].x >= screen_width || particles[i].x < 0 || particles[i].y >= screen_height || particles[i].y < 0) {
            reset_particle(i);
            continue;
        }

        int current_x = (int)roundf(particles[i].x);
        int current_y = (int)roundf(particles[i].y);

        if (is_inside_shape(current_x, current_y)) {
            particles[i].x = (float)prev_x; 

            float center_y = shape_y + shape_h / 2.0f;
            float dy = particles[i].y - center_y;

            particles[i].vy = dy * 0.1f; 

            particles[i].vx = air_speed * 0.5f;

        } else {

            if (particles[i].vx < air_speed) {
                particles[i].vx += 0.05f;
            }
        }
    }
}

void draw_frame() {
    clear();

    for (int i = 0; i < num_particles; i++) {
        int px = (int)roundf(particles[i].x);
        int py = (int)roundf(particles[i].y);
        if (px >= 0 && px < screen_width && py >= 0 && py < screen_height) {
            mvaddch(py, px, '.');
        }
    }

    draw_shape();

    const char* shape_name;
    switch(current_shape) {
        case SHAPE_SQUARE: shape_name = "Square"; break;
        case SHAPE_RECTANGLE: shape_name = "Rectangle"; break;
        case SHAPE_TRIANGLE: shape_name = "Triangle"; break;
        case SHAPE_CIRCLE: shape_name = "Circle"; break;
        default: shape_name = "Unknown"; break;
    }
    attron(A_REVERSE);
    mvprintw(screen_height - 1, 1, "Speed: %.2f | Density: %.2f | Shape: %s",
             air_speed, air_density, shape_name);
    mvprintw(screen_height - 1, screen_width - 20, "Press 'm' for Menu ");
    attroff(A_REVERSE);

    refresh();
}

void draw_shape() {
    attron(A_REVERSE);
    for (int y = shape_y; y < shape_y + shape_h; y++) {
        for (int x = shape_x; x < shape_x + shape_w; x++) {
            if (is_inside_shape(x, y)) {
                mvaddch(y, x, ' ');
            }
        }
    }
    attroff(A_REVERSE);
}

void show_menu() {
    nodelay(stdscr, FALSE); 

    int menu_width = 45;
    int menu_height = 8;
    int menu_x = screen_width / 2 - menu_width / 2;
    int menu_y = screen_height / 2 - menu_height / 2;

    const char* shape_name;

    int running = 1;
    while(running) {

        switch(current_shape) {
            case SHAPE_SQUARE: shape_name = "Square"; break;
            case SHAPE_RECTANGLE: shape_name = "Rectangle"; break;
            case SHAPE_TRIANGLE: shape_name = "Triangle"; break;
            case SHAPE_CIRCLE: shape_name = "Circle"; break;
            default: shape_name = "Unknown"; break;
        }

        attron(A_REVERSE);
        for (int y = 0; y < menu_height; y++) {
            mvhline(menu_y + y, menu_x, ' ', menu_width);
        }
        mvprintw(menu_y + 1, menu_x + 2, "--- SETTINGS MENU ---");
        attroff(A_REVERSE);

        mvprintw(menu_y + 3, menu_x + 2, "1. Change Shape (Current: %s)", shape_name);
        mvprintw(menu_y + 4, menu_x + 2, "2. Change Air Speed (Current: %.2f)", air_speed);
        mvprintw(menu_y + 5, menu_x + 2, "3. Change Air Density (Current: %.2f)", air_density);
        mvprintw(menu_y + 6, menu_x + 2, "Press 'm' or 'q' to exit menu");

        int choice = getch();
        switch (choice) {
            case '1':
                current_shape = (current_shape + 1) % 4; 
                if (current_shape == SHAPE_SQUARE) { shape_w = 10; shape_h = 10; }
                else if (current_shape == SHAPE_RECTANGLE) { shape_w = 20; shape_h = 8; }
                else if (current_shape == SHAPE_TRIANGLE) { shape_w = 15; shape_h = 15; }
                else if (current_shape == SHAPE_CIRCLE) { shape_w = 12; shape_h = 12; }
                shape_x = screen_width / 2 - shape_w / 2;
                shape_y = screen_height / 2 - shape_h / 2;
                break;
            case '2':
                air_speed += 0.1;
                if (air_speed > 2.0) air_speed = 0.1;
                break;
            case '3':
                air_density += 0.05;
                if (air_density > 1.0) air_density = 0.05;

                init_simulation();
                break;
            case 'm':
            case 'q':
                running = 0;
                break;
        }
    }

    nodelay(stdscr, TRUE); 
}
