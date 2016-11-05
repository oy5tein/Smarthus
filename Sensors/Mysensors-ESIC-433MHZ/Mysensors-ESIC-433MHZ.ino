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


#define LED_TRASMIT_TO_GW 8


#include <SPI.h>
#include <MySensors.h>


//#define DEBUG 0 //prints additional information to serial port

MyMessage msgHum(0, V_HUM);
MyMessage msgTemp(1, V_TEMP);


#define TIMEOUT 1000
#define BIT0_LENGTH 2000
#define BIT1_LENGTH 1000
#define VARIATION 500
#define DATA_LENGTH 5
#define SENSOR_COUNT 5
#define HOUSECODE_COUNT 16
#define MSGLENGTH 36
#define DELAY_BETWEEN_POSTS 200  

struct sensor {
  unsigned char humidity;
  signed char   temp_int;
  unsigned char temp_dec;
  unsigned char updated;
  unsigned char presented;
};

struct sensor measurement[HOUSECODE_COUNT+1][SENSOR_COUNT+1];

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
       

        
       if ( net < HOUSECODE_COUNT )
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

 pinMode(LED_TRASMIT_TO_GW, OUTPUT);
 
  // Send the Sketch Version Information to the Gateway
  sendSketchInfo("Humidity/Temperature", "3.0");
  /*dont present measurement here, its done runtime when new sensors are received*/


  for (l=0; l<HOUSECODE_COUNT; l++)    // the 'network' here is the wireless sensor network
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

    digitalWrite(LED_TRASMIT_TO_GW, LOW);  
   
        
   for (int n=0; n<HOUSECODE_COUNT; n++){    // n is the 433MHz housecode "network"
    for (int c=0; c<SENSOR_COUNT; c++){      
     if ( (true == newDataReceived(n,c)) ){

       
      unsigned int childIdHum;
      unsigned int childIdTemp;

      /* calc child id for humidity and temp
        Since child id is displayed as 0-255 we dont use hex 
          format of child id: HHS
          H = housecode 1-15
          S = Sensor 1-8
              1-4 temperature (1 = temp sensor 1)
              5-8 humidity    (5 = temp sensor 1)
      */

      if (c==0){
        /*channel 0 is labeled 4 on sensor*/

        childIdHum = n*10 + (4+4);
        childIdTemp = n*10 + 4;    
      }else{
        childIdHum = n*10 + (c+4) ;   
        childIdTemp = n*10 + c;
      }

     if (measurement[n][c].presented == 0){
     /* this new network/channel has not been presented to the gateway before*/


         Serial.print("present childid for Hum: ");
         Serial.println(childIdHum);
        present(childIdHum, S_HUM);
        wait(500);

         Serial.print("present childid for Temp: ");
         Serial.println(childIdTemp);
        present(childIdTemp, S_TEMP);
        wait(500);

        /*Now this network channel has been presented*/
        //measurement[n][c].presented = true;

      }

     /*indicate new trasmitted */
     digitalWrite(LED_TRASMIT_TO_GW, HIGH);  

     float temp= (float) measurement[n][c].temp_dec * 0.1f;
     temp = temp + measurement[n][c].temp_int;
     float rh = measurement[n][c].humidity;
     measurement[n][c].updated = false;

      
      Serial.print("Data for childid: ");
      Serial.print(childIdHum);
      Serial.print(" Value: ");
      Serial.print(rh);
      Serial.println("[%]");

      MyMessage msgTemp(childIdTemp, V_TEMP);
      send(msgTemp.set(temp, 1));
      wait(500);


      Serial.print("Data for childid: ");
      Serial.print(childIdTemp);
      Serial.print(" Value: ");
      Serial.print(temp);
      Serial.println("[C]");
      
      MyMessage msgHum(childIdHum, V_HUM);
      send(msgHum.set(rh, 1));
      wait(500);

      digitalWrite(LED_TRASMIT_TO_GW, LOW);

     }

   }}
   }
}
 


