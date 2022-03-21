# The compilation for these examples should be very simple.
# Something like in this file should work (if not exactly this).
cc image.c -o image -O3 -I..
# These examples use SDL2 (https://libsdl.org/).
cc cubes.c -o cubes -O3 -I.. `sdl2-config --cflags --libs`
cc obj.c -o obj -O3 -I.. `sdl2-config --cflags --libs`
cc fps.c -o fps -O3 -I.. `sdl2-config --cflags --libs`
