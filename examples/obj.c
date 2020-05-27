#include <SDL2/SDL.h>
#define BOOTLEG3D_IMPLEMENTATION
#include "bootleg3d.c"

int main() {
    int width = 800;
    int height = 600;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window * window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
    SDL_Renderer * renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture * texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetRelativeMouseMode(1);

    uint32_t * pixel_buffer = malloc(width * height * sizeof(float));
    float * depth_buffer = malloc(width * height * sizeof(float));
    b3d_init(pixel_buffer, depth_buffer, width, height, 70);

    // Barebones .obj file loader. (this wont work for most obj files,
    // you should use one of the many good existing obj loaders out there)
    char * file_name = "moai.obj";
    int vert_count = 0;
    float * triangles = NULL;
    {
        int vi = 0, ti = 0;
        float * vertices = NULL;
        FILE * obj_file = fopen(file_name, "r");
        if (!obj_file) {
            perror("Failed to load file 'moai.obj' \nerror");
            exit(1);
        }
        char line[128];
        float x, y, z;
        while (fgets(line, 128, obj_file)) {
            if (sscanf(line, " v %f %f %f ", &x, &y, &z)) {
                vertices = realloc(vertices, (vi+3) * sizeof(vertices[0]));
                vertices[vi++] = x;
                vertices[vi++] = y;
                vertices[vi++] = z;
            }
        }
        rewind(obj_file);
        int a, b, c;
        while (fgets(line, 128, obj_file)) {
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
        free(vertices);
        vert_count = ti;
    }

    printf("Loaded %d triangles from file '%s'.\n", vert_count / 9, file_name);

    b3d_set_camera(0, 1, -2, 0, 0, 0);

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
            uint32_t c = (160 + (i % 64)) & 0xff;
            b3d_triangle(
                triangles[i + 0], triangles[i + 1], triangles[i + 2],
                triangles[i + 3], triangles[i + 4], triangles[i + 5],
                triangles[i + 6], triangles[i + 7], triangles[i + 8],
                (c << 16 | c << 8 | c)
            );
        }

        SDL_Delay(1);
        SDL_RenderClear(renderer);
        SDL_UpdateTexture(texture, NULL, pixel_buffer, width * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}
