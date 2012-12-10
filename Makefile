
scrollwm: scrollwm.c config.h
	gcc -o scrollwm scrollwm.c -lX11

clean:
	rm scrollwm
