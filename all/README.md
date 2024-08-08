# Building

Set your preferred build parameters via the docker build command.

* `ARCH` - use the Axis architecture string you can fetch from `/axis-cgi/param.cgi?action=list&group=Properties.System.Architecture`
* `TAILSCALE_VERSION` - Specify the version string as it appears on the [Tailscale archive](https://pkgs.tailscale.com/stable/#static)

Tailscale arm arch strings are different than the axis format. The Dockerfile has some mappings for common strings.

```
docker build --build-arg ARCH=armv7hf --build-arg TAILSCALE_VERSION=1.70.0 -t axis_tailscale:latest .
```

Then copy out the build artifacts:

```
docker cp $(docker create axis_tailscale):/opt/app ./build
```
