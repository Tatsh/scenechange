CC ?= gcc
MAIN_CFLAGS = -Wall -Wshadow -std=gnu99 -I.
XLDFLAGS = -Wl,-s

CFLAGS ?= -Os -g0 -ffast-math -fexcess-precision=fast -fomit-frame-pointer
LDFLAGS ?= -shared -fPIC -L.

all:
	$(CC) -o libscenechange.so $(CFLAGS) $(MAIN_CFLAGS) $(LDFLAGS) $(XLDFLAGS) scenechange.c
	$(CC) -o libtemporalsoften2.so $(CFLAGS) $(MAIN_CFLAGS) $(LDFLAGS) $(XLDFLAGS)

clean:
	rm -f *.dll *.so
