SDLFLAGS=-lSDL2 -lSDL2_image -lSDL2_mixer
Rocks: Rocks.c Timer.c Levels.c Log.c
	gcc $^ -lm -lpthread $(SDLFLAGS) -o $@
clean:
	rm -f Rocks
