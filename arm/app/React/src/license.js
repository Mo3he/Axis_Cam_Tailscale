import React from 'react';
import ReactDOM from 'react-dom';
import axios from 'axios'
import {did,appName,appId,startACAP} from './App'

var serialNbr = "";

var enterLicenseCode = (
                        <div>                                     
                            <div className='input-group mb-3' id="licenseCodeRef">
                                <div className='input-group-prepend'><br/>
                                    <span className='input-group-text' id=''>License Code</span><br/>
                                </div>
                                <input type="text" placeholder="Enter License Code" className="form-control" id="licenseCode" />
                            </div>                                                                       
                            <br/>
                            <button type="button" id="getLicenseButton" className="btn btn-primary" onClick={glk}>
                                Get License
                            </button>                          
                        </div>
);

var freeLicenseCode = (
                        <div></div>
);

class LicensePage extends React.Component{    
    render(){            
      return(
        <div className="App">
          <header className="App-header">
          <div className="lic">
            <div className="licArea">
                  <div style={{width:"100vh",height:"100vh",borderWidth:"0px"}}>     
                    <div className="center">
                        <img src="./img/axis.svg" height="100" alt=""/><br/>                        
                        <div id="license">
                            <br/>                             
                            <p>{this.props.message}</p>
                            {/*{this.props.licStatus}*/}
                        </div>                                                            
                    </div>
                  </div>
            </div>
          </div>
          </header>
        </div>
      )
    }
  }

//request license to axis site
async function glk(){  
    //licStatus = "Retrieving license key. Please wait ...";    
    //render License Page
    ReactDOM.render(<LicensePage message="Retrieving license key. Please wait ..." />,document.getElementById('root')); 

    //var lcode = document.getElementById("licenseCode").value; 
    //console.log("license code : " + lcode);
    //payload
    //var d = "{\"licenseCode\":\""+lcode+"\",\"deviceId\":\""+serialNbr+"\"}";
    if(did==="free"){
        var d = "{\"applicationId\":\""+appId+"\",\"deviceId\":\""+serialNbr+"\",\"licenseType\":1}"
    }   
    //build http get request
    var getLicenseRequest = {
        method: "POST",
        crossDomain: true,
        url: "https://gateway.api.axis.com/info-ext/acap/aca/v2/licensekey",
        data: d,
        dataType: "json",        
        headers: {
            Authorization: "Bearer 2f742d39-1251-30bc-ba38-bc460e820596",
            'Content-Type': 'application/json'           
        },
    }
    //send request
    var getLicense = await axios(getLicenseRequest)
    //error handling
    .catch(err => {
        //console.log(err.response);
        var errorMessage = "Error! Code: " + err.response.status + " Message: " + err.response.data.originalTrace;
        console.log(errorMessage);        
        ReactDOM.render(<LicensePage message={errorMessage} licStatus={enterLicenseCode}/>,document.getElementById('root')); 
    })
    
    //debug
    console.log(getLicense.status);
    console.log(getLicense.data);
    if (getLicense.status===200){        
        //console.log("200OK!");
        ilk(getLicense.data);
    }else{              
        alert("Error receiving license key, install through Axis Device Management or the ACAP List page");
    }    
}

//upload license into camera
async function ilk(lr){      
    //render License Page
    ReactDOM.render(<LicensePage message="Installing license key. Please wait ..." licStatus={enterLicenseCode}/>,document.getElementById('root'));

    console.log("LR = "+lr.xml);
    if(lr.xml !== undefined){        
        var formData = new FormData();
        var blob = new Blob([lr.xml], {type: "application/octet-stream"});
        formData.append("lic_data",blob,"key.lic");
        var uploadKeyRequest = {
            method: "POST",
            url: "/axis-cgi/applications/license.cgi?action=uploadlicensekey&package=" + appName,
            data: formData            
        };

        var uploadKey = await axios(uploadKeyRequest);
        console.log("Response: " + uploadKey.status);
        if (uploadKey.data.includes("OK")){
            console.log("License key installed");
            //start acap
            restartAcap();            
            //check license again and redirect if Valid
            ValidateLicense();
        }else{
            if (uploadKey.data.includes("21")){
                console.log("Error: Invalid license key file.");
                alert("Error: Invalid license key file.");
            }
            if (uploadKey.data.includes("22")){
                console.log("Error: File upload failed.");
                alert("Error: File upload failed.");
            }
            if (uploadKey.data.includes("23")){
                console.log("Error: Failed to remove the license key file.");
                alert("Error: Failed to remove the license key file.");
            }
            if (uploadKey.data.includes("24")){
                console.log("Error: The application is not correctly installed");
                alert("Error: The application is not correctly installed");
            }
            if (uploadKey.data.includes("25")){
                console.log("Error: The key’s application ID does not match the installed application.");
                alert("Error: The key’s application ID does not match the installed application.");
            }
            if (uploadKey.data.includes("26")){
                console.log("Error: The license key cannot be used with this version of the application.");
                alert("Error: The license key cannot be used with this version of the application.");
            }
            if (uploadKey.data.includes("27")){
                console.log("Error: Failed to connect to Axis online service.");
                alert("Error: Failed to connect to Axis online service.");
            }
            if (uploadKey.data.includes("28")){
                console.log("Error: Failed to receive license from Axis online service.");
                alert("Error: Failed to receive license from Axis online service.");
            }
            if (uploadKey.data.includes("29")){
                console.log("Error: Bad configuration file for the application.");
                alert("Error: Bad configuration file for the application.");
            }
            if (uploadKey.data.includes("30")){
                console.log("Error: Wrong serial number.");
                alert("Error: Wrong serial number.");
            }
            if (uploadKey.data.includes("31")){
                console.log("Error: The license key has expired.");
                alert("Error: The license key has expired.");
            }
        }
    }else {
        //All kinds of errors.
        console.log("Error receiving license key, install through Axis Device Management or the ACAP List page");
        alert("Error receiving license key, install through Axis Device Management or the ACAP List page");
    }

}

//Validate license key (this is done first at startup)
export async function ValidateLicense(){
    //get serial number
    getSerialNbr();
   
    var listRequest = {
        method: "GET",
        url: "/axis-cgi/applications/list.cgi"       
    }

    var list = await axios(listRequest);
    //debug
    console.log(list.data);
    var XMLParser = require('react-xml-parser');
    var xmlDoc = new XMLParser().parseFromString(list.data);
    //debug
    console.log(xmlDoc);
    //find number of installed apps
    var installedApps = xmlDoc.getElementsByTagName("application").length;
    //debug
    console.log("Installed Apps: " + installedApps);
    //scan through them and look for our app
    var i =0;
    for (i = 0; i < installedApps; i++){
        var licenseStatus = xmlDoc.getElementsByTagName("application")[i].attributes.License;
        if ((xmlDoc.getElementsByTagName("application")[i].attributes.Name) === appName){
            if (licenseStatus === 'Missing'){                 
                console.log("Missing license.");
                if (did === 'free'){                    
                    //install the license
                    glk();
                }
                //render License Page
                ReactDOM.render(<LicensePage message="A license key needs to be installed." licStatus={enterLicenseCode}/>,document.getElementById('root'));                
            }
            if (licenseStatus === 'Valid'){
                console.log("Valid license.");
                //This is the function to call if the license is Valid. This resides in the app.js file
                startACAP();                 
                //render License Page            
                ReactDOM.render(<LicensePage message="Loading ..."/>,document.getElementById('root'));
            }
            if (licenseStatus === 'Invalid'){                
                console.log("Invalid license.");
                //render License Page            
                ReactDOM.render(<LicensePage message="Invalid license." licStatus={enterLicenseCode}/>,document.getElementById('root'));
            }
        }
    }
}

async function getSerialNbr(){   
    var serialNumberRequest = {
        method: "GET",
        url: "/axis-cgi/param.cgi?action=list&group=Properties.System.SerialNumber"
    }
    var response = await axios(serialNumberRequest);
    serialNbr = response.data.split("=")[1].replace("\n","").trim();
    console.log("serial number: " + serialNbr);
}

async function restartAcap() {
    var restartRequest = {
        method: "GET",
        url: "/axis-cgi/applications/control.cgi?action=restart&package=" + appName.toLowerCase()
    }

    var restart = await axios(restartRequest);
}


//export default glk;
