ARG UBUNTU_VERSION=20.04
FROM axisecp/acap-sdk:3.5-armv7hf-ubuntu${UBUNTU_VERSION}

RUN apt-get update -qq && apt-get install -y --no-install-recommends upx-ucl && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

COPY ./app /opt/app/
WORKDIR /opt/app

# Rename the shell startup script (the ELF launcher will take the Tailscale_VPN name)
RUN mv Tailscale_VPN start.sh && chmod +x start.sh

# Compile a minimal ELF launcher as APPNAME.
# ACAP 3 elflibcheck requires an ELF binary and uses pidof(APPNAME) for status.
# We fork+exec start.sh so the parent "Tailscale_VPN" process stays resident,
# making pidof find it and the camera UI correctly show Running/Stopped.
RUN . /opt/axis/acapsdk/environment-setup* && \
    ${CC} -o Tailscale_VPN launcher.c && \
    ${STRIP} -s Tailscale_VPN

# Strip then UPX-compress the Tailscale binaries so they fit on flash
RUN . /opt/axis/acapsdk/environment-setup* && \
    ${STRIP} -s lib/tailscale lib/tailscaled 2>/dev/null || true && \
    upx --best lib/tailscale lib/tailscaled

# ACAP 3 firmware expects the settings page at the app root, not in html/
RUN cp html/index.html index.html

# Symlink tailscaled.log into html/ so the web UI can fetch it via HTTP.
# The log is written at runtime to localdata/ (resolved path at runtime).
RUN ln -sf ../localdata/tailscaled.log html/tailscaled.log

# Build and package
RUN . /opt/axis/acapsdk/environment-setup* && create-package.sh ./

# Patch STARTMODE: create-package.sh hardcodes "never" unless RESTRICTION_STARTMODE is set,
# but the ACAP 3 SDK does not honour our package.conf's STARTMODE=respawn without it.
# Repack the .eap with the corrected value.
RUN for eap in *.eap; do \
      tmpdir=$(mktemp -d) && tar xf "$eap" -C "$tmpdir" && \
      sed -i 's/STARTMODE="never"/STARTMODE="respawn"/' "$tmpdir/package.conf" && \
      (cd "$tmpdir" && tar czf "/opt/app/$eap" .) && \
      rm -rf "$tmpdir"; \
    done
