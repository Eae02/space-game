#!/bin/bash
rm -R obj/src/* 2> /dev/null

CFLAGS_DBG="-g -DDEBUG"
CFLAGS_REL="-O2"

COMPILER="g++"
PKG_CONFIG="pkg-config"

CFLAGS="$CFLAGS_DBG --std=c++2a -isystem ext/glm -isystem ext -DGLM_FORCE_CTOR_INIT -DGLM_ENABLE_EXPERIMENTAL
 $($PKG_CONFIG --cflags sdl2 glew)
 -Wall -Wextra -Wshadow -pedantic -Wfatal-errors -Wno-unused-parameter -Wno-missing-field-initializers"

if [ ! -f pch.hpp.gch ]; then
	$COMPILER $CFLAGS pch.hpp -o pch.hpp.gch
fi

if [ ! -d obj/ext ]; then
	CFLAGS_LIBS="-O2 -isystem ext --std=c++11"
	echo "compiling external files..."
	mkdir -p obj/ext
	$COMPILER ext/stb_image_impl.cpp $CFLAGS_LIBS -c -o obj/ext/stb_image_impl.cpp.o &
	$COMPILER ext/tiny_obj_loader_impl.cpp $CFLAGS_LIBS -c -o obj/ext/tiny_obj_loader_impl.cpp.o &
fi

echo "compiling..."
for f in $(find src -name "*.cpp" -not -path "*asteroids_gen.cpp"); do
	mkdir -p obj/$(dirname $f)
	$COMPILER $f $CFLAGS -include pch.hpp -c -o obj/$f.o &
done

$COMPILER src/graphics/asteroids_gen.cpp $CFLAGS -include pch.hpp -O2 -g0 -c -o obj/src/graphics/asteroids_gen.cpp.o &

wait

echo "linking..."
$COMPILER $(find obj -name "*.cpp.o") -o game $($PKG_CONFIG --libs sdl2 glew) -lnoise
