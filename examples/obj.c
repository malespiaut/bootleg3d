/*
    Loads a model from a '.obj' file and renders it using bootleg3d.
    You can pass the path to a file as the first argument, otherwise
    it will load a file (moai.obj) provided in this examples folder.

    Right now the '.obj' loading code will fail on models containing
    non-triangular faces.
*/

#include <SDL2/SDL.h>
#define BOOTLEG3D_IMPLEMENTATION
#include "bootleg3d.c"

int main(int argument_count, char ** arguments) {
    int width = 800;
    int height = 600;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture * texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

    uint32_t * pixels = malloc(width * height * sizeof(pixels[0]));
    float * depth = malloc(width * height * sizeof(depth[0]));
    b3d_init(pixels, depth, width, height, 70);

    // Barebones .obj file loader.
    char * file_name = "moai.obj";
    if (argument_count == 2) file_name = arguments[1];
    int vert_count = 0;
    float * triangles = NULL;
    {
        int vi = 0, ti = 0;
        float * vertices = NULL;
        FILE * obj_file = fopen(file_name, "r");
        if (!obj_file) {
            printf("Failed to load file '%s'.\n", file_name);
            exit(1);
        }
        char line[128];
        float x, y, z;
        while (fgets(line, 128, obj_file)) {
            if (line[0] == 'v' && sscanf(line, " v %f %f %f ", &x, &y, &z)) {
                vertices = realloc(vertices, (vi+3) * sizeof(vertices[0]));
                vertices[vi++] = x;
                vertices[vi++] = y;
                vertices[vi++] = z;
            }
        }
        rewind(obj_file);
        int a, b, c;
        while (fgets(line, 128, obj_file)) {
            if (line[0] == 'f') {
                // Erase texture and normal information.
                for (int i = 0; i < 128; ++i) {
                    if (line[i] == '/') {
                        while (line[i] != ' ' && line[i] != '\n' && line[i] != '\0') {
                            line[i++] = ' ';
                        }
                    }
                }
                if (sscanf(line, " f %d %d %d ", &a, &b, &c)) {
                    a--, b--, c--;
                    triangles = realloc(triangles, (ti+9) * sizeof(triangles[0]));
                    triangles[ti++] = vertices[(a*3)+0];
                    triangles[ti++] = vertices[(a*3)+1];
                    triangles[ti++] = vertices[(a*3)+2];
                    triangles[ti++] = vertices[(b*3)+0];
                    triangles[ti++] = vertices[(b*3)+1];
                    triangles[ti++] = vertices[(b*3)+2];
                    triangles[ti++] = vertices[(c*3)+0];
                    triangles[ti++] = vertices[(c*3)+1];
                    triangles[ti++] = vertices[(c*3)+2];
                }
            }
        }
        free(vertices);
        vert_count = ti;
    }

    printf("Loaded %d triangles from file '%s'.\n", vert_count / 9, file_name);

    // Figure out how tall the model is.
    float min_y = FLT_MAX;
    float max_y = FLT_MIN;
    for (int i = 1; i < vert_count; i += 3) {
        if (triangles[i] > max_y) max_y = triangles[i];
        if (triangles[i] < min_y) min_y = triangles[i];
    }
    // Centre the model along the y axis.
    float y_offset = (min_y + max_y) / 2;

    // Figure out how wide the model is (along x and z axis).
    float max_xz = FLT_MIN;
    for (int i = 0; i < vert_count; i += 3) {
        if (triangles[i+0] > max_xz) max_xz = fabsf(triangles[i+0]);
        if (triangles[i+2] > max_xz) max_xz = fabsf(triangles[i+2]);
    }
    // Move backwards based on how tall and wide the model is.
    float z_offset = -((max_y - min_y) + max_xz);

    b3d_set_camera(0, y_offset, z_offset, 0, 0, 0);

    while (1) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) exit(0);
        }

        float t = SDL_GetTicks() * 0.001f;
        b3d_clear();
        b3d_reset();
        b3d_rotate_y(t * 0.3);

        for (int i = 0; i < vert_count; i += 9) {
            float avg_y = (triangles[i + 1] + triangles[i + 4] + triangles[i + 7]) / 3;
            float brightness = (avg_y - min_y) / max_y;
            uint32_t c = 50 + (int)(brightness * 200) & 0xff;
            b3d_triangle(
                triangles[i + 0], triangles[i + 1], triangles[i + 2],
                triangles[i + 3], triangles[i + 4], triangles[i + 5],
                triangles[i + 6], triangles[i + 7], triangles[i + 8],
                (c << 16 | c << 8 | c)
            );
        }

        SDL_Delay(1);
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(texture, NULL, pixels, width * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}
