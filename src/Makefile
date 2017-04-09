CC ?= gcc
MAIN_CFLAGS = -Wall -Wshadow -std=gnu99 -I.

CFLAGS ?= -Os -g0 -ffast-math -fexcess-precision=fast -fomit-frame-pointer
LDFLAGS ?= -shared -fPIC -L.

all:
	$(CC) -o libscenechange.so $(CFLAGS) $(MAIN_CFLAGS) $(LDFLAGS) -Wl,-soname,libscenechange.so scenechange.c
	$(CC) -o libtemporalsoften2.so $(CFLAGS) $(MAIN_CFLAGS) $(LDFLAGS) -Wl,-soname,libtemporalsoften2.so temporalsoften.c

clean:
	rm -f *.dll *.so
