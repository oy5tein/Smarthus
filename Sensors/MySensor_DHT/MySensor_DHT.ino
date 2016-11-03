/*DHT lib in arduino version 1.3.0 is broken, select 1.2.3 */
#include <DHT.h>


/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 * 
 * DESCRIPTION
 * Example sketch showing how to measue light level using a LM393 photo-resistor 
 * http://www.mysensors.org/build/light
 */


#define DHTPIN 3     // what digital pin we're connected to

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  dht.begin();
}

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69


#include <SPI.h>
#include <MySensors.h>  


#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1

MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

void presentation()  {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Humidity/Temperature", "1.0");
  wait(2000);
  // Register all sensors to gateway (they will be created as child devices)

  present(CHILD_ID_HUM, S_HUM);
  wait(2000);
  present(CHILD_ID_TEMP, S_TEMP);

}
void loop()      
{ 
    // Wait a few seconds between measurements.
  wait(30000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  if (isnan(t)) {
      Serial.println("Failed reading temperature from DHT");
  } else{
    send(msgTemp.set(t, 1));
    Serial.print("Temp: ");
    Serial.println(t);
    
  }

  if (isnan(h)) {
      Serial.println("Failed reading hum from DHT");
  } else{
    send(msgHum.set(h, 1));
    
  }

  
}



