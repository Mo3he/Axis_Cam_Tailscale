#!/usr/bin/env sh
# Resolve the Tailscale version to package.
#
# Tailscale's GitHub "latest" release sometimes lands before the static ARM
# tarballs are published, so fall back to the newest version that actually has
# an ARM package on pkgs.tailscale.com.
set -eu

GH_VERSION=$(curl -fsS https://api.github.com/repos/tailscale/tailscale/releases/latest \
    | sed -n 's/.*"tag_name": *"v\{0,1\}\([^"]*\)".*/\1/p' | head -1)

if [ -n "${GH_VERSION}" ] && \
   curl -sfI "https://pkgs.tailscale.com/stable/tailscale_${GH_VERSION}_arm.tgz" >/dev/null 2>&1; then
    printf '%s\n' "${GH_VERSION}"
    exit 0
fi

curl -fsS https://pkgs.tailscale.com/stable/ \
    | grep -o 'tailscale_[0-9.]*_arm\.tgz' \
    | sed -E 's/^tailscale_([0-9.]+)_arm\.tgz$/\1/' \
    | sort -V | tail -1
