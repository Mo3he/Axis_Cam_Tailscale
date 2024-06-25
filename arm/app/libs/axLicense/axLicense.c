#include <glib.h>
#include <licensekey.h>
#include <stdlib.h>
#include "../../app.h"

gboolean axLicense_checkLicense1()
{
//Verify license  
  const gchar* vmajor = NULL;
  const gchar* vminor = NULL;
  int version[2];
  gboolean license_ok = FALSE;
  int license = 0;
  
  /* util-function for getting package.conf variables */
  //ax_package_get("APPID", &appid);
  //ax_package_get("APPNAME", &appname);
  ax_package_get("APPMAJORVERSION", &vmajor);
  ax_package_get("APPMINORVERSION", &vminor);

  version[0] = atoi(vmajor);
  version[1] = atoi(vminor);  

  /* licensekey_verify: Return 1 on success, 0 on failure. */
  license = licensekey_verify(APP_NAME, APP_ID, version[0], version[1]);
  
  license_ok = (license)	? 	TRUE	: FALSE;
  

  return license_ok;
}
