/* =================================================================================
 * Aerodynamic Terminal Simulator - V3 (Interactive Flap)
 * =================================================================================
 * Description:
 * A 2D aerodynamic simulation with an interactable controlled flap for that ;
 * Use 'w' and 's' to change the flap's angle during the simulation.
 *
 * How to Compile:
 * gcc -o aero_sim_v3 aero_sim_v3.c -lncurses -lm
 *
 * How to Run:
 * ./aero_sim_v3
 * =================================================================================
 */

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define MAX_PARTICLES 10000
#define INITIAL_DENSITY 0.25
#define INITIAL_SPEED 0.8

typedef struct {
    float x, y;
} Vector2D;

typedef struct {
    Vector2D pos; 
    Vector2D vel; 
} Particle;

typedef enum {
    SHAPE_SQUARE,
    SHAPE_AEROFOIL,
    SHAPE_CIRCLE,
    SHAPE_FLAP 
} ShapeType;

typedef struct {
    ShapeType type;
    Vector2D pos;
    Vector2D size;
    float angle; 
} Shape;

typedef struct {
    Particle particles[MAX_PARTICLES];
    int num_particles;
    int screen_width, screen_height;
    float air_speed;
    float air_density;
    Shape object;
} SimState;

void init_simulation(SimState *state);
void reset_particle(SimState *state, int i);
void handle_particle_collision(Particle *p, const Shape *object);
void update_simulation(SimState *state);
void draw_frame(const SimState *state);
void show_menu(SimState *state);

int main() {
    initscr();
    noecho();
    cbreak();
    curs_set(0);
    nodelay(stdscr, TRUE);
    srand(time(NULL));

    SimState state;
    init_simulation(&state);

    while (1) {
        int ch = getch();
        if (ch == 'q') break;
        if (ch == 'm') show_menu(&state);

        if (state.object.type == SHAPE_FLAP) {
            if (ch == 'w' || ch == 'W') {
                state.object.angle -= 0.1; 
            }
            if (ch == 's' || ch == 'S') {
                state.object.angle += 0.1; 
            }
        }

        update_simulation(&state);
        draw_frame(&state);
        usleep(16000); 
    }

    endwin();
    return 0;
}

void init_simulation(SimState *state) {
    getmaxyx(stdscr, state->screen_height, state->screen_width);

    state->air_speed = INITIAL_SPEED;
    state->air_density = INITIAL_DENSITY;

    state->object.type = SHAPE_FLAP;
    state->object.size = (Vector2D){25, 4}; 
    state->object.pos = (Vector2D){
        state->screen_width / 3,
        state->screen_height / 2
    };
    state->object.angle = 0.0f; 

    state->num_particles = (int)(MAX_PARTICLES * state->air_density);
    if (state->num_particles > MAX_PARTICLES) state->num_particles = MAX_PARTICLES;

    for (int i = 0; i < state->num_particles; i++) {
        state->particles[i].pos = (Vector2D){(float)(rand() % state->screen_width), (float)(rand() % state->screen_height)};
        state->particles[i].vel = (Vector2D){state->air_speed, 0};
    }
}

void reset_particle(SimState *state, int i) {
    state->particles[i].pos.x = 0;
    state->particles[i].pos.y = (float)(rand() % state->screen_height);
    state->particles[i].vel.x = state->air_speed + ((float)rand() / RAND_MAX - 0.5f) * 0.2f;
    state->particles[i].vel.y = 0;
}

int is_inside_shape(int x, int y, const Shape *object) {
    float center_x = object->pos.x;
    float center_y = object->pos.y;

    if (object->type == SHAPE_FLAP) {

        float dx = x - center_x;
        float dy = y - center_y;
        float cos_a = cosf(-object->angle); 
        float sin_a = sinf(-object->angle);

        float rotated_x = dx * cos_a - dy * sin_a;
        float rotated_y = dx * sin_a + dy * cos_a;

        return (fabs(rotated_x) < object->size.x / 2.0f &&
                fabs(rotated_y) < object->size.y / 2.0f);
    }

    if (x < object->pos.x - object->size.x / 2.0f || x >= object->pos.x + object->size.x / 2.0f ||
        y < object->pos.y - object->size.y / 2.0f || y >= object->pos.y + object->size.y / 2.0f) {
        return 0; 
    }

    switch (object->type) {
        case SHAPE_SQUARE:
            return 1;
        case SHAPE_CIRCLE: {
            float radius = object->size.x / 2.0f;
            float aspect = 2.0f; 
            float dx = (x - center_x);
            float dy = (y - center_y) * aspect;
            return (dx * dx + dy * dy < radius * radius);
        }
        case SHAPE_AEROFOIL: {
            float norm_x = (x - (object->pos.x - object->size.x/2.0f)) / object->size.x; 
            float thickness = 0.5f * (0.2969f * sqrtf(norm_x) - 0.1260f * norm_x - 0.3516f * powf(norm_x, 2) + 0.2843f * powf(norm_x, 3) - 0.1015f * powf(norm_x, 4));
            float half_h = object->size.y * thickness;
            return fabs(y - center_y) < half_h;
        }
        case SHAPE_FLAP: 
            return 1;
    }
    return 0;
}

void handle_particle_collision(Particle *p, const Shape *object) {
    Vector2D normal = {0, 0};

    if (object->type == SHAPE_FLAP) {

        float cos_a = cosf(object->angle);
        float sin_a = sinf(object->angle);

        Vector2D local_vel = {
            p->vel.x * cos_a + p->vel.y * sin_a,
           -p->vel.x * sin_a + p->vel.y * cos_a
        };

        if (local_vel.y > 0) { 
            normal.x = sin_a;
            normal.y = -cos_a;
        } else { 
            normal.x = -sin_a;
            normal.y = cos_a;
        }

    } else { 
        normal.x = -1.0f;
        normal.y = 0.0f;
    }

    float restitution = 0.4f;
    float friction = 0.8f;
    float vel_dot_normal = p->vel.x * normal.x + p->vel.y * normal.y;
    Vector2D normal_vel = {normal.x * vel_dot_normal, normal.y * vel_dot_normal};
    Vector2D tangent_vel = {p->vel.x - normal_vel.x, p->vel.y - normal_vel.y};
    normal_vel.x *= -restitution;
    normal_vel.y *= -restitution;
    tangent_vel.x *= friction;
    tangent_vel.y *= friction;
    p->vel.x = normal_vel.x + tangent_vel.x;
    p->vel.y = normal_vel.y + tangent_vel.y;
}

void update_simulation(SimState *state) {
    for (int i = 0; i < state->num_particles; i++) {
        Particle *p = &state->particles[i];
        Vector2D last_pos = p->pos;

        p->pos.x += p->vel.x;
        p->pos.y += p->vel.y;
        p->vel.y *= 0.98f;

        if (p->pos.x >= state->screen_width || p->pos.x < 0 || p->pos.y >= state->screen_height || p->pos.y < 0) {
            reset_particle(state, i);
            continue;
        }

        if (is_inside_shape((int)roundf(p->pos.x), (int)roundf(p->pos.y), &state->object)) {
            p->pos = last_pos;
            handle_particle_collision(p, &state->object);
            p->pos.x += p->vel.x;
            p->pos.y += p->vel.y;
        } else {
            if (p->vel.x < state->air_speed) {
                p->vel.x += 0.02f;
            }
        }
    }
}

void draw_shape(const Shape *object) {
    attron(A_REVERSE);

    float max_dim = fmax(object->size.x, object->size.y);
    int start_x = object->pos.x - max_dim;
    int end_x = object->pos.x + max_dim;
    int start_y = object->pos.y - max_dim;
    int end_y = object->pos.y + max_dim;

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            if (is_inside_shape(x, y, object)) {
                mvaddch(y, x, ' ');
            }
        }
    }
    attroff(A_REVERSE);
}

void draw_frame(const SimState *state) {
    clear();
    for (int i = 0; i < state->num_particles; i++) {
        mvaddch((int)roundf(state->particles[i].pos.y), (int)roundf(state->particles[i].pos.x), '.');
    }
    draw_shape(&state->object);

    const char* shape_name;
    switch(state->object.type) {
        case SHAPE_SQUARE: shape_name = "Square"; break;
        case SHAPE_AEROFOIL: shape_name = "Aerofoil"; break;
        case SHAPE_CIRCLE: shape_name = "Circle"; break;
        case SHAPE_FLAP: shape_name = "Flap"; break;
        default: shape_name = "Unknown"; break;
    }

    attron(A_REVERSE);
    mvprintw(state->screen_height - 1, 1, "Speed: %.2f | Density: %.2f | Shape: %s",
             state->air_speed, state->air_density, shape_name);

    if (state->object.type == SHAPE_FLAP) {
        mvprintw(state->screen_height - 2, 1, " Angle: %.2f rad | Controls: W/S ", state->object.angle);
    }

    mvprintw(state->screen_height - 1, state->screen_width - 20, "Press 'm' for Menu ");
    attroff(A_REVERSE);

    refresh();
}

void show_menu(SimState *state) {
    nodelay(stdscr, FALSE);

    int menu_width = 45, menu_height = 8;
    int menu_x = state->screen_width / 2 - menu_width / 2;
    int menu_y = state->screen_height / 2 - menu_height / 2;

    int running = 1;
    while(running) {
        attron(A_REVERSE);
        for(int y=0; y<menu_height; ++y) mvhline(menu_y + y, menu_x, ' ', menu_width);
        mvprintw(menu_y + 1, menu_x + 2, "--- SETTINGS MENU ---");
        attroff(A_REVERSE);

        const char* shape_name;
        switch(state->object.type) {
             case SHAPE_SQUARE: shape_name = "Square"; break;
             case SHAPE_AEROFOIL: shape_name = "Aerofoil"; break;
             case SHAPE_CIRCLE: shape_name = "Circle"; break;
             case SHAPE_FLAP: shape_name = "Flap"; break;
             default: shape_name = "Unknown"; break;
        }

        mvprintw(menu_y + 3, menu_x + 2, "1. Change Shape (Current: %s)", shape_name);
        mvprintw(menu_y + 4, menu_x + 2, "2. Change Air Speed (Current: %.2f)", state->air_speed);
        mvprintw(menu_y + 5, menu_x + 2, "3. Change Air Density (Current: %.2f)", state->air_density);
        mvprintw(menu_y + 6, menu_x + 2, "Press 'm' or 'q' to exit menu");

        int choice = getch();
        switch (choice) {
            case '1':
                state->object.type = (state->object.type + 1) % 4; 

                state->object.pos = (Vector2D){state->screen_width / 3, state->screen_height / 2};
                if(state->object.type == SHAPE_FLAP) state->object.size = (Vector2D){25, 4};
                else state->object.size = (Vector2D){15, 15};
                break;
            case '2':
                state->air_speed += 0.2;
                if (state->air_speed > 2.0) state->air_speed = 0.2;
                break;
            case '3':
                state->air_density += 0.1;
                if (state->air_density > 1.0) state->air_density = 0.1;
                init_simulation(state);
                break;
            case 'm':
            case 'q':
                running = 0;
                break;
        }
    }

    nodelay(stdscr, TRUE);
}
