# Tailscale ACAP for Axis Cameras

[![Release](https://img.shields.io/github/v/release/Mo3he/Axis_Cam_Tailscale?style=flat)](https://github.com/Mo3he/Axis_Cam_Tailscale/releases)
[![License](https://img.shields.io/github/license/Mo3he/Axis_Cam_Tailscale?style=flat)](LICENSE)
[![Downloads](https://img.shields.io/github/downloads/Mo3he/Axis_Cam_Tailscale/total?label=Downloads&color=blue&style=flat)](https://github.com/Mo3he/Axis_Cam_Tailscale/releases)
[![Build](https://github.com/Mo3he/Axis_Cam_Tailscale/actions/workflows/build.yml/badge.svg)](https://github.com/Mo3he/Axis_Cam_Tailscale/actions/workflows/build.yml)
[![Super-Linter](https://github.com/Mo3he/Axis_Cam_Tailscale/actions/workflows/super-linter.yml/badge.svg)](https://github.com/Mo3he/Axis_Cam_Tailscale/actions/workflows/super-linter.yml)
[![Sponsor](https://img.shields.io/badge/Sponsor%20My%20Work-EA4AAA?style=flat&logo=github&logoColor=white)](https://github.com/sponsors/Mo3he)
[![Buy Me A Coffee](https://img.shields.io/badge/Buy%20Me%20A%20Coffee-FFDD00?style=flat&logo=buy-me-a-coffee&logoColor=black)](https://www.buymeacoffee.com/mo3he)

This repository provides an **ACAP package** that installs the
[Tailscale VPN client](https://tailscale.com/) on Axis cameras, for secure remote
access without extra hardware or complex network configuration.

**[Visit the Homepage](https://mo3he.github.io/Axis_Cam_Tailscale/)**

> **Disclaimer:** Independent, community-developed ACAP package. Not an official
> Axis product and not affiliated with, endorsed by, or supported by Axis
> Communications AB or Tailscale Inc. Use at your own risk.

> **Tailscale Notice:** Tailscale is a product of Tailscale Inc. This package
> independently redistributes the Tailscale binaries under the
> [BSD 3-Clause License](LICENSE) and is not affiliated with, endorsed by, or
> supported by Tailscale Inc. For the official Tailscale client, visit
> [tailscale.com](https://tailscale.com).

## Table of Contents

- [Overview](#overview)
- [Compatibility](#compatibility)
- [Installation](#installation)
- [Configuration](#configuration)
- [Ports & security](#ports--security)
- [Accessing Tailnet services from the camera](#accessing-tailnet-services-from-the-camera)
- [Updating Tailscale](#updating-tailscale)
- [Build from source](#build-from-source)
- [Roadmap](#roadmap)
- [Links](#links)
- [License](#license)

## Overview

Adding a VPN client directly to the camera enables secure remote access without
additional hardware or complex network configuration, through Tailscale's
lightweight WireGuard-based tunnel.

- Secure remote access to cameras.
- Easy to install via EAP package.
- Works on **AXIS OS 10.12+** (non-root version, verified across 10.12–12.10).
- Works on **legacy AXIS OS 9.x / 10.x** via the ACAP 3 variant.
- Based on **WireGuard VPN** technology.

Tailscale ACAP runs **without root privileges** in userspace networking mode,
making it compatible with AXIS OS 10.12+. For **full kernel networking**, use the
**ROOT** version (AXIS OS 10.12–11.x only; AXIS OS 12 and later removed root
access for third-party applications). Learn more:
[How Tailscale Works](https://tailscale.com/blog/how-tailscale-works/).

## Compatibility

| Build | AXIS OS | Architecture | Notes |
|---|---|---|---|
| ACAP 4 (native SDK) | 10.12 – 13 | aarch64 | Standard, userspace networking |
| ACAP 4 (native SDK) | 10.12 – 13 | armv7hf | Standard, userspace networking |
| ACAP 4 root | 10.12 – 11.x | aarch64 | Full kernel networking (not on OS 12+) |
| ACAP 4 root | 10.12 – 11.x | armv7hf | Full kernel networking (not on OS 12+) |
| ACAP 3 (legacy SDK) | 9.x – 10.x | armv7hf | Legacy cameras |

> Most cameras use the standard **ACAP 4** build. The **root** builds add
> kernel-level networking but only run on AXIS OS 10.12–11.x (AXIS OS 12+ removed
> root access for ACAPs). Use the **ACAP 3** build only on legacy cameras that
> don't support ACAP 4 (AXIS OS 9–10).

**Verified on AXIS OS 13** (13.0.0, aarch64).

## Installation

> **Signed packages:** Release `.eap` files are signed with the Axis ACAP
> signing service and install normally on AXIS OS 12.10 and later.
>
> **Upgrading from an earlier version?** The signing vendor changed, so
> installing over a previously installed unsigned build can fail with
> **"Couldn't install: app"** (device log: *"Vendor ID in manifest does not
> match the vendor ID of the previous version"*). To upgrade: back up your app
> configuration, **uninstall** the old version, then install the signed one.

Get the **prebuilt `.eap` file** from the
[Releases page](https://github.com/Mo3he/Axis_Cam_Tailscale/releases).

1. Log into your Axis camera.
2. Go to **Apps -> Add App**.
3. Upload the `.eap` file.

Once installed:

- Start the app.
- Click **Open** to view logs and get your Tailscale authentication URL.
- On uninstall, all changes/files are removed.

> You'll need a [Tailscale account](https://tailscale.com/) to authenticate.

## Configuration

The app runs a C-based parameter bridge that reads settings from the ACAP
parameter store and launches Tailscale. View logs and connection status via the
**Open** button in the app, and authenticate using the provided URL or by
pre-entering an auth key in **Settings**. Parameter changes (ports, server URL,
auth key) are applied automatically without reinstalling the app.

All parameters are configurable via the web UI (**Open -> Settings** card) and
take effect immediately:

| Parameter | Default | Description |
|---|---|---|
| Custom Server URL | *(empty)* | Control server URL for [Headscale](https://headscale.net/) or other self-hosted servers. Leave blank to use Tailscale's official servers. |
| Auth Key | *(empty)* | Pre-authentication key (`tskey-auth-...`). Cleared automatically after first successful connection. Leave blank to authenticate via browser. |
| HTTP Proxy Port | `8080` | Port for the outbound HTTP/HTTPS proxy. |
| SOCKS5 Proxy Port | `1080` | Port for the outbound SOCKS5 proxy. |
| Accept DNS | `off` | Passes `--accept-dns=true` to `tailscale up`. Allows the tailnet to push DNS settings to the camera. Not available on `armv7hf_acap3`. |
| Accept Routes | `off` | Passes `--accept-routes=true` to `tailscale up`. Allows the camera to use subnet routes advertised by other nodes. Not available on `armv7hf_acap3`. |
| Advertise Routes (Subnet Router) | *(empty)* | Comma-separated CIDRs (e.g. `192.168.1.0/24,10.0.0.0/8`) this camera will route for the tailnet, turning it into a subnet router. Approve the routes in the Tailscale admin console after saving. Leave blank to disable. |

## Ports & security

All non-ROOT variants expose two local proxy endpoints that route outbound
traffic through the Tailscale tunnel. The ports are configurable via **Settings
-> HTTP Proxy Port / SOCKS5 Proxy Port**.

| Proxy | Default address | Routes |
|---|---|---|
| HTTP CONNECT | `http://127.0.0.1:8080` | HTTP and HTTPS traffic |
| SOCKS5 | `127.0.0.1:1080` | Any SOCKS5-aware app or service |

Set the HTTP CONNECT proxy wherever an HTTP/HTTPS proxy field is available on the
camera (System -> Network -> Global proxies; System -> MQTT -> Broker). For
SOCKS5-aware apps, set their proxy to `127.0.0.1:<port>`.

> **Security:** the proxies bind to **loopback only** (`127.0.0.1`), so they are
> not exposed on the camera's network interface, the least-exposed of the VPN
> ACAPs. The active proxy addresses are shown in the **Proxy Configuration** card
> of the web UI. If you change a port that is already in use, the app logs an
> error and exits rather than silently falling back.

## Accessing Tailnet services from the camera

There is an important asymmetry. Making the camera **reachable from** the tailnet
(browsing to it, VAPIX, SSH from another tailnet node) works on every build. The
harder direction is the camera **reaching out to** a tailnet peer, for example
mounting an SMB/CIFS share hosted on another node. How well this works depends on
the build:

| Build | Networking mode | Camera-initiated access to tailnet peers |
|---|---|---|
| Non-root (`aarch64`, `armv7hf`) and `armv7hf_acap3` | `--tun=userspace-networking` (no kernel `tailscale0`) | Only through the local **SOCKS5 / HTTP proxies**, and only for **proxy-aware** apps. Firmware system services (SMB client, NTP, etc.) are proxy-unaware and **cannot** reach a peer's `100.x` IP directly. |
| **ROOT** (`aarch64_root`, `armv7hf_root`) | Kernel networking with a real `tailscale0` interface | Peer `100.x` IPs are routable at the OS level, so firmware services **can** connect directly. Enable **Accept Routes** to also reach subnets behind other nodes. |

### Plan B: reverse-SSH tunnel

> **Requires root on the camera.** Port 445 is privileged, so binding it needs a
> root-capable build (e.g. developer certificates installed).

If you cannot use the ROOT build but still need the camera to mount a share on a
machine that is on your tailnet, make the remote share appear **local** to the
camera with a reverse SSH tunnel. Because the destination becomes `127.0.0.1`, the
proxy-unaware SMB client never has to route over the tailnet.

```bash
# Forward the camera's local port 445 back to the SMB share on this machine
ssh -R 445:localhost:445 root@<camera-tailscale-ip>
```

Then, in **System -> Storage -> Add network share**, use `127.0.0.1` as the share
host and connect.

## Updating Tailscale

- New `.eap` files are auto-built and released **weekly** (if a new Tailscale
  version is available).
- To update, simply install the new `.eap` over the existing one.

### Manual update (advanced)

Replace the binaries in `common/app/lib/` (shared by `aarch64`, `armv7hf`, and
their ROOT variants) or `arm_acap3/app/lib/` (legacy variant, kept separate):

- `tailscale`
- `tailscaled`

Download the latest versions:
[Tailscale static builds](https://pkgs.tailscale.com/stable/#static).

## Build from source

The Tailscale binaries are not stored in git, so first download them (see
[Manual update](#manual-update-advanced)) and place them in `common/app/lib/`, or
`arm_acap3/app/lib/` for the legacy variant.

All variants build from the **repository root**, pointing at the variant's own
`Dockerfile`:

```bash
docker build -f aarch64/Dockerfile --tag <package_name> .
docker cp $(docker create <package_name>):/opt/app ./build
```

(Same for the others: just swap in `arm/Dockerfile`, `aarch64_ROOT/Dockerfile`,
`arm_ROOT/Dockerfile`, or `arm_acap3/Dockerfile`.)

## Roadmap

### AXIS OS 13 Preparation

AXIS OS 13 (scheduled for September 2026) introduces several breaking changes that
affect all ACAP applications. See the full
[AXIS OS 13 breaking changes](https://www.axis.com/for-developers/news/AXIS-OS-13-breaking-changes)
announcement for details.

- [x] **Recompile for 64-bit time (Y2038)** - Done for the standard
  `aarch64`/`armv7hf` builds (now built against ACAP Native SDK 12.10.0); the
  ROOT variants intentionally stay on the older SDK since AXIS OS 12+ never
  supports root third-party apps.
- [x] **Migrate to Manifest Schema v2** - Done for `aarch64`/`armv7hf` (schema
  2.0.0, `compatibleOsVersions` declared); verified installability on OS
  10.12–12.10.
- [x] **Audit for executable stack usage** - All compiled binaries report
  `flags rw-` (no executable stack) on every architecture and variant.
- [x] **Verify web UI works over HTTPS** - Verified live; the UI only issues
  relative-path requests, so it inherits the page's protocol with no
  mixed-content risk.
- [x] **Sign the ACAP via the Axis ACAP Portal** - Done; `aarch64`/`armv7hf`
  packages are signed with the Axis ACAP signing service. The `root` and
  `acap3` variants use manifest schema v1.x and are distributed unsigned.

### General Improvements

- [x] **Accept DNS from tailnet toggle** - Opt-in setting passing
  `--accept-dns=true` to `tailscale up` (defaults off).
- [x] **Accept routes toggle** - Opt-in setting passing `--accept-routes=true`.
- [ ] **Switch to tiny-tailscale binaries** - Evaluate replacing the bundled
  `tailscale`/`tailscaled` with [tiny-tailscale](https://github.com/iamromulan/tiny-tailscale)
  builds (single binary, ~43% smaller).

## Links

- [Tailscale](https://tailscale.com/)
- [Tailscale GitHub](https://github.com/tailscale/tailscale)
- [WireGuard](https://www.wireguard.com/)
- [Axis Communications](https://www.axis.com/)

## License

The packaging code in this repository is licensed under BSD 3-Clause (see
[LICENSE](LICENSE)); this also covers the redistributed Tailscale binaries
(upstream Tailscale is BSD 3-Clause). Bundled upstream components are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
