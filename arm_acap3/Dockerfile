ARG UBUNTU_VERSION=20.04
FROM axisecp/acap-sdk:3.5-armv7hf-ubuntu${UBUNTU_VERSION}

RUN apt-get update -qq && apt-get install -y --no-install-recommends upx-ucl && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

COPY arm_acap3/app /opt/app/
WORKDIR /opt/app

# The ELF launcher takes over the Tailscale_VPN name.
RUN mv Tailscale_VPN start.sh && chmod +x start.sh

# ACAP 3 elflibcheck needs APPNAME to be ELF and uses pidof(APPNAME) for status.
RUN . /opt/axis/acapsdk/environment-setup* && \
    ${CC} -o Tailscale_VPN launcher.c && \
    ${STRIP} -s Tailscale_VPN

# UPX so the binaries fit on flash
RUN . /opt/axis/acapsdk/environment-setup* && \
    ${STRIP} -s lib/tailscale lib/tailscaled 2>/dev/null || true && \
    upx --best lib/tailscale lib/tailscaled

# ACAP 3 firmware expects the settings page at the app root, not in html/
RUN cp html/index.html index.html

# Runtime files live in localdata/; symlink them so the web UI can fetch them.
RUN ln -sf ../localdata/tailscaled.log html/tailscaled.log

RUN ln -sf ../localdata/status.json html/status.json

RUN . /opt/axis/acapsdk/environment-setup* && create-package.sh ./

# create-package.sh hardcodes STARTMODE="never" (ignoring package.conf) unless
# RESTRICTION_STARTMODE is set, so repack the .eap with respawn.
RUN for eap in *.eap; do \
      tmpdir=$(mktemp -d) && tar xf "$eap" -C "$tmpdir" && \
      sed -i 's/STARTMODE="never"/STARTMODE="respawn"/' "$tmpdir/package.conf" && \
      (cd "$tmpdir" && tar czf "/opt/app/$eap" .) && \
      rm -rf "$tmpdir"; \
    done
