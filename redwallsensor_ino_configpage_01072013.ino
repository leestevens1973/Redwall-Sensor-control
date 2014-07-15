/*
Web Server Sensor Control
 
 A simple web server that the Control of sensors.
 
 created 10 June 2013
 by Jonathan Leach and Lee Stevens
 
 V1.0   Initial Release
 V1.1   Read and record data from HTTP header
 V1.2   Read data from EEPROM, only write if data has changed
 v1.2.1 Inital autorun code. Function is called int sensorAutoRun(), action times are currently hard coded 
 
 Added config page to change network settings and device settings, variable to split alarm time into minutes and hours, byte variable to set ip address at bootup 
 
 25/06/2013 added change IP Address now working and saves values to eeprom and reads them back at bootup, added RESET FUNCTION, Hyperlinks now change depending on IP Address of board
 27/06/2013 on and off times now save to eeprom, pinmode set by device count
 28/06/2013 added save NTP to eeprom, show free ram on settings page
 1/7/2013 added css to updated settings, moved code that checks auto run to 
 beginning of loop else would only work when client connected.
 2/7/2013 manual status checked at startup and set pin high or low rather than wait for client to connect
 
 TODO:  Function for autorun. Like adjustBSTEurope() but hours and minutes instead of months and days  Config page. Autorun times, IP, Timeserver, sensor pins etc...
 Tidy up HTML and CSS code
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>
#include <EEPROM.h>

// version number
char vernum[]= "1.3.1";

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:

byte ipoct[] = {
  192,168,0,34}; // default  or written from eeprom if its different ?
byte ipgate[] = {
  192,168,0,1}; // default  or written from eeprom if its different ?
byte ntpdefault[]={
  216,171,112,36};
byte ipeeprom[4];
byte gateeeprom[4];
byte ntpserver[4];

byte ntpserver1=EEPROM.read(35);
byte ntpserver2=EEPROM.read(36);
byte ntpserver3=EEPROM.read(37);
byte ntpserver4=EEPROM.read(38);


// network settings

byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0D, 0x6F, 0x48 };
byte gateway[] = { 
  ipgate[0], ipgate[1], ipgate[2], ipgate[3] };
byte subnet[] = {
  255, 255, 255,0};
IPAddress ip(ipoct[0],ipoct[1],ipoct[2],ipoct[3]);


// Initialize the Ethernet server library 
EthernetServer server(80);

unsigned int localPort = 8888;      // local port to listen for UDP packets

String inString = String(); //string for fetching data from address

IPAddress timeServer(ntpserver1,ntpserver2,ntpserver3,ntpserver4); // address of NTP server
//IPAddress timeServer(ntpdefault[0],ntpdefault[1],ntpdefault[2],ntpdefault[3]);
const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

// A UDP instance to let us send and receive packets over UDP 

EthernetUDP Udp;

time_t t;
// alarm Time Variables
int onTimeHr ;
int onTimeMn  ;
int offTimeHr ;
int offTimeMn ;
int offTime = 1551;
int onTime = 1552;
// strings to store alarm string
String TIME;
String Thr;
String Tmin;
// strings for links

char Main[] = "http://10.110.110.34/";
char Settings[] = "/settings";

// sensor settings
int sensorcount=5;
int sensornum=1;
int eepromconftype[10] = {
  0,0,0,0,0,0,0,0,0,0};
int sensorconftype[10] = {
  0,0,0,0,0,0,0,0,0,0};
int eepromstatus[10] = {
  0,0,0,0,0,0,0,0,0,0};
int sensorstatus[10] = {
  0,0,0,0,0,0,0,0,0,0};
int senspin=sensornum+2;


void setup() {


  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  // set pins according to devicecount
  int i = 0;
  while (sensornum<=sensorcount){
    pinMode(senspin, OUTPUT);
    Serial.print("Sensor ID - ");
    Serial.print(sensornum);
    Serial.print("- senspin ");
    Serial.println(senspin);
    sensornum++;
    senspin++;

  }

  //Let's read config data from the EEPROM

  //Read Sensor Config Type first address 0-9
  i = 0;
  Serial.print("Loaded conftype from eeprom - ");
  while ( i <= 9 ){
    eepromconftype[i] = EEPROM.read(i);   
    Serial.print(eepromconftype[i]);
    Serial.print(",");
    sensorconftype[i] = eepromconftype[i]; 
    i++;
  }
  Serial.println(".");
  //Read Sensor Status address 10-19
  i = 10;
  Serial.print("Loaded eepromstatus from eeprom - ");
  while ( i <= 19){
    eepromstatus[i -10] = EEPROM.read(i);
    Serial.print(eepromstatus[i -10]);
    Serial.print(",");
    sensorstatus[i -10] = eepromstatus[i-10];  
    i++;
  }
  Serial.println(".");
  // Set IP from eeprom address 20-24  - comment and re-upload if it screws up
  //  i = 20;
  //  while (i<=23){     
  //    ipoct[i-20]=EEPROM.read(i);
  //    ipeeprom[i-20]=ipoct[i-20];
  //    i++;
  //  }
  //  // Set IP from eeprom address 25-29  - comment and re-upload if it screws up
  //  i = 25;
  //  while (i<=28){    
  //    ipgate[i-25]=EEPROM.read(i);
  //    gateeeprom[i-25]=ipgate[i-25];
  //    i++;
  //  }

  // set alarm off times from eeprom 31-32
  offTimeHr = EEPROM.read(31);
  offTimeMn = EEPROM.read(32);
  offTime = offTimeHr*100 + offTimeMn;
  Serial.print("Off time -");
  Serial.println(offTime);
  // set alarm on times from eeprom 33-34
  onTimeHr = EEPROM.read(33);
  onTimeMn = EEPROM.read(34);
  onTime = onTimeHr*100 + onTimeMn;
  Serial.print("On time -");
  Serial.println(onTime);

  // load NTP server from eeprom 35-38

  ntpserver[0]=EEPROM.read(35);
  ntpserver[1]=EEPROM.read(36);
  ntpserver[2]=EEPROM.read(37);
  ntpserver[3]=EEPROM.read(38);

  // Load network parameters from eeprom

  IPAddress ip(ipoct[0],ipoct[1],ipoct[2],ipoct[3]);
  byte gateway[] = {
    ipgate[0], ipgate[1], ipgate[2], ipgate[3]        };


  // start ethernet

  Ethernet.begin(mac, ip, subnet , gateway);
  server.begin();
  Serial.print(F("IP "));
  Serial.println(Ethernet.localIP());
  Serial.print(F("Gateway "));
  Serial.println(Ethernet.gatewayIP());

  // Send UDP packet and Get time
  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  setSyncInterval(3600);

}


void loop() {

  sensornum=1; // reset sensor count    
  senspin=sensornum+2;

  //          
  while (sensornum<=sensorcount){
    if (sensorconftype[sensornum -1] == 1){ // It's running manual
       if (sensorstatus[sensornum -1] == 0)
      { 
       digitalWrite(senspin,LOW);
      }
      if (sensorstatus[sensornum -1] == 1)
      {
        digitalWrite(senspin,HIGH);
      }
      
    }
    

    if (sensorconftype[sensornum -1] == 0){ // It's running auto
      if (sensorAutoRun() == 1 ){ //Should be off
        sensorstatus[sensornum -1] = 0;
        digitalWrite(senspin,LOW);
      }
      if (sensorAutoRun() == 0 ){ //should be on
        sensorstatus[sensornum -1] = 1;
        digitalWrite(senspin,HIGH);
      }
    }
    sensornum++;
    senspin++;
  }

  // Set Date & Time
  time_t t = now(); // time in secs since 1970
  sensornum=1; // reset sensor count

  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    inString = "";

    Serial.println(F("new client"));
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (inString.length() < 125) {
          //inString.concat(c);
          inString += c;

        }

        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {

          // send a standard http response header
          // Read HTTP Header and Check Sensor Type Settings

          while (sensornum<=sensorcount){
            //Serial.println(sensornum);
            String SensorString = "S";
            //Serial.println(SensorString);
            //SensorString.concat(String(sensornum));
            SensorString += sensornum;
            //Serial.println(SensorString);

            if (inString.indexOf(SensorString) > 1 ) {
              //Serial.println(inString.substring(inString.indexOf(SensorString)+3,inString.indexOf(SensorString)+4));
              if (inString.substring(inString.indexOf(SensorString)+3,inString.indexOf(SensorString)+4) == "M") {
                sensorconftype[sensornum -1] = 1;
                // If value has changed from the value stored in eeprom, update it
                if (sensorconftype[sensornum -1] != eepromconftype[sensornum -1]){
                  //Serial.print("I should write this to eeprom!");
                  eepromconftype[sensornum -1] = sensorconftype[sensornum -1]; 
                  EEPROM.write((sensornum -1), 1);
                }
                //Serial.println("BINGO M");
              }
              else if (inString.substring(inString.indexOf(SensorString)+3,inString.indexOf(SensorString)+4) == "A"){
                sensorconftype[sensornum -1] = 0;
                // If value has changed from the value stored in eeprom, update it
                if (sensorconftype[sensornum -1] != eepromconftype[sensornum -1]){
                  //Serial.print("I should write this to eeprom!");
                  eepromconftype[sensornum -1] = sensorconftype[sensornum -1];
                  EEPROM.write((sensornum -1), 0);
                }
                //Serial.println("BINGO A");
              }
            }

            sensornum++;
          }

          sensornum=1; // reset sensor count

          // Read HTTP Header and Check Sensor Control Settings
          while (sensornum<=sensorcount){
            //Serial.println(sensornum);
            String SensorString = "C";
            //Serial.println(SensorString);
            //SensorString.concat(String(sensornum));
            SensorString += sensornum;
            //Serial.println(SensorString);

            if (inString.indexOf(SensorString) > 1 ) {
              //Serial.println(inString.substring(inString.indexOf(SensorString)+3,inString.indexOf(SensorString)+4));
              if (inString.substring(inString.indexOf(SensorString)+3,inString.indexOf(SensorString)+4) == "E") {
                sensorstatus[sensornum -1] = 1;
                // If value has changed from the value stored in eeprom, update it
                if (sensorstatus[sensornum -1] != eepromstatus[sensornum -1]){
                  //Serial.print("I should write this to eeprom!");
                  eepromstatus[sensornum -1] = sensorstatus[sensornum -1];
                  EEPROM.write((sensornum +9), 1);
                }
                //Serial.println("BINGO Enable");
              }
              else if (inString.substring(inString.indexOf(SensorString)+3,inString.indexOf(SensorString)+4) == "D"){
                sensorstatus[sensornum -1] = 0;
                // If value has changed from the value stored in eeprom, update it
                if (sensorstatus[sensornum -1] != eepromstatus[sensornum -1]){
                  //Serial.print("I should write this to eeprom!");
                  eepromstatus[sensornum -1] = sensorstatus[sensornum -1];
                  EEPROM.write((sensornum +9), 0);
                }
                // Serial.println("BINGO Disable");
              }
            }

            sensornum++;
          }

          sensornum=1; // reset sensor count


          // Write Header from flash

          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html"));
          client.println(F("Connection: close"));  // the connection will be closed after completion of the response
          client.println();
          client.println(F("<!DOCTYPE HTML>"));
          client.println(F("<html>"));
          client.println(F("<head>"));
          client.println(F("<link rel='stylesheet' type='text/css' href='http://192.168.0.101:81/redwall.css' 'media='screen' />"));
          //          client.println(F("<link rel='stylesheet' type='text/css' href='http://10.150.150.15/redwall.css' 'media='screen' />"));
          client.println(F("</head>"));
          client.println(F("<h1 class='normal'>Redwall Sensor Configuration v1.31</h1>"));

          // have we asked for a reboot?

          if (inString.indexOf("reboot") > 1)
          {
            client.println("rebooting");
            Serial.println("REBOOTING");            
            delay(5000);
            void(* resetFunc) (void) = 0; //declare reset function @ address 0
            resetFunc();  //call reset
          }

          // are there any settings in header

          if (inString.indexOf("IP") > 1 ) 
          {          

            // store network settings to strings

            // Strip out IP address octlets
            int indexnum = inString.indexOf("=");
            int delimPos = inString.indexOf(".");
            int secdelimPos = inString.indexOf(".",delimPos+1);
            int thrdelimPos = inString.indexOf(".",secdelimPos+1);
            int endpos = inString.indexOf("&");


            String oct1 = (inString.substring(indexnum+1,delimPos));
            String oct2 = (inString.substring(delimPos+1,secdelimPos));
            String oct3 = (inString.substring(secdelimPos+1,thrdelimPos));
            String oct4 = (inString.substring(thrdelimPos+1,endpos));

            // change to byte to set ip

            ipoct[0] = oct1.toInt();
            ipoct[1] = oct2.toInt();
            ipoct[2] = oct3.toInt();
            ipoct[3] = oct4.toInt();

            //Strip out Gateway
            delimPos = thrdelimPos+1;
            indexnum=(inString.indexOf("=",indexnum+1));
            delimPos = (inString.indexOf(".",thrdelimPos+1));
            secdelimPos = inString.indexOf(".",delimPos+1);
            thrdelimPos = inString.indexOf(".",secdelimPos+1);
            endpos = inString.indexOf("&",endpos+1);

            String gate1 = (inString.substring(indexnum+1,delimPos));
            String gate2 = (inString.substring(delimPos+1,secdelimPos));
            String gate3 = (inString.substring(secdelimPos+1,thrdelimPos));
            String gate4 = (inString.substring(thrdelimPos+1,endpos));

            ipgate[0] = gate1.toInt();
            ipgate[1] = gate2.toInt();
            ipgate[2] = gate3.toInt();
            ipgate[3] = gate4.toInt();

            // NTP Server settings from string
            indexnum=(inString.indexOf("NTP=")+4);
            endpos=inString.indexOf("&",indexnum);            
            String NTPstring=inString.substring(indexnum,endpos);
            delimPos=NTPstring.indexOf(".");

            indexnum=0;            
            String ntp1=(NTPstring.substring(indexnum,delimPos));
            ntpserver[0]=ntp1.toInt();

            indexnum=delimPos+1;
            delimPos=NTPstring.indexOf(".",delimPos+1);           
            String ntp2=(NTPstring.substring(indexnum,delimPos));
            ntpserver[1]=ntp2.toInt();

            indexnum=delimPos+1;
            delimPos=NTPstring.indexOf(".",delimPos+1);
            String ntp3=(NTPstring.substring(indexnum,delimPos));
            ntpserver[2]=ntp3.toInt();

            indexnum=delimPos+1;
            delimPos=NTPstring.indexOf(".",delimPos+1);
            String ntp4=(NTPstring.substring(indexnum,delimPos));
            ntpserver[3]=ntp4.toInt();
            int i=0;
            while (i<3){
              Serial.print("New NTP Server ");
              ( Serial.print(ntpserver[i]));
              i++;
            }

            Serial.print("NTP Server =");
            Serial.println(NTPstring);           




            // check network settings are valid - must be between 0-255

            // check ip address against EEPROM and write if different

            i=0;
            int x = 20; // start of ip eeprom address
            while (i<=3){
              if (ipeeprom[i]!=ipoct[i])
              { 
                // change IP for writing to eeprom
                ipeeprom[i]=ipoct[i];
                // write settings to eeprom  
                Serial.print(F("writing to eeprom  "));
                Serial.println(ipeeprom[i]);
                Serial.print(F("address"));
                Serial.println(x);
                EEPROM.write(x,ipeeprom[i]); // write to eeprom
                x++;
                i++;    
              }
              else if (ipeeprom[i]==ipoct[i])
              {
                i++;
                x++;
              }      
            } 
            // now do the same for the gateway adress 25 -29

            i=0;
            x = 25; // start of ip eeprom address
            while (i<=3){
              if (gateeeprom[i]!=ipgate[i])
              { 
                // change IP for writing to eeprom
                gateeeprom[i]=ipgate[i];
                // write settings to eeprom  
                Serial.print(F("writing to eeprom  "));
                Serial.println(gateeeprom[i]);
                Serial.print(F("address"));
                Serial.println(x);
                EEPROM.write(x,gateeeprom[i]); // write to eeprom
                x++;
                i++;    
              }
              else if (gateeeprom[i]==ipgate[i]){
                i++;
                x++;
              }      
            } 


            // No need to change subnet mask on to device settings
            // set number of devices

            indexnum = inString.indexOf("nt=");
            String devnum = (inString.substring(indexnum+3,indexnum+4));
            sensorcount = devnum.toInt();

            // save devices to eeprom address 30

            indexnum=0;

            // set on time
            indexnum = inString.indexOf("ON=");            
            TIME = inString.substring(indexnum+3,indexnum+7);
            onTime = TIME.toInt();            
            // get hour
            Thr = TIME.substring(0,2);
            onTimeHr = Thr.toInt();
            // get min
            Tmin = TIME.substring(2,4);
            onTimeMn = Tmin.toInt();  

            // set off time
            indexnum = inString.indexOf("ff=");
            TIME = inString.substring(indexnum+3,indexnum+7);
            offTime = TIME.toInt();                   
            // get hour
            Thr = TIME.substring(0,2);
            offTimeHr = Thr.toInt();
            // get min 
            Tmin = TIME.substring(2,4);
            offTimeMn = Tmin.toInt();

            // save offtime settings to eeprom 31 - 32

            byte eepromHr = byte(offTimeHr);
            EEPROM.write(31,eepromHr);
            byte eepromMn = byte(offTimeMn);           
            EEPROM.write(32,eepromMn);
            offTime = offTimeHr*100 + offTimeMn;

            // save on times to eeprom 33-34

            eepromHr = byte(onTimeHr);
            EEPROM.write(33,eepromHr);
            eepromMn = byte(onTimeMn);           
            EEPROM.write(34,eepromMn);
            onTime = onTimeHr*100 + onTimeMn;

            // save Time server settings to eeprom 35 - 39
            byte NTPeeprom;

            NTPeeprom = byte(ntpserver[0]);
            EEPROM.write(35,NTPeeprom);
            Serial.print(NTPeeprom);

            NTPeeprom = byte(ntpserver[1]);
            EEPROM.write(36,NTPeeprom);
            Serial.print(NTPeeprom);

            NTPeeprom = byte(ntpserver[2]);
            EEPROM.write(37,NTPeeprom);
            Serial.print(NTPeeprom);

            NTPeeprom = byte(ntpserver[3]);
            EEPROM.write(38,NTPeeprom);
            Serial.print(NTPeeprom);


            // print out changed settings

            client.println(F("<table summary='Redwall Sensors' cellspacing='1'>"));
            client.println(F("<tr>"));

            // Start Table
            client.println(F("<td>"));
            client.print(F("<table summary='Redwall Sensor "));
            client.print(sensornum);
            client.print(F("' class='sofT' cellspacing='0'>"));

            // Table Header
            client.println(F("<tr>"));
            client.print(F("<td colspan='2' class='helpHed'>Updated Settings"));
            client.println(F("</td>"));

            // cell 
            client.println(F("<tr>"));
            client.println(F("<td>IP Address</td>"));
            client.println(F("<td>"));
            client.print(oct1);
            client.print(F("."));
            client.print(oct2);
            client.print(F("."));
            client.print(oct3);
            client.print(F("."));
            client.print(oct4);
            client.print(F("</td>"));

            // cell

            client.println(F("<tr>"));
            client.println(F("<td>Gateway</td>"));
            client.println(F("<td>"));
            client.print(gate1);
            client.print(F("."));
            client.print(gate2);
            client.print(F("."));
            client.print(gate3);
            client.print(F("."));
            client.print(gate4);
            client.print(F("</td>"));

            // cell

            client.println(F("<tr>"));
            client.println(F("<td>NTP Server</td>"));
            client.println(F("<td>"));
            client.println(timeServer);
            client.print(F("</td>"));

            //cell
            client.println(F("<tr>"));
            client.println(F("<td>New On Time</td>"));
            client.println(F("<td>"));
            if (onTimeHr<10)
            {
              client.print("0");
            }
            client.print(onTimeHr);
            client.print(F(":"));
            if (onTimeMn<10)
            {
              client.print("0");
            }
            client.println(onTimeMn);
            client.print(F("</td>"));

            //cell
            client.println(F("<tr>"));
            client.println(F("<td>New Off Time</td>"));
            client.println(F("<td>"));
            if (offTimeHr<10)
            {
              client.print("0");
            }
            client.print(offTimeHr);
            client.print(F(":"));
            if (offTimeMn<10)
            {
              client.print("0");
            }
            client.println(offTimeMn);
            client.print(F("</td>"));

            // cell

            client.println(F("<tr>"));
            client.println(F("<td>Devices</td>"));
            client.println(F("<td>"));
            client.println(sensorcount);
            client.print(F("</td>"));

            client.println("</table>");

            client.println(F("</br>"));
            client.print(F("<a href= http://"));
            client.print(Ethernet.localIP());
            client.print(F(">main</a><br />"));
            client.print(F("<a href= http://"));
            client.print(Ethernet.localIP());             
            client.print(F("/settings >settings</a><br />"));
            client.print(F("<a href= http://"));
            client.print(Ethernet.localIP());
            client.print(F("/reboot >reboot</a><br />"));           

            break;

          }
          // which page do we want?

          if (inString.indexOf("settings") > 1 ) 
          {

            // print current settings
            delay(100);
            client.println(F("<form name='settings' action='post'>"));
            client.println(F("<table summary='Redwall Sensors' cellspacing='1'>"));
            client.println(F("<tr>"));

            // Start Table
            client.println(F("<td>"));
            client.print(F("<table summary='Redwall Sensor "));
            client.print(sensornum);
            client.print(F("' class='sofT' cellspacing='0'>"));

            // Table Header
            client.println(F("<tr>"));
            client.print(F("<td colspan='2' class='helpHed'>LAN Settings"));
            client.println(F("</td>"));         

            // Table Cell
            client.println(F("<tr>"));
            client.println(F("<td>IP Address</td>"));
            client.println(F("<td>"));
            client.print(F("<input class='sofT' size=12 value = '"));
            client.print(Ethernet.localIP());
            client.println(F("'name='IP'>"));
            client.print(F("</td>"));
            client.println(F("<tr>"));
            client.println(F("<td>Gateway</td>"));
            client.println(F("<td>"));
            client.print(F("<input type='text' size=12 value = '"));
            client.print(Ethernet.gatewayIP());
            client.println(F("'name='GW'>"));
            client.print(F("</td>"));
            client.println(F("<tr>"));
            client.println(F("<td>Subnet Mask</td>"));
            client.println(F("<td>"));
            client.print(F("<input type='text' size=12 value = '"));
            client.print(Ethernet.subnetMask());
            client.println(F("'name='mask'>"));
            client.print(F("</td>"));
            client.println(F("<tr>"));
            client.println(F("<tr>"));
            client.println(F("<td>NTP</td>"));
            client.println(F("<td>"));
            client.print(F("<input type='text' size=12 value = '"));
            client.print(timeServer);
            client.println(F("'name='NTP'>"));
            client.print(F("</td>"));
            client.println(F("<tr>"));

            // End Table
            client.println(F("</table>"));

            client.println(F("<td valign='top'>"));
            client.print(F("<table summary='Redwall Sensor "));
            client.print(sensornum);
            client.print(F("' class='sofT' cellspacing='0'>"));

            // Table Header
            client.println(F("<tr>"));
            client.print(F("<td colspan='2'  class='helpHed'>Device Settings"));
            client.println(F("</td>"));         

            // Table Cell
            client.println(F("<tr>"));
            client.println(F("<td>devices</td>"));
            client.println(F("<td>"));
            client.print(F("<input class='sofT' size=1 value = '"));           
            client.print(sensorcount);
            client.println(F("'name='count'>"));
            client.print(F("</td>"));
            client.println(F("<tr>"));
            client.println(F("<td>Off Time</td>"));
            client.println(F("<td>"));
            client.print(F("<input class='sofT' size=4 value = '"));
            if (offTime < 1000)
            {
              client.print(F("0"));
            } 
            client.println(offTime);
            client.println(F("'name='off'>"));
            client.println(F("</td>"));     

            client.println(F("<tr>"));
            client.println(F("<td>On Time</td>"));
            client.println(F("<td>"));
            client.print(F("<input class='sofT' size=4 value = '"));
            if (onTime < 1000)
            {
              client.print(F("0"));
            } 
            client.println(onTime);
            client.println(F("'name='ON'>"));
            client.print(F("</td>"));

            client.print(F("</td>"));
            client.println(F("<tr>"));
            client.println(F("<td>Free Ram"));  
            client.println(F("</br>"));
            client.println(F("Version Number"));
            client.println(F("<td>"));    
            client.println(freeRam());
            client.println(F(" kb"));
            client.println(F("</br>"));
            client.println(vernum);
            

            
            // print out up time
//            long UpTime=millis();
//            client.println(F("</br>"));
//            client.print(UpTime / 86400000);
//            client.print(" Days ");
//            client.print((UpTime % 86400000) / 3600000);
//            client.print(" Hrs ");
//            client.println(((UpTime % 86400000) % 3600000) / 60000);
//            client.print(" Mins ");

            client.print(F("</td>"));
            client.print(F("</td>"));

            // End Table
            client.println(F("</table>"));
            client.print(F("</tr><tr><td colspan='"));
            client.print(sensorcount);
            client.print(F("' class='helpHed'>"));
            client.print(F("Current Date and Time "));
            if (day(t) < 10) {
              client.print(F("0"));
            }

            client.print(day(t));
            client.print(F("-")); 
            if (month(t) < 10) {
              client.print(F("0"));
            }
            client.print(month(t));
            client.print(F("-"));
            client.println(year(t));

            if (hour(t) < 10) {
              client.print(F("0"));
            }

            client.print(hour(t) + adjustBSTEurope()); 
            client.print(F(":"));
            if (minute(t) < 10) {
              client.print(F("0"));
            } 
            client.print(minute(t));
            client.print(F(":"));
            if (second(t) < 10) {
              client.print(F("0"));
            }
            client.println(second(t)); 
            client.println(F("</td>"));
            client.println(F("</tr>"));
            client.println(F("</table>"));
            client.println(F("<br>"));
            client.println(F("<input type='submit' value='Submit'>"));           
            client.println(F("</form>"));
            client.print(F("</br>"));
            client.print(F("<a href=main"));
            client.println(F(">Main</a><br />"));
            client.println(F("</html>"));           
            delay(10);
            break;
          }


          else  {
            // Let's Draw some Tables        
            client.println(F("<form name='redwall' action='post'>"));
            client.println(F("<table summary='Redwall Sensors' cellspacing='1'>"));
            client.println(F("<tr>"));

            while (sensornum <= sensorcount){
              int senspin=sensornum+2;

              // Start Table
              client.println(F("<td>"));
              client.print(F("<table summary='Redwall Sensor "));
              client.print(sensornum);
              client.print(F("' class='sofT' cellspacing='0'>"));

              // Sensor Header
              client.println(F("<tr>"));
              client.print(F("<td colspan='2' class='helpHed'>Redwall Sensor "));
              client.print(sensornum);
              client.println(F("</td></tr>"));

              // Sensor ID
              client.println(F("<tr>"));
              client.println(F("<td>Sensor ID</td>"));
              client.println(F("<td>"));
              client.print(senspin);
              client.print(F("</td>"));
              client.println(F("</tr>"));

              // Sensor Status
              client.println(F("<tr>")); 
              client.println(F("<td>Status</td>"));
              if (sensorstatus[senspin-3] == 1){
                client.println(F("<td class='dup'>"));
                client.print(F("On"));
                digitalWrite(senspin,HIGH);

              }
              else{
                client.println(F("<td class='sup'>"));
                client.print(F("Off"));
                digitalWrite(senspin,LOW);

              }
              client.println(F("</td>"));
              client.println(F("</tr>")); 

              // Sensor Type
              client.println(F("<tr>")); 
              client.println(F("<td valign='top'>Type</td>")); 
              client.println(F("<td>"));

              if (sensorconftype[sensornum - 1] == 1){
                client.print(F("<input type='radio' name='S"));
                client.print(sensornum);
                client.println(F("' value='A'>Auto"));
                client.print(F("<input type='radio' name='S"));
                client.print(sensornum);
                client.println(F("' value='M' checked>Manual"));
              }
              else{
                client.print(F("<input type='radio' name='S"));
                client.print(sensornum);
                client.println(F("' value='A' checked>Auto")); 
                client.print(F("<input type='radio' name='S"));
                client.print(sensornum);
                client.println(F("' value='M'>Manual"));
              }

              client.println(F("</td>"));
              client.println(F("</tr>"));

              // Sensor Control
              client.println(F("<tr>")); 
              client.println(F("<td valign='top'>Control</td>")); 
              client.println(F("<td>"));

              if (sensorstatus[sensornum - 1] == 0){
                client.print(F("<input type='submit' name='C"));
                client.print(sensornum);
                client.println(F("' value='Enable'"));
                if (sensorconftype[sensornum - 1] == 0){
                  client.println(F("Disabled"));                
                }
                client.println(F(">"));

              }
              else{
                client.print(F("<input type='submit' name='C"));
                client.print(sensornum);
                client.println(F("' value='Disable'"));
                if (sensorconftype[sensornum - 1] == 0){
                  client.println(F("Disabled"));                
                }
                client.println(F(">")); 
              }

              client.println(F("</td>"));
              client.println(F("</tr>"));

              // End Table
              client.println(F("</table>"));
              client.println(F("</td>"));
              sensornum++;
            }

            // I'm Done Drawing

            sensornum=1; // reset sensor count

            client.print(F("</tr><tr><td colspan='"));
            client.print(sensorcount);
            client.print(F("' class='helpHed'>"));
            client.print(F("Current Date and Time "));
            if (day(t) < 10) {
              client.print(F("0"));
            }
            client.print(day(t));
            client.print(F("-")); 
            if (month(t) < 10) {
              client.print(F("0"));
            }
            client.print(month(t));
            client.print(F("-"));
            client.println(year(t));

            if (hour(t) < 10) {
              client.print(F("0"));
            }

            client.print(hour(t) + adjustBSTEurope()); 
            client.print(F(":"));
            if (minute(t) < 10) {
              client.print(F("0"));
            } 
            client.print(minute(t));
            client.print(F(":"));
            if (second(t) < 10) {
              client.print(F("0"));
            }
            client.println(second(t)); 


            client.println(F("</td></tr></table><br>"));
            client.println(F("<input type='submit' value='Submit'>"));
            client.println(F("<INPUT TYPE='button' onClick='history.go(0)' VALUE='Refresh'>"));          
            client.println(F("</form>"));

            // print links
            client.println(F("</br>"));
            client.print(F("<a href="));
            Serial.println(Ethernet.localIP());
            ;
            client.println(F("/settings>Settings</a><br />"));             
            client.println(F("</html>"));
            break;
          }
          if (c == '\n') {
            // you're starting a new line
            currentLineIsBlank = true;
          } 
          else if (c != '\r') {
            // you've gotten a character on the current line
            currentLineIsBlank = false;
          }
        }

      }
    }
    // give the web browser time to receive the data
    delay(10);
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
    
  }

}
unsigned long getNtpTime()
{
  sendNTPpacket(timeServer); // send an NTP packet to a time server
  delay(1000);
  if ( Udp.parsePacket() ) { 
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read packet into buffer

    //the timestamp starts at byte 40, convert four bytes into a long integer
    unsigned long hi = word(packetBuffer[40], packetBuffer[41]);
    unsigned long low = word(packetBuffer[42], packetBuffer[43]);
    // this is NTP time (seconds since Jan 1 1900
    unsigned long secsSince1900 = hi << 16 | low;  
    // Unix time starts on Jan 1 1970
    const unsigned long seventyYears = 2208988800UL;     
    unsigned long epoch = secsSince1900 - seventyYears;  // subtract 70 years
    return epoch;
  }
  return 0; // return 0 if unable to get the time 
}

// send an NTP request to the time server at the given address 

unsigned long sendNTPpacket(IPAddress address)

{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);  // set all bytes in the buffer to 0

  // Initialize values needed to form NTP request
  packetBuffer[0] = B11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum
  packetBuffer[2] = 6;     // Max Interval between messages in seconds
  packetBuffer[3] = 0xEC;  // Clock Precision
  // bytes 4 - 11 are for Root Delay and Dispersion and were set to 0 by memset
  packetBuffer[12]  = 49;  // four-byte reference ID identifying
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // send the packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();
}

int adjustBSTEurope()
{
  // last sunday of march
  int beginBSTDate=  (31 - (5* year() /4 + 4) % 7);
  int beginBSTMonth=3;
  //last sunday of october
  int endBSTDate= (31 - (5 * year() /4 + 1) % 7);
  int endBSTMonth=10;
  // BST is valid as:
  if (((month() > beginBSTMonth) && (month() < endBSTMonth))
    || ((month() == beginBSTMonth) && (day() >= beginBSTDate)) 
    || ((month() == endBSTMonth) && (day() <= endBSTDate)))
    return 1;  // BST = utc +1 hour
  else return 0; // GMT = utc
}

int sensorAutoRun()
{
  // Set current time
  time_t t = now();
  int currentTime = ((hour(t) + adjustBSTEurope()) * 100) + minute(t);

  // off time:
  if (((currentTime >= offTime) && (currentTime < onTime)))
    return 1;  // Sensor should be off
  else return 0; // Sensor should be on

}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}













