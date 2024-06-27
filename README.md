# The Tailscale installer ACAP

This ACAP packages the scripts and files required to install the Tailscale VPN client on Axis Cameras.

Current version 1.68.1

The "Auto Update" version has been removed since it no longer works given the files are not stored in the default location.

https://tailscale.com/changelog/

## Good news, everyone!

We have found a way to run the Tailscale ACAP without root privileges allowing the it to run on Axis OS 12.

The changes are implemented from version 1.68.1

Thank you for your continued support.

## Purpose

Adding a VPN client directly to the camera allows secure remote access to the device without requiring any other equipment or network configuration.
Tailscale achieves this in a secure, simple to setup and easy to use way.
Tailscale is based on WireGuard VPN tunneling technology.

https://tailscale.com/blog/how-tailscale-works/

## Links

https://tailscale.com/

https://github.com/tailscale/tailscale 

https://www.wireguard.com/

https://www.axis.com/

## Compatibility

The Tailscale ACAP is compatable with Axis cameras with arm and aarch64 based Soc's.

```
curl --anyauth "*" -u <username>:<password> <device ip>/axis-cgi/basicdeviceinfo.cgi --data "{\"apiVersion\":\"1.0\",\"context\":\"Client defined request ID\",\"method\":\"getAllProperties\"}"
```

where `<device ip>` is the IP address of the Axis device, `<username>` is the root username and `<password>` is the root password. Please
note that you need to enclose your password with quotes (`'`) if it contains special characters.

## Installing

The recommended way to install this ACAP is to use the pre built eap file.
Go to "Apps" on the camera and click "Add app".


## Using the Tailscale ACAP

The Tailscale ACAP will run a script on startup that sets the required permissions and starts the service and app.
Once started click "Open" to see the output of the logs for further instructions and obtain the authetication URL.

When uninstalling the ACAP, all changes and files are removed from the camera.

You will need a tailscale.com account to use the ACAP

## Updating Tailscale version

The eap files will be updated from time to time and simply installing the new version over the old will update all files.

It's also possible to build and use a locally built image as all necesary files are provided.

Replace binaries "tailscale" and "tailscaled" in lib folder with new versions.
Make sure you use the files for the correct Soc.

Latest versions can be found at 

https://pkgs.tailscale.com/stable/#static


To build, 
From main directory of the version you want (arm/aarch64)

```
docker build --tag <package name> . 
```
```
docker cp $(docker create <package name>):/opt/app ./build 
```





