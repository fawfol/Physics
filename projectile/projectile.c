#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>

#define G 9.8
#define SCALE 2.0

int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;
    struct timeval tv = {0L, 0L};
    fd_set readfds;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
    int hit = FD_ISSET(STDIN_FILENO, &readfds);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return hit;
}

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

void clear_screen() {
    printf("\033[H\033[J");
}

int main() {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    int width = ws.ws_col;
    int height = ws.ws_row - 4;

    char **screen = malloc(height * sizeof(char *));
    if (screen == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    for (int y = 0; y < height; y++) {
        screen[y] = malloc(width * sizeof(char));
        if (screen[y] == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }
    }

    double aim_x = 10, aim_y = 5;
    double v0 = 0, angle = 0;
    double px = 0, py = 0, vx = 0, vy = 0;
    double gravity = G;
    
    int gameState = 0; // 0=aiming, 1=flying, 2=landed
    double final_distance = 0.0;

    struct timespec ts_prev, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_prev);
    double fps = 0.0;

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &ts_now);
        double dt = (ts_now.tv_sec - ts_prev.tv_sec) +
                    (ts_now.tv_nsec - ts_prev.tv_nsec) / 1e9;
        ts_prev = ts_now;
        if (dt > 0) fps = 0.9 * fps + 0.1 * (1.0 / dt);

        if (kbhit()) {
            int c = getch_nonblock();
            if (c == 'q') break;
            
            if (gameState == 0) {
                if (c == 'w' && aim_y < height - 2) aim_y++;
                if (c == 's' && aim_y > 1) aim_y--;
                if (c == 'a' && aim_x > 1) aim_x--;
                if (c == 'd' && aim_x < width - 2) aim_x++;
                if (c == '\n') {
                    double dx = aim_x;
                    double dy = aim_y;
                    v0 = sqrt(dx*dx + dy*dy) / SCALE;
                    angle = atan2(dy, dx);
                    px = 0; py = 0;
                    vx = v0 * cos(angle);
                    vy = v0 * sin(angle);
                    gameState = 1;
                }
            } else if (gameState == 2) {
                gameState = 0;
            }
            
            if (c == 'r') { gameState = 0; aim_x = 10; aim_y = 5; }
        }

        if (gameState == 1) {
            px += vx * dt;
            py += vy * dt;
            vy -= gravity * dt;
            if (py < 0) {
                final_distance = px;
                gameState = 2;
            }
        }

        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                screen[y][x] = ' ';

        for (int x = 0; x < width; x++) screen[0][x] = '_';

        if (gameState == 1) {
            int sx = (int)(px * SCALE);
            int sy = (int)(py * SCALE);
            if (sx >= 0 && sx < width && sy >= 0 && sy < height)
                screen[sy][sx] = 'O';
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

        clear_screen();
        
        if (gameState == 1) {
             printf("FPS: %.1f | H-Speed: %.2f m/s | V-Speed: %.2f m/s\n", fps, vx, vy);
             printf("\n");
             printf("\n");
        } else if (gameState == 0) {
            double potential_v0 = sqrt(aim_x*aim_x + aim_y*aim_y) / SCALE;
            double visual_x = aim_x /2;
            double potential_angle = atan2(aim_y, visual_x);
            
            double potential_vx = potential_v0 * cos(potential_angle);
            double potential_vy = potential_v0 * sin(potential_angle);
            
            printf("FPS: %.1f | H-Speed: %.2f m/s | V-Speed: %.2f m/s\n", fps, potential_vx, potential_vy);
            printf("Cursor Angle: %.1f deg\n", potential_angle * 180.0 / M_PI);
            printf("[W/S/A/D] to Aim | [Enter] to Launch | [R] to Reset | [Q] to Quit\n");
        } else {
            printf("FPS: %.1f | Landed!\n", fps);
            printf("Final distance is shown below.\n");
            printf("Press any key to aim again.\n");
        }

        for (int y = height-1; y >= 0; y--) {
            for (int x = 0; x < width; x++)
                putchar(screen[y][x]);
            putchar('\n');
        }

        usleep(16000);
    }

    for (int y = 0; y < height; y++) {
        free(screen[y]);
    }
    free(screen);

    return 0;
}
