SDLFLAGS=-lSDL2 -lSDL2_image -lSDL2_mixer
asteroids: asteroids.c hr_time.c levels.c lib.c
	gcc $^ -lm -lpthread $(SDLFLAGS) -o $@
clean:
	rm -f asteroids
