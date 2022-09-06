## To install Tailscale on Axis cameras

### Check Soc
```
curl --anyauth "*" -u username:password 192.168.0.90/axis-cgi/basicdeviceinfo.cgi --data "{\"apiVersion\":\"1.0\",\"context\":\"Client defined request ID\",\"method\":\"getAllProperties\"}"
```

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

SFTP the files "tailscale" and "tailscaled" for the correct Soc to the camera to /usr/local/packages/tailscale

 SSH to camera with root user then run
 ```
 cd /usr/local/packages/tailscale
 
 chmod 777 tailscaled
 
 chmod 777 tailscale
 ```
 SFTP "tailscaled.service" to /etc/systemd/system then run
 ```
 systemctl daemon-reload
 
 systemctl enable tailscaled.service
 
 systemctl start tailscaled.service
 
 cd /usr/local/packages/tailscale
 
 ./tailscale up
 ```
 Copy URL to browser to authenticate Tailscale



