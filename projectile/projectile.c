#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>

//deine gravitational constants for various celestial bodies
#define G_earth 9.8
#define G_moon 1.62
#define G_mars 3.711
#define G_saturn 10.44
#define G_jupiter 24.79
#define G_sun 274

//scale factor for converting simulation coordinates to screen coordinates
#define SCALE 2.0

//forward declaration for the settings menu function
void valueSet_menu(double *mass, double *drag_coefficient, double *gravity, double *power, int width, int height);


/*******@brief moves the terminal cursor to a specific (x, y) position***************************/	
void move_cursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

/***********checks if a key has been pressed in the terminal without blocking return 1 if a key is pressed or 0 otherwise*****************/
int kbhit(void) {
    struct termios oldt, newt;
    int oldf;
    struct timeval tv = {0L, 0L};
    fd_set readfds;

    //get current terminal settings
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    //disable canonical mode and echo
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    //check for input
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);

    int hit = FD_ISSET(STDIN_FILENO, &readfds);
    //restore old terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return hit;
}

/***************gets a character from the terminal without waiting for the Enter key and return the character code of the key pressed*********************/
int getch_nonblock(void) {
    int ch;
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

/********************Clears the terminal screen using ANSI escape codes**********************************************/
void clear_screen() {
    printf("\033[H\033[J");
}

int main() {
    //get terminal dimensions
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int width = ws.ws_col;
    int height = ws.ws_row - 4; //reserve 4 rows for status info

    //allocate screen buffer
    char **screen = malloc(height * sizeof(char *));
    if (screen == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    for (int y = 0; y < height; y++) {
        screen[y] = malloc(width * sizeof(char));
        if (screen[y] == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            //free previously allocated memory before exiting
            for (int i = 0; i < y; i++) free(screen[i]);
            free(screen);
            return 1;
        }
    }

    //siumlation vars
    double aim_x = 10, aim_y = 5;
    double v0 = 0, angle = 0;
    double px = 0, py = 0, vx = 0, vy = 0;
    
    //physics properties
    double mass = 1.0; //kg
    double drag_coefficient = 0.47; //sphere
    double gravity = G_earth;
    double power = 1.0; //throw power multiplier

    //show the settings menu to the user before starting
    valueSet_menu(&mass, &drag_coefficient, &gravity, &power, ws.ws_col, ws.ws_row);
    
    //game stateh 0=Aiming & 1=Flying & 2=Landed  .. no bounce
    int gameState = 0;
    double final_distance = 0.0;

    //time and fps calculation variables
    struct timespec ts_prev, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_prev);
    double fps = 0.0;

    // Main game loop
    while (1) {
        // Calculate delta time (dt) for physics calculations
        clock_gettime(CLOCK_MONOTONIC, &ts_now);
        double dt = (ts_now.tv_sec - ts_prev.tv_sec) +
                    (ts_now.tv_nsec - ts_prev.tv_nsec) / 1e9;
        ts_prev = ts_now;
        if (dt > 0) fps = 0.9 * fps + 0.1 * (1.0 / dt); //fps counter smooth

        //input handling
        if (kbhit()) {
            int c = getch_nonblock();
            if (c == 'q') break;
            
            if (gameState == 0) { 
                if (c == 'w' && aim_y < height - 2) aim_y++;
                if (c == 's' && aim_y > 1) aim_y--;
                if (c == 'a' && aim_x > 1) aim_x--;
                if (c == 'd' && aim_x < width - 2) aim_x++;
                if (c == 'm') {
                    valueSet_menu(&mass, &drag_coefficient, &gravity, &power, ws.ws_col, ws.ws_row);
                }
                if (c == '\n' || c ==' ') {
                    double dx = aim_x;
                    double dy = aim_y;
                    //calculate initial velocity based on aim and power
                    v0 = (sqrt(dx*dx + dy*dy) / SCALE) * power;
                    angle = atan2(dy, dx);
                    px = 0; py = 0;
                    vx = v0 * cos(angle);
                    vy = v0 * sin(angle);
                    gameState = 1; //switch to flying state
                }
            } else if (gameState == 2) { //landed state
                gameState = 0;
            }
            
            if (c == 'r') { //reset simulation
                gameState = 0;
                aim_x = 10;
                aim_y = 5;
            }
        }

        //physic updation
        if (gameState == 1) {
            double speed = sqrt(vx*vx + vy*vy);
            //calculate drag force
            double force_drag_magnitude = 0.5 * drag_coefficient * speed * speed;
            double force_drag_x = -force_drag_magnitude * (vx / (speed + 1e-9)); //avoid division by zero <3
            double force_drag_y = -force_drag_magnitude * (vy / (speed + 1e-9));
            
            //net force is drag + gravity
            double force_net_x = force_drag_x;
            double force_net_y = force_drag_y - (gravity * mass);

            //calculate acceleration (a = f/m)
            double ax = force_net_x / mass;
            double ay = force_net_y / mass;

            //euler integration to velcoity and positon updation
            vx += ax * dt;
            vy += ay * dt;
            px += vx * dt;
            py += vy * dt;
            
            //check for collision with the ground
            if (py < 0) {
                final_distance = px;
                gameState = 2; //switch to landed state
            }
        }

        //draw canvas
        //clear the screen buffer
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                screen[y][x] = ' ';

        //draw ground
        for (int x = 0; x < width; x++) screen[0][x] = '_';

        //draw based on game state
        if (gameState == 1) { //flying
            int sx = (int)(px * SCALE);
            int sy = (int)(py * SCALE);
            if (sx >= 0 && sx < width && sy >= 0 && sy < height)
                screen[sy][sx] = 'O'; //draw projectile
        } else if (gameState == 0) {
            screen[(int)aim_y][(int)aim_x] = '+';
        } else if (gameState == 2) {
            char message[100];
            sprintf(message, "Distance Covered: %.2f meters", final_distance);
            int msg_start_x = (width - strlen(message)) / 2;
            int y_pos = height / 2;
            for(int i = 0; i < strlen(message); i++) {
                if ((msg_start_x + i) < width && (msg_start_x + i) >= 0) {
                    screen[y_pos][msg_start_x + i] = message[i];
                }
            }
        }

        //render to terminal
        clear_screen();
        
        //print status infos
        if (gameState == 1) {
            printf("FPS: %.1f | H-Speed: %.2f m/s | V-Speed: %.2f m/s\n", fps, vx, vy);
            printf("Mass: %.2f kg | Gravity: %.2f m/s^2\n", mass, gravity);
            printf("\n");
        } else if (gameState == 0) {
            double potential_v0 = (sqrt(aim_x*aim_x + aim_y*aim_y) / SCALE) * power;
            double visual_x = aim_x /2;
            double potential_angle = atan2(aim_y, visual_x);
            double potential_vx = potential_v0 * cos(potential_angle);
            double potential_vy = potential_v0 * sin(potential_angle);
            
            printf("FPS: %.1f | Potential H-Speed: %.2f m/s | Potential V-Speed: %.2f m/s\n", fps, potential_vx, potential_vy);
            printf("Cursor Angle: %.1f deg | Mass: %.2f kg | Power: %.1fx | Planet (G=%.2f)\n", potential_angle * 180.0 / M_PI, mass, power, gravity);
            printf("[W/S/A/D] to Aim | [Enter] to Launch | [M] Menu | [R] Reset | [Q] Quit\n");
        } else {
            printf("FPS: %.1f | Landed!\n", fps);
            printf("Final distance is shown below.\n");
            printf("Press any key to aim again.\n");
        }

        //print the screen buffer from bottom to top
        for (int y = height-1; y >= 0; y--) {
            for (int x = 0; x < width; x++)
                putchar(screen[y][x]);
            putchar('\n');
        }

        //60fps
        usleep(16000);
    }

    ///free
    for (int y = 0; y < height; y++) {
        free(screen[y]);
    }
    free(screen);

    return 0;
}

/***************center menu: set physic values : mass, dragCoeff, gravity, powerTimes, width and height of terminal***************************/
void valueSet_menu(double *mass, double *drag_coefficient, double *gravity, double *power, int width, int height) {
    clear_screen();
    int choice;
    char buffer[256];
    
    
    int current_y = (height - 18) / 2;
    if (current_y < 1) current_y = 1;

    //title
    const char* title = "[----------PROJECTILE SIMULATION SETTING----------]";
    move_cursor((width - strlen(title)) / 2, current_y++);
    printf("%s", title);
    current_y++;

    //get user input for params
    sprintf(buffer, "------------CURRENT MASS: %.2f kg----------------", *mass);
    move_cursor((width - strlen(buffer)) / 2, current_y++);
    printf("%s", buffer);
    const char* mass_prompt = "ENTER NEW MASS (kg): ";
    move_cursor((width - strlen(mass_prompt) - 5) / 2, current_y++);
    printf("%s", mass_prompt);
    scanf("%lf", mass);

    sprintf(buffer, "------------CURRENT DRAG COEFF : %.2f------------", *drag_coefficient);
    move_cursor((width - strlen(buffer)) / 2, current_y++);
    printf("%s", buffer);
    const char* drag_prompt = "ENTER NEW DRAG COEFFICIENT: ";
    move_cursor((width - strlen(drag_prompt) - 5) / 2, current_y++);
    printf("%s", drag_prompt);
    scanf("%lf", drag_coefficient);

    sprintf(buffer, "-----CURRENT THROW POWER MULTIPLIER: %.2fx------", *power);
    move_cursor((width - strlen(buffer)) / 2, current_y++);
    printf("%s", buffer);
    const char* power_prompt = "ENTER NEW THROW MULTIPLIER: ";
    move_cursor((width - strlen(power_prompt) - 5) / 2, current_y++);
    printf("%s", power_prompt);
    scanf("%lf", power);
    current_y++;

    // gravity
    const char* g_title = "---------[SELECT GRAVITY]---------";
    move_cursor((width - strlen(g_title)) / 2, current_y++);
    printf("%s", g_title);

    const char* g_earth_s = "1. EARTH   (9.80 m/s^2)";
    move_cursor((width - strlen(g_earth_s)) / 2, current_y++);
    printf("%s", g_earth_s);
    
    const char* g_moon_s = "2. MOON   (1.62 m/s^2)";
    move_cursor((width - strlen(g_moon_s)) / 2, current_y++);
    printf("%s", g_moon_s);
    
    const char* g_mars_s = "3. MARS    (3.71 m/s^2)";
    move_cursor((width - strlen(g_mars_s)) / 2, current_y++);
    printf("%s", g_mars_s);

    const char* g_saturn_s = "4. SATURN    (10.44 m/s^2)";
    move_cursor((width - strlen(g_saturn_s)) / 2, current_y++);
    printf("%s", g_saturn_s);

    const char* g_jupiter_s = "5. JUPITER    (24.79 m/s^2)";
    move_cursor((width - strlen(g_jupiter_s)) / 2, current_y++);
    printf("%s", g_jupiter_s);
    
    const char* g_sun_s = "6. SUN (274 m/s^2)";
    move_cursor((width - strlen(g_sun_s)) / 2, current_y++);
    printf("%s", g_sun_s);
    
    const char* g_custom_s = "7. CUSTOM ?";
    move_cursor((width - strlen(g_custom_s)) / 2, current_y++);
    printf("%s", g_custom_s);
    
    const char* choice_prompt = "ENTER YOUR CHOICE : ";
    move_cursor((width - strlen(choice_prompt) - 2) / 2, current_y++);
    printf("%s", choice_prompt);
    scanf("%d", &choice);

    switch(choice) {
        case 1: *gravity = G_earth; break;
        case 2: *gravity = G_moon; break;
        case 3: *gravity = G_mars; break;
        case 4: *gravity = G_saturn; break;
        case 5: *gravity = G_jupiter; break;
        case 6: *gravity = G_sun; break;
        case 7: {
            const char* custom_g_prompt = "Enter custom gravity (m/s^2): ";
            move_cursor((width - strlen(custom_g_prompt) - 5) / 2, current_y++);
            printf("%s", custom_g_prompt);
            scanf("%lf", gravity);
            break;
        }
        default:
            *gravity = G_earth;
            break;
    }
    
    // Clear the input buffer for preventing issues with next input call in the infin loop
    while (getchar() != '\n'); 

    const char* final_msg = "Settings updated. Press ENTER to return to the simulation...";
    move_cursor((width - strlen(final_msg)) / 2, ++current_y);
    printf("%s", final_msg);
    getchar(); //wait for user to press enter to continue
}
