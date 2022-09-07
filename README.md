# The Tailscale ACAP

This is the ACAP packaging scripts and files required to install the Tailscale VPN client on Axis Cameras

## Compatibility

The Tailscale ACAP is compatable with cameras with arm and aarch64 based Soc's.

```
curl --anyauth "*" -u username:password 192.168.0.90/axis-cgi/basicdeviceinfo.cgi --data "{\"apiVersion\":\"1.0\",\"context\":\"Client defined request ID\",\"method\":\"getAllProperties\"}"
```

where `<device ip>` is the IP address of the Axis device and `<password>` is the root password. Please
note that you need to enclose your password with quotes (`'`) if it contains special characters.

## Installing

The recommended way to install this ACAP is to use pre built eap file.

It's also possible to build and use a locally built image as all necesary files are provided.

## Using the Docker ACAP

The Tailscale ACAP will run a script on startup that will copy a service file to systemd, set required permissions and start the service and app.
Once started click open to see the output of the logs for further instructions.

When uninstalling the ACAP all changes and files are removed from the camera.

You will need a tailscale.com account to use the ACAP





