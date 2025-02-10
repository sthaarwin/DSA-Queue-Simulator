all:
	g++ -o bin/generator src/generator.c src/traffic_simulation.c  -Iinclude -Llib -lmingw32 -lSDL2main -lSDL2
	g++ -Iinclude -Llib -o bin/main.exe src/main.c src/traffic_simulation.c -lmingw32 -lSDL2main -lSDL2

