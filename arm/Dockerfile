ARG ARCH=armv7hf
ARG VERSION=12.10.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Building the ACAP application
COPY common/app /opt/app/
COPY arm/app/manifest.json /opt/app/manifest.json
WORKDIR /opt/app
ENV EXTRA_CFLAGS=-DHAS_PROXY_PORTS
RUN arm-linux-gnueabihf-strip -s lib/tailscale lib/tailscaled
RUN . /opt/axis/acapsdk/environment-setup* && acap-build -a Tailscale_VPN_run ./
