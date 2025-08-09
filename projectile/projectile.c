#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

#define WIDTH 80
#define HEIGHT 25
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
    char screen[HEIGHT][WIDTH];
    double aim_x = 10, aim_y = 5;
    double v0 = 0, angle = 0;
    double px = 0, py = 0, vx = 0, vy = 0;
    int launched = 0;
    double gravity = G;

    struct timespec ts_prev, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_prev);
    double fps = 0.0;

    while (1) {
        //timing
        clock_gettime(CLOCK_MONOTONIC, &ts_now);
        double dt = (ts_now.tv_sec - ts_prev.tv_sec) + 
                    (ts_now.tv_nsec - ts_prev.tv_nsec) / 1e9;
        ts_prev = ts_now;
        if (dt > 0) fps = 0.9 * fps + 0.1 * (1.0 / dt);

        //handle input
        if (kbhit()) {
            int c = getch_nonblock();
            if (c == 'q') break;
            if (!launched) {
                if (c == 'w' && aim_y < HEIGHT - 2) aim_y++;
                if (c == 's' && aim_y > 1) aim_y--;
                if (c == 'a' && aim_x > 1) aim_x--;
                if (c == 'd' && aim_x < WIDTH - 2) aim_x++;
                if (c == '\n') {
                    double dx = aim_x;
                    double dy = aim_y;
                    v0 = sqrt(dx*dx + dy*dy) / SCALE;
                    angle = atan2(dy, dx);
                    px = 0; py = 0;
                    vx = v0 * cos(angle);
                    vy = v0 * sin(angle);
                    launched = 1;
                }
            }
            if (c == 'r') { launched = 0; aim_x = 10; aim_y = 5; }
        }
        if (launched) {
            px += vx * dt;
            py += vy * dt;
            vy -= gravity * dt;
            if (py < 0) launched = 0;
        }

        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++)
                screen[y][x] = ' ';

        for (int x = 0; x < WIDTH; x++) screen[0][x] = '_';

        if (launched) {
            int sx = (int)(px / SCALE);
            int sy = (int)(py / SCALE);
            if (sx >= 0 && sx < WIDTH && sy >= 0 && sy < HEIGHT)
                screen[sy][sx] = 'O';
        } else {
            screen[(int)aim_y][(int)aim_x] = '+';
        }
        clear_screen();
        printf("FPS: %.1f  ", fps);
        if (launched) {
            printf("Speed: %.2f m/s  Angle: %.1f deg\n", 
                   sqrt(vx*vx + vy*vy),
                   atan2(vy, vx) * 180.0 / M_PI);
        } else {
            printf("\nAiming... Press Enter to launch\n");
            printf("W S D A for cursor aim\n");
        }
        for (int y = HEIGHT-1; y >= 0; y--) {
            for (int x = 0; x < WIDTH; x++)
                putchar(screen[y][x]);
            putchar('\n');
        }

        usleep(16000); //60fps
    }

    return 0;
}
