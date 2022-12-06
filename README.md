Copyright (C) <year>, Axis Communications AB, Lund, Sweden. All Rights Reserved.
    
# The Tailscale ACAP

This is the ACAP packaging scripts and files required to install the Tailscale VPN client on Axis Cameras.

Current version 1.30.2

## Purpose

Adding a VPN client directly to the camera allows secure remote access to the device without requiring any other equipment or network configuration.
Tailscale achieves this in a secure, easy to setup and easy to use way.
Tailscale is based on WireGuard VPN tunneling technology.

https://tailscale.com/blog/how-tailscale-works/

## Links

https://tailscale.com/

https://github.com/tailscale/tailscale 

https://www.wireguard.com/

## Compatibility

The Tailscale ACAP is compatable with cameras with arm and aarch64 based Soc's.

```
curl --anyauth "*" -u <username>:<password> <device ip>/axis-cgi/basicdeviceinfo.cgi --data "{\"apiVersion\":\"1.0\",\"context\":\"Client defined request ID\",\"method\":\"getAllProperties\"}"
```

where `<device ip>` is the IP address of the Axis device, `<username>` is the root username and `<password>` is the root password. Please
note that you need to enclose your password with quotes (`'`) if it contains special characters.

## Installing

The recommended way to install this ACAP is to use the pre built eap file in the build diretory.

It's also possible to build and use a locally built image as all necesary files are provided.

Replace binaries in lib folder with new versions.

https://pkgs.tailscale.com/stable/

To build, 
From main directory of the version you want (arm/aarch64)

```
docker build --tag <package name> . 
```
```
docker cp $(docker create <package name>):/opt/app ./build 
```

## Using the Tailscale ACAP

The Tailscale ACAP will run a script on startup that sets the required permissions and starts the service and app.
Once started click open to see the output of the logs for further instructions.

When uninstalling the ACAP, all changes and files are removed from the camera.

You will need a tailscale.com account to use the ACAP

## Updating Tailscale version

The eap files will be updated from time to time alternativly simply replace the files "tailscale" and "tailscaled" in the app directory with the new versions before building.
Make sure you use the files for the correct Soc.
Latest versions can be found at https://pkgs.tailscale.com/stable/#static





