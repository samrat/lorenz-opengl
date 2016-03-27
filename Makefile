CFLAGS =  -g -Wall --pedantic -std=c99 -Igl3w
LIBS = -lGL -lglfw -ldl -lm

lorenz: vec3.c lorenz.c gl3w/gl3w.c
	gcc $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	$(RM) lorenz
