#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "./libs/axLicense/axLicense.h"
#include "app.h"

#define TAILSCALE_VPN "\
#!/bin/sh \n\
echo \"Starting Service\" \n\
chmod 777 /usr/local/packages/tailscale_vpn/lib/tailscale \n\
chmod 777 /usr/local/packages/tailscale_vpn/lib/tailscaled \n\
/usr/local/packages/tailscale_vpn/lib/tailscaled --tun=userspace-networking --socket=/usr/local/packages/tailscale_vpn/tailscaled.sock & \n\
echo \"Service Started\" \n\
echo \"Scroll to Bottom for link\" \n\
/usr/local/packages/tailscale_vpn/lib/tailscale --socket=/usr/local/packages/tailscale_vpn/tailscaled.sock up \n\
wait \n\
"

//****************/
/* main function */
//****************/
int main() {
/* Loop main (keeps the app running. without this part the app will start and stop right after) */
    GMainLoop *main_loop;
    main_loop = g_main_loop_new(NULL, FALSE);

    //check license
    if (!axLicense_checkLicense1()){
        syslog(LOG_ERR,"License key not installed. Exiting");
        g_message("License key not installed. Exiting");
        return 0;
    }

    /* LOG app name (appname) */    
    syslog(LOG_INFO, "Starting %s", APP_NAME); 
    g_message("Starting %s",APP_NAME);
    //run script
    system(TAILSCALE_VPN);
    
    /* calling main loop */
    g_main_loop_run(main_loop); 

//exit    
 return 0; 
}
