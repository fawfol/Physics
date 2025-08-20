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

/* =================================================================================
 * Aerodynamic Terminal Simulator - V4 (Lift & Drag Gauges)
 * =================================================================================
 * Description:
 * A 2D aerodynamic simulation that visualizes lift and drag forces.
 * Use 'w' and 's' to change the flap's angle and see the forces change.
 *
 * How to Compile:
 * gcc -o aero_sim_v4 aero_sim_v4.c -lncurses -lm
 *
 * How to Run:
 * ./aero_sim_v4
 * =================================================================================
 */

#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define MAX_PARTICLES 10000
#define INITIAL_DENSITY 0.3
#define INITIAL_SPEED 0.8

// A simple 2D vector for physics calculations
typedef struct {
    float x, y;
} Vector2D;

typedef struct {
    Vector2D pos; // Position
    Vector2D vel; // Velocity
} Particle;

typedef enum {
    SHAPE_FLAP,
    SHAPE_AEROFOIL,
    SHAPE_CIRCLE,
    SHAPE_SQUARE
} ShapeType;

typedef struct {
    ShapeType type;
    Vector2D pos;
    Vector2D size;
    float angle; // Angle in radians for the flap
} Shape;

// A central struct to hold the entire simulation state
typedef struct {
    Particle particles[MAX_PARTICLES];
    int num_particles;
    int screen_width, screen_height;
    float air_speed;
    float air_density;
    Shape object;
    Vector2D total_force; // NEW: To accumulate forces from collisions
} SimState;

// --- Function Prototypes ---
void init_simulation(SimState *state);
void reset_particle(SimState *state, int i);
void handle_particle_collision(Particle *p, const Shape *object);
void update_simulation(SimState *state);
void draw_frame(const SimState *state);
void show_menu(SimState *state);

// --- Main Loop ---
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

        // --- REAL-TIME FLAP CONTROL ---
        if (state.object.type == SHAPE_FLAP) {
            if (ch == 'w' || ch == 'W') state.object.angle -= 0.1;
            if (ch == 's' || ch == 'S') state.object.angle += 0.1;
        }

        update_simulation(&state);
        draw_frame(&state);
        usleep(16000); // ~60 FPS
    }

    endwin();
    return 0;
}

// --- Simulation Initialization ---
void init_simulation(SimState *state) {
    getmaxyx(stdscr, state->screen_height, state->screen_width);
    
    state->air_speed = INITIAL_SPEED;
    state->air_density = INITIAL_DENSITY;
    state->total_force = (Vector2D){0, 0}; // Initialize forces
    
    state->object.type = SHAPE_FLAP;
    state->object.size = (Vector2D){25, 4};
    state->object.pos = (Vector2D){state->screen_width / 3, state->screen_height / 2};
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
    state->particles[i].vel.y = ((float)rand() / RAND_MAX - 0.5f) * 0.1f;
}


// --- Physics and Collision ---
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
        return (fabs(rotated_x) < object->size.x / 2.0f && fabs(rotated_y) < object->size.y / 2.0f);
    }
    
    // Bounding box for non-rotating shapes
    float half_w = object->size.x / 2.0f;
    float half_h = object->size.y / 2.0f;
    if (x < center_x - half_w || x >= center_x + half_w || y < center_y - half_h || y >= center_y + half_h) {
        return 0; 
    }
    // Specific shape checks for non-rectangular shapes
    switch (object->type) {
        case SHAPE_SQUARE: return 1;
        case SHAPE_CIRCLE: {
            float radius = object->size.x / 2.0f;
            float aspect = 2.0f;
            float dx = (x - center_x);
            float dy = (y - center_y) * aspect;
            return (dx * dx + dy * dy < radius * radius);
        }
        case SHAPE_AEROFOIL: {
            float norm_x = (x - (center_x - half_w)) / object->size.x;
            if (norm_x < 0 || norm_x > 1) return 0;
            float thickness = 0.5f * (0.2969f * sqrtf(norm_x) - 0.1260f * norm_x - 0.3516f * powf(norm_x, 2) + 0.2843f * powf(norm_x, 3) - 0.1015f * powf(norm_x, 4));
            return fabs(y - center_y) < object->size.y * thickness;
        }
        default: return 0;
    }
}

void handle_particle_collision(Particle *p, const Shape *object) {
    Vector2D normal = {-1, 0}; // Default normal for head-on collision
    if (object->type == SHAPE_FLAP) {
        float cos_a = cosf(object->angle);
        float sin_a = sinf(object->angle);
        Vector2D local_vel = {p->vel.x * cos_a + p->vel.y * sin_a, -p->vel.x * sin_a + p->vel.y * cos_a};
        if (local_vel.y > 0) { normal = (Vector2D){sin_a, -cos_a}; } 
        else { normal = (Vector2D){-sin_a, cos_a}; }
    }
    
    float restitution = 0.4f, friction = 0.8f;
    float vel_dot_normal = p->vel.x * normal.x + p->vel.y * normal.y;
    Vector2D normal_vel = {normal.x * vel_dot_normal, normal.y * vel_dot_normal};
    Vector2D tangent_vel = {p->vel.x - normal_vel.x, p->vel.y - normal_vel.y};
    p->vel.x = (tangent_vel.x * friction) - (normal_vel.x * restitution);
    p->vel.y = (tangent_vel.y * friction) - (normal_vel.y * restitution);
}

void update_simulation(SimState *state) {
    state->total_force = (Vector2D){0, 0}; // Reset forces each frame

    for (int i = 0; i < state->num_particles; i++) {
        Particle *p = &state->particles[i];
        Vector2D last_pos = p->pos;
        Vector2D vel_before = p->vel;

        p->pos.x += p->vel.x;
        p->pos.y += p->vel.y;

        if (p->pos.x >= state->screen_width || p->pos.x < 0 || p->pos.y >= state->screen_height || p->pos.y < 0) {
            reset_particle(state, i);
            continue;
        }

        if (is_inside_shape((int)roundf(p->pos.x), (int)roundf(p->pos.y), &state->object)) {
            p->pos = last_pos;
            handle_particle_collision(p, &state->object);
            
            // --- ACCUMULATE FORCES ---
            // The force on the object is the opposite of the change in the particle's momentum
            state->total_force.x += vel_before.x - p->vel.x; // Drag
            state->total_force.y += vel_before.y - p->vel.y; // Lift
            
            p->pos.x += p->vel.x; // Bounce-out step
            p->pos.y += p->vel.y;
        } else {
            if (p->vel.x < state->air_speed) p->vel.x += 0.02f;
        }
    }
}


// --- Drawing and UI ---
void draw_shape(const Shape *object) {
    attron(A_REVERSE);
    float max_dim = fmax(object->size.x, object->size.y) * 1.5;
    for (int y = object->pos.y - max_dim / 2; y < object->pos.y + max_dim / 2; y++) {
        for (int x = object->pos.x - max_dim; x < object->pos.x + max_dim; x++) {
            if (is_inside_shape(x, y, object)) mvaddch(y, x, ' ');
        }
    }
    attroff(A_REVERSE);
}

void draw_force_gauges(const SimState *state) {
    int gauge_x = state->screen_width - 25;
    int gauge_y = 5;
    
    // Note: In physics, positive Y is up. In ncurses, it's down.
    // We calculate force so that positive force.y is UPWARD LIFT.
    float lift = state->total_force.y;
    float drag = state->total_force.x;

    // Scale the forces to make the bars visible
    float scale = 3.0f;
    int lift_bar = (int)(lift * scale);
    int drag_bar = (int)(drag * scale);
    
    mvprintw(gauge_y - 2, gauge_x, "--- FORCES ---");
    mvprintw(gauge_y, gauge_x, "LIFT");
    mvprintw(gauge_y + 5, gauge_x, "DRAG");
    
    // Draw LIFT gauge (can be positive or negative)
    mvaddch(gauge_y + 2, gauge_x + 4, '|');
    if (lift_bar > 0) { // Positive lift (Up)
        for (int i = 0; i < lift_bar && i < 15; i++) mvaddch(gauge_y + 1, gauge_x + 5 + i, '#');
    } else { // Negative lift (Down)
        for (int i = 0; i < -lift_bar && i < 15; i++) mvaddch(gauge_y + 3, gauge_x + 5 + i, '#');
    }

    // Draw DRAG gauge
    mvaddch(gauge_y + 5, gauge_x + 4, '|');
    for (int i = 0; i < drag_bar && i < 15; i++) mvaddch(gauge_y + 5, gauge_x + 5 + i, '=');
}

void draw_frame(const SimState *state) {
    clear();
    for (int i = 0; i < state->num_particles; i++) {
        mvaddch((int)roundf(state->particles[i].pos.y), (int)roundf(state->particles[i].pos.x), '.');
    }
    draw_shape(&state->object);
    draw_force_gauges(state); // Draw the new UI

    const char* shape_name;
    switch(state->object.type) {
        case SHAPE_FLAP: shape_name = "Flap"; break;
        case SHAPE_SQUARE: shape_name = "Square"; break;
        case SHAPE_AEROFOIL: shape_name = "Aerofoil"; break;
        case SHAPE_CIRCLE: shape_name = "Circle"; break;
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
             case SHAPE_FLAP: shape_name = "Flap"; break;
             case SHAPE_SQUARE: shape_name = "Square"; break;
             case SHAPE_AEROFOIL: shape_name = "Aerofoil"; break;
             case SHAPE_CIRCLE: shape_name = "Circle"; break;
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
                else if (state->object.type == SHAPE_AEROFOIL) state->object.size = (Vector2D){25, 8};
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
            case 'm': case 'q': running = 0; break;
        }
    }
    nodelay(stdscr, TRUE);
}
