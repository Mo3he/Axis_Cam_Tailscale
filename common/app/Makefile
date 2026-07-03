PROG = Tailscale_VPN
SRCS = param_bridge.c
PKGS = axparameter glib-2.0 gio-2.0
CFLAGS += $(shell pkg-config --cflags $(PKGS))
CFLAGS += $(EXTRA_CFLAGS)
LDADD  = $(shell pkg-config --libs   $(PKGS))

all: $(PROG)
		chmod +x Tailscale_VPN_run

$(PROG): $(SRCS)
		$(CC) $(CFLAGS) -o $@ $^ $(LDADD)

clean:
		rm -f $(PROG)
