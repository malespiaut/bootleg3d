# The compilation for these examples should be very simple.
# Something like in this file (if not exactly this).
cc image.c -o image -I.. -O3
# These examples use SDL2 (https://libsdl.org/).
cc cubes.c -o cubes -I.. -lSDL2 -O3
cc obj.c -o obj -I.. -lSDL2 -O3
cc fps.c -o fps -I.. -lSDL2 -O3

./fps
