# Axis_Cam_Tailscale

## To install tailscale on Axis cameras
1. copy the files "tailscale" and "tailscaled" for the correct proccesor to the camera in a new directory /usr/local/packages/tailscale
2. `cd /usr/local/packages/tailscale`
3. `chmod 777 tailscaled`
4. `chmod 777 tailscale`
5. copy "tailscaled.service" to /etc/systemd/system
6. `systemctl daemon-reload`
7. `systemctl enable tailscaled.service`
8. `systemctl start tailscaled.service`
9. `cd /usr/local/packages/tailscale`
10. `./tailscale up`
11. Copy URL to browser to authenticate Tailscale
