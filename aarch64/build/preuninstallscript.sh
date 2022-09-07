#!/bin/sh


if [ ! -f "/etc/systemd/system/tailscaled.service" ]
then
    echo "Service doesn't exist. Trying to remove anyway"
    /usr/local/packages/Tailscale_VPN/lib/tailscale down
    systemctl stop tailscaled.service
    rm /etc/systemd/system/tailscaled.service
    
    echo "Service deleted"
else
    echo "Service exists. Removing service"
    /usr/local/packages/Tailscale_VPN/lib/tailscale down
    systemctl stop tailscaled.service
    rm /etc/systemd/system/tailscaled.service
fi