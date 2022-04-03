all: pointline lineline collision orbit lab5

lab5: lab5.cpp
	g++ lab5.cpp timers.cpp -Wall -o lab5 -lX11 -lXext -lm

pointline: pointline.cpp
	g++ pointline.cpp libggfonts.a -Wall -o pointline -lX11 -lGLU -lGL -lm

lineline: lineline.cpp
	g++ lineline.cpp -Wall -o lineline -lX11 -lXext -lm

collision: collision.cpp
	g++ collision.cpp -Wall -o collision -lX11 -lXext -lm

orbit: orbit.cpp
	g++ orbit.cpp -Wall -o orbit -lX11 -lXext -lm

clean:
	rm -f lab5 pointline lineline collision orbit

