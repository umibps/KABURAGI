@ECHO OFF
@ECHO Building LuaCairo

gcc -s -shared -W -O2 -Wall -fno-pcc-struct-return -DLUA_BUILD_AS_DLL -DHAVE_W32API_H -DWIN32 -D_WINDOWS -o lcairo.dll lcairo.c lcairo.def -I. -I../../lua/src -I../../cairo/src -I../../pixman/pixman -I../../depend/include/png -I../../depend/include/zlib -L. -L../../lua/lib -L../../depend -L../../cairo -L../../pixman -lcairo -lpixman -lpng -lz -llua51 -lgdi32 -lmsimg32 -luser32 -lkernel32

@PAUSE 

