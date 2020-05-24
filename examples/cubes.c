/*
    An example of using bootleg3d that draws some spinning cubes using SDL2.
    Hold space bar to render the contents of the depth buffer to the screen.
*/

#include <SDL2/SDL.h>
#define BOOTLEG3D_IMPLEMENTATION
#include "bootleg3d.c"

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define clamp(low, value, high) max(low, (min(value, high)))

int main() {
    int width = 800;
    int height = 600;

    // Set up SDL2 stuff, including whats needed to display a buffer of pixels.
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture * texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    const uint8_t * keys = SDL_GetKeyboardState(NULL);

    uint32_t * pixel_buffer = malloc(width * height * sizeof(float));
    float * depth_buffer = malloc(width * height * sizeof(float));

    b3d_init(pixel_buffer, depth_buffer, width, height, 60);

    // For framerate counting.
    double freq = SDL_GetPerformanceFrequency();
    uint32_t next_update = 0;
    const int samples = 16;
    float average_fps[samples];
    int average_index = 0;
    int have_enough_samples = 0;

    while (1) {
        uint64_t time_stamp = SDL_GetPerformanceCounter();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(0);
        }

        // Current time in milliseconds.
        float t = SDL_GetTicks() * 0.001f;

        b3d_clear();

        // Move the camera along the z axis slowly, and reset back to zero when z reaches 100.
        b3d_set_camera(0, 0, fmodf(t*2, 100), 0, 0, 0);

        for (int i = 0; i < 200; ++i) {
            // Reset transformations back to the origin.
            b3d_reset();

            b3d_rotate_z(t);
            b3d_rotate_y(t);
            b3d_rotate_x(t);

            b3d_translate(2,1,i);
            b3d_rotate_z(i + t);

            b3d_triangle(-0.5,-0.5,-0.5, -0.5, 0.5,-0.5,  0.5, 0.5,-0.5, 0xfcd0a1);
            b3d_triangle(-0.5,-0.5,-0.5,  0.5, 0.5,-0.5,  0.5,-0.5,-0.5, 0xb1b695);
            b3d_triangle( 0.5,-0.5,-0.5,  0.5, 0.5,-0.5,  0.5, 0.5, 0.5, 0x53917e);
            b3d_triangle( 0.5,-0.5,-0.5,  0.5, 0.5, 0.5,  0.5,-0.5, 0.5, 0x63535b);
            b3d_triangle( 0.5,-0.5, 0.5,  0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0x6d1a36);
            b3d_triangle( 0.5,-0.5, 0.5, -0.5, 0.5, 0.5, -0.5,-0.5, 0.5, 0xd4e09b);
            b3d_triangle(-0.5,-0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5,-0.5, 0xf6f4d2);
            b3d_triangle(-0.5,-0.5, 0.5, -0.5, 0.5,-0.5, -0.5,-0.5,-0.5, 0xcbdfbd);
            b3d_triangle(-0.5, 0.5,-0.5, -0.5, 0.5, 0.5,  0.5, 0.5, 0.5, 0xf19c79);
            b3d_triangle(-0.5, 0.5,-0.5,  0.5, 0.5, 0.5,  0.5, 0.5,-0.5, 0xa44a3f);
            b3d_triangle( 0.5,-0.5, 0.5, -0.5,-0.5, 0.5, -0.5,-0.5,-0.5, 0x5465ff);
            b3d_triangle( 0.5,-0.5, 0.5, -0.5,-0.5,-0.5,  0.5,-0.5,-0.5, 0x788bff);
        }

        if (keys[SDL_SCANCODE_SPACE]) {
            // Display depth buffer contents.
            for (int i = 0; i < width * height; ++i) {
                uint32_t c = clamp(0, depth_buffer[i] * 5, 255);
                pixel_buffer[i] = c | c << 8 | c << 16;
            }
        }

        // Display the pixel buffer on screen (using a streaming texture).
        SDL_Delay(1);
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(texture, NULL, pixel_buffer, width * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Display the average framerate in the window title.
        if (SDL_GetTicks() > next_update && have_enough_samples) {
            char title[16];
            float fps = 0;
            for (int i = 0; i < samples; ++i) fps += average_fps[i];
            fps /= samples;
            snprintf(title, 16, "%.1f fps", fps);
            SDL_SetWindowTitle(window, title);
            next_update = SDL_GetTicks() + 500;
        }
        average_fps[average_index++] = 1.0 / ((SDL_GetPerformanceCounter() - time_stamp) / freq);
        if (average_index > samples) {
            average_index = 0;
            have_enough_samples = 1;
        }
    }
}
