ARG ARCH=aarch64
ARG VERSION=1.15.1
ARG UBUNTU_VERSION=22.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Building the ACAP application
COPY common/app /opt/app/
COPY aarch64_ROOT/app/manifest.json /opt/app/manifest.json
WORKDIR /opt/app
RUN aarch64-linux-gnu-strip -s lib/tailscale lib/tailscaled
RUN . /opt/axis/acapsdk/environment-setup* && acap-build -a Tailscale_VPN_run ./
