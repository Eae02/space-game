#!/bin/bash

EXE_NAME="spacegame"

CFLAGS_DBG="-g -DDEBUG"
CFLAGS_REL="-O2"

COMPILER="g++"
PKG_CONFIG="pkg-config"

if [ $1 == "rel" ]; then
	CFLAGS=$CFLAGS_REL
	BUILD_TYPE="linux_rel"
else
	CFLAGS=$CFLAGS_DBG
	BUILD_TYPE="linux_dbg"
	EXE_NAME="$EXE_NAME""_d"
fi

OBJ_PATH="./obj/$BUILD_TYPE"
rm -Rf "./$OBJ_PATH/src" 2> /dev/null

CFLAGS="$CFLAGS --std=c++20 -isystem ext/glm -isystem ext -DGLM_FORCE_CTOR_INIT -DGLM_ENABLE_EXPERIMENTAL
 $($PKG_CONFIG --cflags sdl2)
 -Wall -Wextra -Wshadow -pedantic -Wfatal-errors -Wno-unused-parameter -Wno-missing-field-initializers
 -Wno-unused-but-set-variable -Wno-unused-variable"

if [ ! -f pch.hpp.gch ]; then
	$COMPILER $CFLAGS pch.hpp -o pch.hpp.gch
fi

if [ ! -d "$OBJ_PATH/ext" ]; then
	CFLAGS_LIBS="-O2 -isystem ext --std=c++11"
	echo "compiling external files..."
	mkdir -p "$OBJ_PATH/ext"
	$COMPILER ext/stb_image_impl.cpp $CFLAGS_LIBS -c -o "$OBJ_PATH/ext/stb_image_impl.cpp.o" &
	$COMPILER ext/tiny_obj_loader_impl.cpp $CFLAGS_LIBS -c -o "$OBJ_PATH/ext/tiny_obj_loader_impl.cpp.o" &
fi

echo "compiling..."
for f in $(find src -name "*.cpp" -not -path "*asteroids_gen.cpp"); do
	mkdir -p $OBJ_PATH/$(dirname $f)
	$COMPILER "$f" $CFLAGS -include pch.hpp -c -o "$OBJ_PATH/$f.o" &
done

$COMPILER src/graphics/asteroids_gen.cpp $CFLAGS -include pch.hpp -O2 -g0 -c -o "$OBJ_PATH/src/graphics/asteroids_gen.cpp.o" &

wait

echo "linking..."
$COMPILER -Wl,-rpath=\$ORIGIN $(find $OBJ_PATH -name "*.cpp.o") -o $EXE_NAME $($PKG_CONFIG --libs sdl2 gl) -lnoise
