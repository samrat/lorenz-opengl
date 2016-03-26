CFLAGS =  -g -Wall --pedantic -std=c99 -Igl3w
LIBS = -lGL -lglfw -ldl -lm

lorenz: lorenz.c vec3.c gl3w/gl3w.c
	gcc $(CFLAGS) $< gl3w/gl3w.c -o $@ $(LIBS)

clean:
	$(RM) lorenz
