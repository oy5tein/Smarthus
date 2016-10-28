/*
 * Decode Clas Ohlson ESIC WT450HT Temp / Hum sensor  
 * 
 * Work based on 
 * https://netcompsys.wordpress.com/2011/11/20/wireless-temperature-and-humidity/
 * That in turn is based on v0.1 code by Jaakko Ala-Paavola 2011-02-27 
 * http://ala-paavola.fi/jaakko/doku.php?id=wireless
 * 
 * 
 */

  
// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69


#include <SPI.h>
#include <MySensors.h>


//#define DEBUG 0 //prints additional information to serial port
#define HOUSECODE 2

MyMessage msgHum1(10, V_HUM);
MyMessage msgTemp1(11, V_TEMP);
MyMessage msgHum2(20, V_HUM);
MyMessage msgTemp2(21, V_TEMP);
MyMessage msgHum3(30, V_HUM);
MyMessage msgTemp3(31, V_TEMP);
MyMessage msgHum4(40, V_HUM);
MyMessage msgTemp4(41, V_TEMP);

#define TIMEOUT 1000
#define BIT0_LENGTH 2000
#define BIT1_LENGTH 1000
#define VARIATION 500
#define DATA_LENGTH 5
#define SENSOR_COUNT 5
#define NETWORK_COUNT 8
#define MSGLENGTH 36
#define DELAY_BETWEEN_POSTS 200  

struct sensor {
  unsigned char humidity;
  signed char   temp_int;
  unsigned char temp_dec;
  unsigned char updated;
  unsigned char presented;
};

struct sensor measurement[NETWORK_COUNT+1][SENSOR_COUNT+1];

unsigned long time;
unsigned char bitcount = 0;
unsigned char bytecount = 0;
unsigned char second_half = 0;
unsigned char data[DATA_LENGTH];

unsigned long lastPostTimestamp;
unsigned long currentTime;
boolean metric = true; 


int chWithNewData=0;


// This is the interupt driven receive data code

void receive()
{
    unsigned char i;
    unsigned char bit;
    unsigned long current = micros();
    unsigned long diff = current-time;
    time=current;
    digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    
    if ( diff < BIT0_LENGTH + VARIATION && diff > BIT0_LENGTH - VARIATION )
     {
        bit = 0;
        second_half = 0;
     }
     else if ( diff < BIT1_LENGTH + VARIATION && diff > BIT1_LENGTH - VARIATION )
     {
       if (second_half)
       {
         bit = 1;
         second_half = 0;  
       }
       else
       {
         second_half = 1;
         return;
       }
     }
     else 
     {
      goto reset;
     }
     
    
     data[bitcount/8] = data[bitcount/8]<<1;
     data[bitcount/8] |= bit;
     bitcount++;
     
     if ( bitcount == 4 )
     {
       if ( data[0] != 0x0c )
         goto reset;
       bitcount = 8;
#ifdef DEBUG
      Serial.print('#');  // data recieved but not sensor data
#endif
     }

     if ( bitcount >= MSGLENGTH )
     {
       unsigned char net, id;
       int t_int;
       unsigned char rh, t_dec;

#ifdef DEBUG       // show me the raw data in hex
       for (i=0; i<DATA_LENGTH; i++) {
         Serial.print(data[i], HEX);
         Serial.print(' ');
       }
#endif

       Serial.print("NET:");                // Housecode is 1 to 15
       net = 0x0F & (data[1]>>4);           // the -1 in original code removed
                                            // 0x07 --> 0x0F to permit NET up to 15 tobe decodeed instead of 7
       if (net<=9) {                        // just keeping things lined up
           Serial.print(" "); 
       }
       Serial.print(net,DEC);
       Serial.print(" ID:");                // Channel is 1 to 4, this and above changed so serial.print agrees with lcd displays
       id = 0x03 & (data[1]>>2)+1;
       Serial.print(id, DEC);
       Serial.print(" RH:");                // Only WT450H has a Humidity sensor, WT450 is just Temperature
       rh = data[2];
       if (rh==0) {                         // just keeping things lined up
           Serial.print(" "); 
       }
       Serial.print(rh, DEC);
       Serial.print(" T:");
       t_int = data[3]-50;
       Serial.print(t_int, DEC);
       t_dec = data[4]>>1;
       Serial.print('.');
       Serial.print(t_dec,DEC);
       Serial.println("");
       

         
// CSV data out to serial host an alternative to the above

//       Serial.print("ESIC,");               // The name on the front
//       net = 0x0F & (data[1]>>4);           // the -1 in original code removed
//                                            // 0x07 --> 0x0F to permit NET up to 15 tobe decodeed instead of 7
//       Serial.print(net,DEC);               // Housecode is 1 to 15
//       Serial.print(",");                  
//       id = 0x03 & (data[1]>>2)+1;
//       Serial.print(id, DEC);               // Channel is 1 to 4, this and above changed so serial.print agrees with lcd displays
//       Serial.print(",");                
//       rh = data[2];
//       Serial.print(rh, DEC);               // Only WT450H has a Humidity sensor, WT450 is just Temperature
//       Serial.print(",");
//       t_int = data[3]-50;
//       Serial.print(t_int, DEC);
//       t_dec = data[4]>>1;
//       Serial.print('.');
//       Serial.print(t_dec,DEC);
//       Serial.println("");

         
       if ( net < NETWORK_COUNT )
       {
         measurement[net][id].temp_int = t_int;
         measurement[net][id].temp_dec = t_dec;
         measurement[net][id].humidity = rh;
         measurement[net][id].updated = true; 
       }
       goto reset;  
     }
    return;

reset: // set all the receiver variables back to zero
  for (i=0; i<DATA_LENGTH; i++)
    data[i] = 0;

   bytecount = 0;
   bitcount = 0;
   second_half = 0;
   return;
}

void setup()
{
  int k,l; 
//  Serial.begin(115200);
 pinMode(13, OUTPUT);
 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Humidity/Temperature", "3.0");
  
  // Register all sensors to gw (they will be created as child devices)
  present(10, S_HUM);
  wait(1000);
  present(11, S_TEMP);
  wait(1000);
  present(20, S_HUM);
  wait(1000);
  present(21, S_TEMP);
  wait(1000);
  present(30, S_HUM);
  wait(1000);
  present(31, S_TEMP);
  wait(1000);
  present(40, S_HUM);
  wait(1000);
  present(41, S_TEMP);

  metric = getConfig().isMetric;


// Serial.println("Wireless Temperature & Humidity Sensors to Serial");
//  Serial.println("Dir --> Wireless_Temp_Serial8");

 
  for (l=0; l<NETWORK_COUNT; l++)    // the 'network' here is the wireless sensor network
  {
    for (k=0; k<SENSOR_COUNT; k++)
    {
      measurement[l][k].temp_int = 0;
      measurement[l][k].temp_dec = 0;
      measurement[l][k].humidity = 0;
      measurement[l][k].updated = false;
      measurement[l][k].presented = false;
    }
  }
 
//  Serial.println("Initialise interrupt");
  attachInterrupt(1,receive,CHANGE); // interrupt 0 is pin D2 1 is pin D3 
                                     // needs to be 1 for an ENC28J60 ethernet shield
  
  time = micros();
 lastPostTimestamp = currentTime;
//  Serial.println("Setup complete - Waiting for data");
}


boolean newDataReceived(const unsigned int n, const unsigned int c)
{
  if (measurement[n][c].updated==true)
  return true;
  else
  return false;
}






void loop()
{
  currentTime = millis();
  unsigned long difftime = currentTime - lastPostTimestamp;
   
  if(difftime > DELAY_BETWEEN_POSTS){   
    lastPostTimestamp = currentTime;

    digitalWrite(13, LOW);  
   
   //int n = 2;     
   for (int n=0; n<NETWORK_COUNT; n++){    // the 'network' here is the wireless sensor network
    for (int c=0; c<SENSOR_COUNT; c++){
     if ( (true == newDataReceived(n,c)) ){

      /* calc child id for humidity and temp*/
      unsigned int childIdHum = n*10 + 2*c;   
      unsigned int childIdTemp = n*10 + 2*c+1;

     if (measurement[n][c].presented == 0){
     /* this new network/channel has not been presented to the gateway before*/
        present(childIdHum, S_HUM);
        wait(500);
        present(childIdHum, S_TEMP);
        wait(500);
      }

     float temp= (float) measurement[n][c].temp_dec * 0.1f;
     temp = temp + measurement[n][c].temp_int;
     float rh = measurement[n][c].humidity;
     measurement[n][c].updated = false;

     
     
     switch (c)
     {
       case 1:
         send(msgTemp1.set(temp, 1));
         wait(2000);
         send(msgHum1.set(rh, 1));
         break;       
       case 2:
         send(msgTemp2.set(temp, 1));
         wait(2000);
         send(msgHum2.set(rh, 1));
         break;       
       case 3:
         send(msgTemp3.set(temp, 1));
         wait(2000);
         send(msgHum3.set(rh, 1));
         break;       
       case 0:
         send(msgTemp4.set(temp, 1));
         wait(2000);
         send(msgHum4.set(rh, 1));
         break;       
     }

     }

   }}
   }
}
 


