import React, { useCallback }  from 'react';

import './App.css';
//Bootstrap
import './css/bootstrap.min.css'
import './css/style.css'
import axios from 'axios'

/*********************************************/
//LICENSE
import {ValidateLicense} from './license';
export var did = "free";
export var appName = "tailscale_vpn";
export var appId = "414812";
/*********************************************/
//Template
export var acapName = "Tailscale VPN";
const styleLink = document.createElement("link");
styleLink.rel = "stylesheet";
styleLink.href = "./css/semantic.min.css";
document.head.appendChild(styleLink);
/*********************************************/

function App() {
  //check license
  ValidateLicense();
  
  return (    
    <div> 
    </div>
  );
}



export async function startACAP(){
  
  console.log("In startACAP");
  var httpRequest = {
    method: "GET",
    url: "/axis-cgi/applications/list.cgi"
  }
  var httpGet = await axios(httpRequest);
  //console.log(httpGet.status);
  console.log(httpGet.data);
  var XMLParser = require('react-xml-parser');
  var xmlDoc = new XMLParser().parseFromString(httpGet.data);
  console.log(xmlDoc);
  var installedApps = xmlDoc.getElementsByTagName("application").length;
  console.log("Installed Apps: " + installedApps);
  var i =0;
  for (i = 0; i < installedApps; i++){
    var appstatus = xmlDoc.getElementsByTagName("application")[i].attributes.Status;
    //console.log("app number "+i+"Status "+appstatus);
    /* look for tailscale_vpn app */
    if ((xmlDoc.getElementsByTagName("application")[i].attributes.Name) === appName){	
      /* check if tailscale_vpn is running */
      if (appstatus === 'Running'){
        console.log(acapName + " is running");  
        //load logs.html page
        window.location="/local/tailscale_vpn/logs.html";
      }else{
        console.log(acapName + " is NOT running");
        console.log(acapName + " is Starting");
        var startACAPRequest = {
          method: "GET",
          url: "/axis-cgi/applications/control.cgi?action=start&package=" + appName
        }
        var startACAP = await axios(startACAPRequest)
          .then((response) => {
            console.log(response);
            console.log("Response from tailscale_vpn acap: " + response.data);
            //load logs.html page
            window.location="/local/tailscale_vpn/logs.html";
          })    
        
      }
    }
  }  
}

export default App;
