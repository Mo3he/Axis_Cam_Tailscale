## To install Tailscale on Axis cameras

### Check Soc
`curl --anyauth "*" -u username:password 192.168.0.90/axis-cgi/basicdeviceinfo.cgi --data "{\"apiVersion\":\"1.0\",\"context\":\"Client defined request ID\",\"method\":\"getAllProperties\"}"`


### Soc	Architecture
* ARTPEC-6 =	arm
* ARTPEC-7 = arm
* ARTPEC-8 = aarch64
* S2E =	arm
* S2L =	arm
* S3L =	arm
* S5 = aarch64
* S5L =	aarch64

### You will need to enable SSH via the plain config then:

1. Copy the files "tailscale" and "tailscaled" for the correct Soc to the camera to /usr/local/packages/tailscale
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



