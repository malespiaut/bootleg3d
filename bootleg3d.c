/*
    BOOTLEG 3D
    A super simple software renderer written in C99.
    by Benedict Henshaw, 2020

    This file is available under two licenses; see end of file.
*/

#include <stdint.h>

// Public API
void b3d_init(uint32_t * pixel_buffer, float * depth_buffer, int w, int h, float fov);
void b3d_clear();
void b3d_reset();
void b3d_translate(float x, float y, float z);
void b3d_rotate_x(float angle);
void b3d_rotate_y(float angle);
void b3d_rotate_z(float angle);
void b3d_scale(float x, float y, float z);
void b3d_set_camera(float x, float y, float z, float xr, float yr, float zr);
void b3d_set_fov(float fov_in_degrees);
void b3d_triangle(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, uint32_t c);

// You can also access these, but best to only read from them.
extern int b3d_width, b3d_height;
extern uint32_t * b3d_pixels;
extern float * b3d_depth;

#ifdef BOOTLEG3D_IMPLEMENTATION

#include <math.h>
#include <string.h>

int b3d_width, b3d_height;
uint32_t * b3d_pixels;
float * b3d_depth;

typedef struct { float x, y, z, w; } b3d_vec_t;
typedef struct { float m[4][4]; } b3d_mat_t;
typedef struct { b3d_vec_t p[3]; } b3d_triangle_t;

b3d_mat_t b3d_model, b3d_view, b3d_proj;
b3d_vec_t b3d_camera;

float b3d_vec_dot(b3d_vec_t a, b3d_vec_t b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
float b3d_vec_length(b3d_vec_t v) { return sqrtf(b3d_vec_dot(v, v)); }
b3d_vec_t b3d_vec_add(b3d_vec_t a, b3d_vec_t b) { return (b3d_vec_t){ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
b3d_vec_t b3d_vec_cross(b3d_vec_t a, b3d_vec_t b) { return (b3d_vec_t){ a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x, 1.0f }; }
b3d_vec_t b3d_vec_div(b3d_vec_t a, float b) { return (b3d_vec_t){ a.x / b, a.y / b, a.z / b, 1.0f }; }
b3d_vec_t b3d_vec_mul(b3d_vec_t a, float b) { return (b3d_vec_t){ a.x * b, a.y * b, a.z * b, a.w * b }; }
b3d_vec_t b3d_vec_norm(b3d_vec_t v) { float l = b3d_vec_length(v); return (b3d_vec_t){ v.x / l, v.y / l, v.z / l, 1.0f }; }
b3d_vec_t b3d_vec_sub(b3d_vec_t a, b3d_vec_t b) { return (b3d_vec_t){ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }

b3d_mat_t b3d_mat_ident() {
    return (b3d_mat_t){ .m = {
        [0][0] = 1.0f,
        [1][1] = 1.0f,
        [2][2] = 1.0f,
        [3][3] = 1.0f,
    }};
}

b3d_mat_t b3d_mat_rot_x(float a) {
    return (b3d_mat_t){ .m = {
        [0][0] = 1.0f,
        [1][1] = cosf(a),
        [1][2] = sinf(a),
        [2][1] = -sinf(a),
        [2][2] = cosf(a),
        [3][3] = 1.0f,
    }};
}

b3d_mat_t b3d_mat_rot_y(float a) {
    return (b3d_mat_t){ .m = {
        [0][0] = cosf(a),
        [0][2] = sinf(a),
        [2][0] = -sinf(a),
        [1][1] = 1.0f,
        [2][2] = cosf(a),
        [3][3] = 1.0f,
    }};
}

b3d_mat_t b3d_mat_rot_z(float a) {
    return (b3d_mat_t){ .m = {
        [0][0] = cosf(a),
        [0][1] = sinf(a),
        [1][0] = -sinf(a),
        [1][1] = cosf(a),
        [2][2] = 1.0f,
        [3][3] = 1.0f,
    }};
}

b3d_mat_t b3d_mat_trans(float x, float y, float z) {
    return (b3d_mat_t){ .m = {
        [0][0] = 1.0f,
        [1][1] = 1.0f,
        [2][2] = 1.0f,
        [3][3] = 1.0f,
        [3][0] = x,
        [3][1] = y,
        [3][2] = z,
    }};
}

b3d_mat_t b3d_mat_scale(float x, float y, float z) {
    return (b3d_mat_t){ .m = {
        [0][0] = x,
        [1][1] = y,
        [2][2] = z,
        [3][3] = 1.0f,
    }};
}

b3d_mat_t b3d_mat_proj(float fov, float aspect, float near, float far) {
    fov = 1.0f / tanf(fov * 0.5f / 180.0f * 3.14159f);
    return (b3d_mat_t){ .m = {
        [0][0] = aspect * fov,
        [1][1] = fov,
        [2][2] = far / (far - near),
        [3][2] = (-far * near) / (far - near),
        [2][3] = 1.0f,
        [3][3] = 0.0f,
    }};
}

b3d_mat_t b3d_mat_mul(b3d_mat_t a, b3d_mat_t b) {
    b3d_mat_t matrix;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            matrix.m[r][c] =
                a.m[r][0] * b.m[0][c] +
                a.m[r][1] * b.m[1][c] +
                a.m[r][2] * b.m[2][c] +
                a.m[r][3] * b.m[3][c];
        }
    }
    return matrix;
}

b3d_vec_t b3d_mat_mul_vec(b3d_mat_t m, b3d_vec_t v) {
    return (b3d_vec_t){
        v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0],
        v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1],
        v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2],
        v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + v.w * m.m[3][3],
    };
}

b3d_mat_t b3d_mat_qinv(b3d_mat_t m) {
    b3d_mat_t o = (b3d_mat_t){ .m = {
        [0][0] = m.m[0][0], [0][1] = m.m[1][0], [0][2] = m.m[2][0], [0][3] = 0.0f,
        [1][0] = m.m[0][1], [1][1] = m.m[1][1], [1][2] = m.m[2][1], [1][3] = 0.0f,
        [2][0] = m.m[0][2], [2][1] = m.m[1][2], [2][2] = m.m[2][2], [2][3] = 0.0f,
    }};
    o.m[3][0] = -(m.m[3][0] * o.m[0][0] + m.m[3][1] * o.m[1][0] + m.m[3][2] * o.m[2][0]);
    o.m[3][1] = -(m.m[3][0] * o.m[0][1] + m.m[3][1] * o.m[1][1] + m.m[3][2] * o.m[2][1]);
    o.m[3][2] = -(m.m[3][0] * o.m[0][2] + m.m[3][1] * o.m[1][2] + m.m[3][2] * o.m[2][2]);
    o.m[3][3] = 1.0f;
    return o;
}

b3d_mat_t b3d_mat_point_at(b3d_vec_t pos, b3d_vec_t target, b3d_vec_t up) {
    b3d_vec_t forward = b3d_vec_sub(target, pos);
    forward = b3d_vec_norm(forward);
    b3d_vec_t a = b3d_vec_mul(forward, b3d_vec_dot(up, forward));
    up = b3d_vec_sub(up, a);
    up = b3d_vec_norm(up);
    b3d_vec_t right = b3d_vec_cross(up, forward);
    return (b3d_mat_t){ .m = {
        [0][0] = right.x,   [0][1] = right.y,   [0][2] = right.z,   [0][3] = 0.0f,
        [1][0] = up.x,      [1][1] = up.y,      [1][2] = up.z,      [1][3] = 0.0f,
        [2][0] = forward.x, [2][1] = forward.y, [2][2] = forward.z, [2][3] = 0.0f,
        [3][0] = pos.x,     [3][1] = pos.y,     [3][2] = pos.z,     [3][3] = 1.0f,
    }};
}

b3d_vec_t b3d_intersect_plane(b3d_vec_t plane, b3d_vec_t norm, b3d_vec_t start, b3d_vec_t end) {
    norm = b3d_vec_norm(norm);
    float delta = -b3d_vec_dot(norm, plane);
    float ad = b3d_vec_dot(start, norm);
    float bd = b3d_vec_dot(end, norm);
    float t = (-delta - ad) / (bd - ad);
    b3d_vec_t start_to_end = b3d_vec_sub(end, start);
    b3d_vec_t segment = b3d_vec_mul(start_to_end, t);
    return b3d_vec_add(start, segment);
}

int b3d_clip_against_plane(b3d_vec_t plane, b3d_vec_t norm, b3d_triangle_t in, b3d_triangle_t out[2]) {
    norm = b3d_vec_norm(norm);
    b3d_vec_t * inside[3];
    int inside_count = 0;
    b3d_vec_t * outside[3];
    int outside_count = 0;
    float d0 = (norm.x * in.p[0].x + norm.y * in.p[0].y + norm.z * in.p[0].z - b3d_vec_dot(norm, plane));
    float d1 = (norm.x * in.p[1].x + norm.y * in.p[1].y + norm.z * in.p[1].z - b3d_vec_dot(norm, plane));
    float d2 = (norm.x * in.p[2].x + norm.y * in.p[2].y + norm.z * in.p[2].z - b3d_vec_dot(norm, plane));
    if (d0 >= 0) inside[inside_count++] = &in.p[0]; else outside[outside_count++] = &in.p[0];
    if (d1 >= 0) inside[inside_count++] = &in.p[1]; else outside[outside_count++] = &in.p[1];
    if (d2 >= 0) inside[inside_count++] = &in.p[2]; else outside[outside_count++] = &in.p[2];
    if (inside_count == 3) {
        out[0] = in;
        return 1;
    } else if (inside_count == 1 && outside_count == 2) {
        out[0].p[0] = *inside[0];
        out[0].p[1] = b3d_intersect_plane(plane, norm, *inside[0], *outside[0]);
        out[0].p[2] = b3d_intersect_plane(plane, norm, *inside[0], *outside[1]);
        return 1;
    } else if (inside_count == 2 && outside_count == 1) {
        out[0].p[0] = *inside[0];
        out[0].p[1] = *inside[1];
        out[0].p[2] = b3d_intersect_plane(plane, norm, *inside[0], *outside[0]);
        out[1].p[0] = *inside[1];
        out[1].p[1] = out[0].p[2];
        out[1].p[2] = b3d_intersect_plane(plane, norm, *inside[1], *outside[0]);
        return 2;
    }
    return 0;
}

void b3d_rasterise(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, uint32_t c) {
    ay = floorf(ay), by = floorf(by), cy = floorf(cy);
    float t = 0.0f;
    if (ay > by) t = ax, ax = bx, bx = t, t = ay, ay = by, by = t, t = az, az = bz, bz = t;
    if (ay > cy) t = ax, ax = cx, cx = t, t = ay, ay = cy, cy = t, t = az, az = cz, cz = t;
    if (by > cy) t = bx, bx = cx, cx = t, t = by, by = cy, cy = t, t = bz, bz = cz, cz = t;
    float line_count = cy - ay;
    float alpha_step = 1.0f / line_count;
    float alpha = 0.0f;
    float segment_height = by - ay;
    float beta_step = 1.0f / segment_height;
    float beta = 0.0f;
    for (float y = ay; y < by; y++) {
        float Ax = ax + (cx - ax) * alpha;
        float Az = az + (cz - az) * alpha;
        float Bx = ax + (bx - ax) * beta;
        float Bz = az + (bz - az) * beta;
        if (Ax > Bx) t = Ax, Ax = Bx, Bx = t, t = Az, Az = Bz, Bz = t;
        float steps = (Bx - Ax) < 1 ? 1 : (Bx - Ax);
        float depth_step = (Bz - Az) / steps;
        float d = Az;
        Bx = floorf(Bx);
        for (int x = Ax; x < Bx; ++x) {
            int p = x + y * b3d_width;
            if (d < b3d_depth[p]) {
                b3d_pixels[p] = c;
                b3d_depth[p] = d;
            }
            d += depth_step;
        }
        alpha += alpha_step;
        beta += beta_step;
    }
    segment_height = cy - by;
    beta_step = 1.0f / segment_height;
    beta = 0.0f;
    for (float y = by; y < cy; y++) {
        float Ax = ax + (cx - ax) * alpha;
        float Az = az + (cz - az) * alpha;
        float Bx = bx + (cx - bx) * beta;
        float Bz = bz + (cz - bz) * beta;
        if (Ax > Bx) t = Ax, Ax = Bx, Bx = t, t = Az, Az = Bz, Bz = t;
        float steps = (Bx - Ax) < 1 ? 1 : (Bx - Ax);
        float depth_step = (Bz - Az) / steps;
        float d = Az;
        Bx = floorf(Bx);
        for (int x = Ax; x < Bx; ++x) {
            int p = x + y * b3d_width;
            if (d < b3d_depth[p]) {
                b3d_pixels[p] = c;
                b3d_depth[p] = d;
            }
            d += depth_step;
        }
        alpha += alpha_step;
        beta += beta_step;
    }
}

/*
    Public API
*/

void b3d_triangle(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, uint32_t c) {
    b3d_triangle_t t = (b3d_triangle_t){{{ax,ay,az,1},{bx,by,bz,1},{cx,cy,cz,1}}};
    t.p[0] = b3d_mat_mul_vec(b3d_model, t.p[0]);
    t.p[1] = b3d_mat_mul_vec(b3d_model, t.p[1]);
    t.p[2] = b3d_mat_mul_vec(b3d_model, t.p[2]);
    b3d_vec_t line_a = b3d_vec_sub(t.p[1], t.p[0]);
    b3d_vec_t line_b = b3d_vec_sub(t.p[2], t.p[0]);
    b3d_vec_t normal = b3d_vec_cross(line_a, line_b);
    b3d_vec_t cam_ray = b3d_vec_sub(t.p[0], b3d_camera);
    if (b3d_vec_dot(normal, cam_ray) < 0.0f) {
        t.p[0] = b3d_mat_mul_vec(b3d_view, t.p[0]);
        t.p[1] = b3d_mat_mul_vec(b3d_view, t.p[1]);
        t.p[2] = b3d_mat_mul_vec(b3d_view, t.p[2]);
        b3d_triangle_t clipped[2];
        int count = b3d_clip_against_plane(
            (b3d_vec_t){ 0.0f, 0.0f, 0.1f, 1.0f },
            (b3d_vec_t){ 0.0f, 0.0f, 1.0f, 1.0f },
            t,
            clipped
        );
        b3d_triangle_t queue[16];
        int queue_count = 0;
        for (int n = 0; n < count; ++n) {
            t.p[0] = b3d_mat_mul_vec(b3d_proj, clipped[n].p[0]);
            t.p[1] = b3d_mat_mul_vec(b3d_proj, clipped[n].p[1]);
            t.p[2] = b3d_mat_mul_vec(b3d_proj, clipped[n].p[2]);
            t.p[0] = b3d_vec_div(t.p[0], t.p[0].w);
            t.p[1] = b3d_vec_div(t.p[1], t.p[1].w);
            t.p[2] = b3d_vec_div(t.p[2], t.p[2].w);
            float xs = b3d_width / 2.0f;
            float ys = b3d_height / 2.0f;
            t.p[0].x = ( t.p[0].x + 1.0f) * xs;
            t.p[0].y = (-t.p[0].y + 1.0f) * ys;
            t.p[1].x = ( t.p[1].x + 1.0f) * xs;
            t.p[1].y = (-t.p[1].y + 1.0f) * ys;
            t.p[2].x = ( t.p[2].x + 1.0f) * xs;
            t.p[2].y = (-t.p[2].y + 1.0f) * ys;
            queue[queue_count++] = t;
        }
        b3d_vec_t tp = { 0.0f, 0.5f, 0.0f, 1.0f };
        b3d_vec_t tn = { 0.0f, 1.0f, 0.0f, 1.0f };
        b3d_vec_t bp = { 0.0f, (float)b3d_height - 1, 0.0f, 1.0f };
        b3d_vec_t bn = { 0.0f, -1.0f, 0.0f, 1.0f };
        b3d_vec_t lp = { 0.5f, 0.0f, 0.0f, 1.0f };
        b3d_vec_t ln = { 1.0f, 0.0f, 0.0f, 1.0f };
        b3d_vec_t rp = { (float)b3d_width - 1, 0.0f, 0.0f, 1.0f };
        b3d_vec_t rn = { -1.0f, 0.0f, 0.0f, 1.0f };
        int triangles_to_clip = queue_count;
        for (int p = 0; p < 4; ++p) {
            int n = 0;
            while (triangles_to_clip > 0) {
                b3d_triangle_t test = queue[0];
                --queue_count;
                --triangles_to_clip;
                memmove(queue, queue + 1, sizeof(b3d_triangle_t) * queue_count);
                switch (p) {
                    case 0: n = b3d_clip_against_plane(tp, tn, test, clipped); break;
                    case 1: n = b3d_clip_against_plane(bp, bn, test, clipped); break;
                    case 2: n = b3d_clip_against_plane(lp, ln, test, clipped); break;
                    case 3: n = b3d_clip_against_plane(rp, rn, test, clipped); break;
                }
                for (int w = 0; w < n; ++w) {
                    queue[queue_count++] = clipped[w];
                }
            }
            triangles_to_clip = queue_count;
        }
        for (int i = 0; i < queue_count; ++i) {
            b3d_triangle_t * t = &queue[i];
            b3d_rasterise(
                t->p[0].x, t->p[0].y, t->p[0].z,
                t->p[1].x, t->p[1].y, t->p[1].z,
                t->p[2].x, t->p[2].y, t->p[2].z,
                c
            );
        }
    }
}

void b3d_reset() { b3d_model = b3d_mat_ident(); }
void b3d_rotate_x(float angle) { b3d_model = b3d_mat_mul(b3d_model, b3d_mat_rot_x(angle)); }
void b3d_rotate_y(float angle) { b3d_model = b3d_mat_mul(b3d_model, b3d_mat_rot_y(angle)); }
void b3d_rotate_z(float angle) { b3d_model = b3d_mat_mul(b3d_model, b3d_mat_rot_z(angle)); }
void b3d_translate(float x, float y, float z) { b3d_model = b3d_mat_mul(b3d_model, b3d_mat_trans(x, y, z)); }
void b3d_scale(float x, float y, float z) { b3d_model = b3d_mat_mul(b3d_model, b3d_mat_scale(x, y, z)); }
void b3d_set_fov(float fov_in_degrees) { b3d_proj = b3d_mat_proj(fov_in_degrees, b3d_height/(float)b3d_width, 0.01f, 1000.0f); }

void b3d_set_camera(float x, float y, float z, float xr, float yr, float zr) {
    b3d_camera = (b3d_vec_t){ x, y, z, 1 };
    b3d_vec_t up = { 0.0f, 1.0f, 0.0f, 1.0f };
    b3d_vec_t target = { 0.0f, 0.0f, 1.0f, 1.0f };
    up = b3d_mat_mul_vec(b3d_mat_rot_z(zr), up);
    b3d_mat_t rotate = b3d_mat_mul(b3d_mat_rot_y(xr), b3d_mat_rot_x(yr));
    target = b3d_vec_add(b3d_camera, b3d_mat_mul_vec(rotate, target));
    b3d_view = b3d_mat_qinv(b3d_mat_point_at(b3d_camera, target, up));
}

void b3d_init(uint32_t * pixel_buffer, float * depth_buffer, int w, int h, float fov) {
    b3d_width = w;
    b3d_height = h;
    b3d_pixels = pixel_buffer;
    b3d_depth = depth_buffer;
    b3d_clear();
    b3d_reset();
    b3d_proj = b3d_mat_proj(fov, b3d_height/(float)b3d_width, 0.01f, 1000.0f);
    b3d_set_camera(0, 0, 0, 0, 0, 0);
}

void b3d_clear() {
    memset(b3d_depth, 0x7f, b3d_width * b3d_height * sizeof(b3d_depth[0]));
    memset(b3d_pixels, 0, b3d_width * b3d_height * sizeof(b3d_pixels[0]));
}

#endif

/*
This software is available under 2 licenses, choose whichever you prefer:

ALTERNATIVE A - MIT License
Copyright (c) 2020 Benedict Henshaw
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
