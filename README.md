## To install Tailscale on Axis cameras

You will need to enable SSH via the plainconfig 

1. Copy the files "tailscale" and "tailscaled" for the correct proccesor to the camera to /usr/local/packages/tailscale
2. `cd /usr/local/packages/tailscale`
3. `chmod 777 tailscaled`
4. `chmod 777 tailscale`
5. Copy "tailscaled.service" to /etc/systemd/system
6. `systemctl daemon-reload`
7. `systemctl enable tailscaled.service`
8. `systemctl start tailscaled.service`
9. `cd /usr/local/packages/tailscale`
10. `./tailscale up`
11. Copy URL to browser to authenticate Tailscale

Chip	Architecture
* ARTPEC-6	arm
* ARTPEC-7	arm
* ARTPEC-8	aarch64
* S2E	arm
* S2L	arm
* S3L	arm
* S5	aarch64
* S5L	aarch64
