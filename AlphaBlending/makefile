all: ./Build/main.o ./Build/Blender.o
	g++ -o Blender.out ./Build/main.o ./Build/Blender.o \
	-lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system

./Build/main.o: main.cpp
	g++ -c -mavx512f main.cpp -O2 -o ./Build/main.o

./Build/Blender.o: Blender.cpp
	g++ -c -mavx512f Blender.cpp -O2 -o ./Build/Blender.o -march=native
	
rm:
	rm ./Build/main.o
	rm ./Build/Blender.o