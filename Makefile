lorenz: lorenz.c vec3.c
	gcc -g -Wall --pedantic $< -o $@

clean:
	$(RM) lorenz
