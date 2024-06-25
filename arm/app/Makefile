#//Version=Native
AXIS_USABLE_LIBS = UCLIBC GLIBC
include libs/*/makefile.info

PROG     = tailscale_vpn

CFLAGS   += -Wall 

PKGS += glib-2.0 gio-2.0
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags $(PKGS))
LDLIBS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs $(PKGS))

SRCS      += main.c
OBJS      += $(SRCS:.c=.o)

all: $(PROG) $(OBJS)

$(PROG): $(OBJS)
	$(CC) $^ $(CFLAGS) $(LIBS) $(LDFLAGS) -lm $(LDLIBS) -o $@
	$(STRIP) $@

clean:
	#rm -f $(PROG) $(OBJS)

