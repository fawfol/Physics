#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main() {
    float A = 0, B = 0;
    const float R1 = 1;
    const float R2 = 2;
    const float K2 = 5;

    const char luminance_chars[] = ".-~:;o=*%B#@";

    for (;;) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        int width = w.ws_col;
        int height = w.ws_row;

        float K1 = width * K2 * 3 / (8 * (R1 + R2));

        char output[width * height];
        float zbuffer[width * height];
        memset(output, ' ', sizeof(output));
        memset(zbuffer, 0, sizeof(zbuffer));

        for (float theta = 0; theta < 2 * M_PI; theta += 0.07) {
            for (float phi = 0; phi < 2 * M_PI; phi += 0.02) {
                float sinA = sin(A), cosA = cos(A);
                float sinB = sin(B), cosB = cos(B);
                float cosTheta = cos(theta), sinTheta = sin(theta);
                float cosPhi = cos(phi), sinPhi = sin(phi);

                float circleX = R2 + R1 * cosTheta;
                float circleY = R1 * sinTheta;

                float x = circleX * (cosB * cosPhi + sinA * sinB * sinPhi)
                         - circleY * cosA * sinB;
                float y = circleX * (sinB * cosPhi - sinA * cosB * sinPhi)
                         + circleY * cosA * cosB;
                float z = K2 + cosA * circleX * sinPhi + circleY * sinA;
                float ooz = 1 / z;

                int xp = (int)(width / 2 + K1 * ooz * x);
                int yp = (int)(height / 2 - K1 * ooz * y);

                float L = cosPhi * cosTheta * sinB
                        - cosA * cosTheta * sinPhi
                        - sinA * sinTheta
                        + cosB * (cosA * sinTheta - cosTheta * sinA * sinPhi);

                if (L > 0 && xp >= 0 && xp < width && yp >= 0 && yp < height) {
                    int idx = xp + yp * width;
                    if (ooz > zbuffer[idx]) {
                        zbuffer[idx] = ooz;
                        int lumIndex = (int)(L * 8);
                        if (lumIndex > 11) lumIndex = 11;
                        output[idx] = luminance_chars[lumIndex];
                    }
                }
            }
        }

        printf("\x1b[H");
        for (int k = 0; k < width * height; k++) {
            putchar(k % width ? output[k] : '\n');
        }

        A += 0.04;
        B += 0.02;
        usleep(30000);
    }

    return 0;
}
