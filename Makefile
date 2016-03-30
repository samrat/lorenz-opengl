CFLAGS =  -g -Wall --pedantic -std=c11 -Igl3w
LIBS = -lGL -lglfw -ldl -lm

lorenz: vec3.c lorenz.c
	gcc $(CFLAGS) lorenz.c gl3w/gl3w.c -o $@ $(LIBS)

clean:
	$(RM) lorenz
