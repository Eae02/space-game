#!/bin/bash
rm -R obj/src/* 2> /dev/null

CFLAGS="--std=c++2a -g -isystem ext/glm -isystem ext -DGLM_FORCE_CTOR_INIT -DGLM_ENABLE_EXPERIMENTAL -DDEBUG
 $(pkg-config --cflags sdl2 SDL2_image glew)
 -Wall -Wextra -Wshadow -pedantic -Wfatal-errors -Wno-unused-parameter -Wno-missing-field-initializers"

if [ ! -f pch.hpp.gch ]; then
	g++ $CFLAGS pch.hpp -o pch.hpp.gch
fi

if [ ! -d obj/ext ]; then
	CFLAGS_LIBS="-O2 -isystem ext --std=c++11"
	echo "compiling external files..."
	mkdir -p obj/ext
	g++ ext/tiny_obj_loader_impl.cpp $CFLAGS_LIBS -c -o obj/ext/tiny_obj_loader_impl.cpp.o &
	g++ ext/fastnoise/FastNoiseSIMD/FastNoiseSIMD.cpp $CFLAGS_LIBS -c -o obj/ext/FastNoiseSIMD.cpp.o &
	g++ ext/fastnoise/FastNoiseSIMD/FastNoiseSIMD_sse2.cpp -msse2 $CFLAGS_LIBS -c -o obj/ext/FastNoiseSIMD_sse2.cpp.o &
	g++ ext/fastnoise/FastNoiseSIMD/FastNoiseSIMD_sse41.cpp -msse4.1 $CFLAGS_LIBS -c -o obj/ext/FastNoiseSIMD_sse41.cpp.o &
fi

echo "compiling..."
for f in $(find src -name "*.cpp"); do
	mkdir -p obj/$(dirname $f)
	g++ $f $CFLAGS -include pch.hpp -c -o obj/$f.o &
done
wait

echo "linking..."
g++ $(find obj -name "*.cpp.o") -o game $(pkg-config --libs sdl2 SDL2_image glew) -lnoise
