/*
    An example of using bootleg3d that draws some spinning cubes using SDL2.
    This can be used as a crude benchmark, as it will add more cubes to the
    scene until the framerate is around 60fps.
*/

#include <SDL2/SDL.h>
#define BOOTLEG3D_IMPLEMENTATION
#include "bootleg3d.c"

int main() {
    int width = 800;
    int height = 600;

    // Set up SDL2 stuff, including whats needed to display a buffer of pixels.
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, 0 /*SDL_RENDERER_PRESENTVSYNC*/);
    SDL_Texture * texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    uint32_t * pixel_buffer = malloc(width * height * sizeof(float));
    float * depth_buffer = malloc(width * height * sizeof(float));

    b3d_init(pixel_buffer, depth_buffer, width, height, 60);

    // For framerate counting.
    double freq = SDL_GetPerformanceFrequency();
    uint32_t next_update = 0;
    const int samples = 32;
    float average_fps[samples];
    int average_index = 0;
    int have_enough_samples = 0;

    int cube_count = 100;

    b3d_set_camera(0, 0, -4, 0, 0, 0);

    while (1) {
        uint64_t time_stamp = SDL_GetPerformanceCounter();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(0);
        }

        // Current time in milliseconds.
        float t = SDL_GetTicks() * 0.001f;

        b3d_clear();

        for (int i = 0; i < cube_count; ++i) {
            // Reset transformations back to the origin.
            b3d_reset();

            b3d_rotate_z(t);
            b3d_rotate_y(t);
            b3d_rotate_x(t);

            b3d_rotate_y(i*0.1);
            b3d_translate(1,1,fmodf(i*0.1, 100));
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

        // Display the pixel buffer on screen (using a streaming texture).
        SDL_Delay(1);
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(texture, NULL, pixel_buffer, width * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // Display the average framerate in the window title.
        if (SDL_GetTicks() > next_update && have_enough_samples) {
            char title[32];
            float fps = 0;
            for (int i = 0; i < samples; ++i) fps += average_fps[i];
            fps /= samples;
            if (fps > 61) cube_count += 10 * (fps - 60);
            snprintf(title, 32, "%d tris, %.1f fps", cube_count * 12, fps);
            SDL_SetWindowTitle(window, title);
            next_update = SDL_GetTicks() + 250;
        }
        average_fps[average_index++] = 1.0 / ((SDL_GetPerformanceCounter() - time_stamp) / freq);
        if (average_index > samples) {
            average_index = 0;
            have_enough_samples = 1;
        }
    }
}
