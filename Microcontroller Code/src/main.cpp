#include <Arduino.h>

//Thanks so much to Stahl Now for the OSC code and examples
//https://github.com/stahlnow/OSCLib-for-ESP8266
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 4, 9);
IPAddress subnet(255, 255, 255, 0);
// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

const unsigned int localPort = 5005; // local port to listen for UDP packets (here's where we send the packets)

OSCErrorCode error;

//Information for actuator wiring
const int PeltRed = D1;
const int PeltBlack = D2;
const int Pump = D8;

const int PeltMaxHot = 755;

bool LEDBlink = false;

void setup()
{
  // Set up all the pins
  // initite the motors and set to low:
  pinMode(PeltRed, OUTPUT);
  pinMode(PeltBlack, OUTPUT);
  pinMode(Pump, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(PeltRed, LOW);
  digitalWrite(PeltBlack, LOW);
  digitalWrite(Pump, LOW);
  digitalWrite(LED_BUILTIN, HIGH);
  // done setting up all of the pins

  //set up the WiFi network
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP("Wo's thesis") ? "Ready" : "Failed!");

  //once we're running print the IP address
  Serial.println("WiFi connection");
  Serial.println("IP address: ");
  Serial.println(WiFi.softAPIP());

  //Also tell us which port we're listening on
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  digitalWrite(LED_BUILTIN, HIGH);
}

void shutdown()
{
  /*
  This method turns off all of the actuators connected
  Call this method in an emergency
  */
  digitalWrite(PeltRed, LOW);
  digitalWrite(PeltBlack, LOW);
  digitalWrite(Pump, LOW);
}

void cooler(OSCMessage &msg)
{
  /*
  This method handles setting the state of the Peltier cooler
  The method expects state of the cooler (0 = off, 1= cold, 2= hot) as the first int of a OSC message
  In case the int is out of bounds (i.e. 12) then the cooler shuts off
  */
  int peltState = msg.getInt(0);
  Serial.println(peltState);
  switch (peltState)
  {
  case 1:
    //cool the user
    //this is red pos, black neg
    digitalWrite(PeltRed, HIGH);
    digitalWrite(PeltBlack, LOW);
    break;
  case 2:
    //heat the user (but not too much to burn them)
    //this is red neg, black pos
    digitalWrite(PeltRed, LOW);
    analogWrite(PeltBlack, PeltMaxHot);
    break;
  default:
    //let's stop the cooler
    digitalWrite(PeltRed, LOW);
    digitalWrite(PeltBlack, LOW);
    break;
  }
}

void pump(OSCMessage &msg)
{
  /*
  This method handles setting the state of the pressure cuff pump 
  The method expects state of the pump (0 = off, 1= on) as the first int of a OSC message
  In case the int is out of bounds (i.e. 12) then the pump shuts off
  */
  int pumpState = msg.getInt(0);
  if (pumpState == 1)
  {
    digitalWrite(Pump, HIGH);
  }
  else
  {
    digitalWrite(Pump, LOW);
  }
}

void loop()
{
  /*
  This code is provided by 
  */
  OSCMessage msg;
  int size = Udp.parsePacket();

  if (size > 0)
  {
    while (size--)
    {
      msg.fill(Udp.read());
    }
    if (!msg.hasError())
    {
      //shutdown();
      msg.dispatch("/cooler", cooler);
      msg.dispatch("/pressure", pump);
    }
    else
    {
      error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
      shutdown();
    }
  }
}
