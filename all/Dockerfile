# example usage:
# docker build --build-arg ARCH=armv7hf --build-arg TAILSCALE_VERSION=1.70.0 -t axis_tailscale:latest .
# docker cp $(docker create axis_tailscale):/opt/app ./build

ARG REPO=axisecp
ARG SDK=acap-native-sdk
ARG VERSION=1.3
ARG ARCH=armv7hf
ARG UBUNTU_VERSION=22.04

FROM ${REPO}/${SDK}:${VERSION}-${ARCH}-ubuntu${UBUNTU_VERSION}
WORKDIR /opt/app
COPY ./app /opt/app/

ARG ARCH=armv7hf
ARG TAILSCALE_VERSION=1.68.1

# template files
RUN sed -i "s/@@ARCH@@/${ARCH}/g" /opt/app/manifest.json \
   && sed -i "s/@@TAILSCALE_VERSION@@/${TAILSCALE_VERSION}/g" /opt/app/manifest.json

# tailscale uses "arm(64)" instead of the more specific arm arch names
RUN \
   if   [ "${ARCH}" = "armv7hf" ]; then TAILSCALE_ARCH="arm";   \
   elif [ "${ARCH}" = "aarch64" ]; then TAILSCALE_ARCH="arm64"; \
   else TAILSCALE_ARCH="${ARCH}"; fi \
   && curl -sS --fail -L -o /tmp/tailscale.tgz https://pkgs.tailscale.com/stable/tailscale_${TAILSCALE_VERSION}_${TAILSCALE_ARCH}.tgz \
   && mkdir -p /opt/app/lib \
   && tar xzf /tmp/tailscale.tgz -C /opt/app/lib --strip-components=1 --wildcards --no-anchored '*/tailscale' '*/tailscaled' \
   && rm /tmp/tailscale.tgz

RUN . /opt/axis/acapsdk/environment-setup* && acap-build ./
