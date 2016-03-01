
#include <SPI.h>
#include <WiFi101.h>

const int MKR1000_LED = PIN_LED ;

///*** WiFi Network Config ***///
char ssid[] = "Otavio"; //  your network SSID (name)
char pass[] = "@Hue9567";    // your network password (use for WPA, or use as key for WEP)

///*** Azure IoT Hub Config ***///

char hostname[] = "YourIoTHub.azure-devices.net";    // <<trocar isso pelo id 
char authSAS[] = "YourSASToken";


String azureReceive = "/devices/MyMKR1000/messages/devicebound?api-version=2016-02-03"; 
String azureComplete = "/devices/MyMKR1000/messages/devicebound/etag?api-version=2016-02-03";         
String azureReject   = "/devices/MyMKR1000/messages/devicebound/etag?reject&api-version=2016-02-03";  
String azureAbandon  = "/devices/MyMKR1000/messages/devicebound/etag/abandon?&api-version=2016-02-03"; 

///*** Azure IoT Hub Config ***///

unsigned long lastConnectionTime = 0;            
const unsigned long pollingInterval = 5L * 1000L; // 5 sec polling delay, in milliseconds

int status = WL_IDLE_STATUS;

WiFiSSLClient client;

void setup() {

   pinMode(MKR1000_LED, OUTPUT);

  //check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
}

void loop() 
{
  String response = "";
  char c;
  ///read response if WiFi Client is available
  while (client.available()) {
    c = client.read();
    response.concat(c);
  }

  if (!response.equals(""))
  {
    //Serial.println(response);
 
    if (response.startsWith("HTTP/1.1 204")) //sem resposta
    {
      //turn off onboard LED
      digitalWrite(MKR1000_LED, LOW);
    }
    else
    {
      //turn on onboard LED
      digitalWrite(MKR1000_LED, HIGH);
      String eTag=getHeaderValue(response,"ETag");
      azureIoTCompleteMessage(eTag);
      //azureIoTRejectMessage(eTag);
      //azureIoTAbandonMessage(eTag); 
      
    }
  }

  if (millis() - lastConnectionTime > pollingInterval) {
    digitalWrite(MKR1000_LED, LOW);
    azureIoTReceiveMessage();
    lastConnectionTime = millis();

  }
}

void azureIoTReceiveMessage()
{
  httpRequest("GET", azureReceive, "","");
}
void azureIoTCompleteMessage(String eTag)
{
  String uri=azureComplete;
  uri.replace("etag",trimETag(eTag));

  httpRequest("DELETE", uri,"","");
}


void azureIoTRejectMessage(String eTag)
{
  String uri=azureReject;
  uri.replace("etag",trimETag(eTag));

  httpRequest("DELETE", uri,"","");
}
void azureIoTAbandonMessage(String eTag)
{
  String uri=azureAbandon;
  uri.replace("etag",trimETag(eTag));

  httpRequest("POST", uri,"text/plain","");
}


void httpRequest(String verb, String uri,String contentType, String content)
{
    if(verb.equals("")) return;
    if(uri.equals("")) return;
    client.stop();
    if (client.connect(hostname, 443)) {
      client.print(verb); //enviar POST, GET or DELETE
      client.print(" ");  
      client.print(uri);
      client.println(" HTTP/1.1"); 
      client.print("Host: "); 
      client.println(hostname);  //nome do header no azure
      client.print("Authorization: ");
      client.println(authSAS);  //SAS token copiado do Azure IoT device explorer
      client.println("Connection: close");

      if(verb.equals("POST"))
      {
          client.print("Content-Type: ");
          client.println(contentType);
          client.print("Content-Length: ");
          client.println(content.length());
          client.println();
          client.println(content);
      }else
      {
          client.println();
      }
    }
}

String getHeaderValue(String response, String headerName)
{
  String headerSection=getHeaderSection(response);
  String headerValue="";
  
  int idx=headerSection.indexOf(headerName);
  
  if(idx >=0)
  { 
  int skip=0;
  if(headerName.endsWith(":"))
    skip=headerName.length() + 1;
  else
    skip=headerName.length() + 2;

  int idxStart=idx+skip;  
  int idxEnd = headerSection.indexOf("\r\n", idxStart);
  headerValue=response.substring(idxStart,idxEnd);  
  }
  
  return headerValue;
}

String trimETag(String value)
{
    String retVal=value;

    if(value.startsWith("\""))
      retVal=value.substring(1);

    if(value.endsWith("\""))
      retVal=retVal.substring(0,retVal.length()-1);

    return retVal;     
}

String getHeaderSection(String response)
{
  int idxHdrEnd=response.indexOf("\r\n\r\n");

  return response.substring(0, idxHdrEnd);
}

String getResponsePayload(String response)
{
  int idxHdrEnd=response.indexOf("\r\n\r\n");

  return response.substring(idxHdrEnd + 4);
}
