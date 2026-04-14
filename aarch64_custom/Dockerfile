ARG ARCH=aarch64
ARG VERSION=12.3.0
ARG UBUNTU_VERSION=24.04
ARG REPO=axisecp
ARG SDK=acap-native-sdk

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}

# Building the ACAP application
COPY ./app /opt/app/
WORKDIR /opt/app
RUN aarch64-linux-gnu-strip -s lib/tailscale lib/tailscaled
RUN . /opt/axis/acapsdk/environment-setup* && acap-build .