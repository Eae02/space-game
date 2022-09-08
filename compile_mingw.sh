#!/bin/bash
rm -R obj/src/* 2> /dev/null

COMPILER="x86_64-w64-mingw32-g++"
PKG_CONFIG="pkg-config"

if [ ! -f libnoise/libnoisesrc-1.0.0.zip ]; then
	rm -R libnoise 2> /dev/null
	mkdir libnoise
	cd libnoise
	echo "downloading libnoise..."
	wget -q http://prdownloads.sourceforge.net/libnoise/libnoisesrc-1.0.0.zip
	unzip -q libnoisesrc-1.0.0.zip
	cd ..
fi
echo "compiling libnoise..."
$COMPILER -fPIC libnoise/noise/src/*.cpp libnoise/noise/src/module/*.cpp libnoise/noise/src/model/*.cpp -O2 -fno-rtti -shared -o libnoise/libnoise.dll
ln -snf $(realpath "./libnoise/noise/src") libnoise/noise/include/noise

CFLAGS="-O2 --std=c++2a -isystem ext/glm -isystem ext -DGLM_FORCE_CTOR_INIT -DGLM_ENABLE_EXPERIMENTAL
 $($PKG_CONFIG --cflags sdl2) -DSDL_MAIN_HANDLED -D_USE_MATH_DEFINES -isystem libnoise/noise/include
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
$COMPILER -Wl,-subsystem,windows $(find obj -name "*.cpp.o") -o game $($PKG_CONFIG --libs sdl2) -lopengl32 "-Llibnoise" -lnoise
