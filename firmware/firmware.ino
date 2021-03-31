/*  Daniel Fearn for CUSF 30/03/21 */

/*  Project Cockroach

    SD card SPI bus connected as follows:
    * SS - pin 10
    * MOSI - pin 11
    * MISO - pin 12
    * SCK - pin 13
    
    Uses Bolder Flight Systems UBLOX by Brian Taylor
    https://github.com/bolderflight/UBLOX
    Available from Arduino Library Manager

    MAX-M8Q gps connected as follows:
    * TP - pin 5 (unused?)
    * RX0 - pin 1 / TX
    * TX0 - pin 0 / RX (default Serial pins)

    Summary:
    * Upon startup a new file named data_<index>.csv is created, where <index> is the smallest integer not already in use
    * 5 Hz LED blink indicates an SD card error, 1 Hz blink indicates gps doesn't have a lock yet (lock should take ~30s)
    * Each time the gps has a new valid packet, it is read
    * Relevant data is extracted and written to the file in csv format:
        * lat, long - degrees
        * mean altitude above sea level - m
        * vertical speed - m/s
        * ground speed - m/s
        * ground track - degrees
        * gps time of week - ms
        * time since startup - ms
    * Opening and closing the file each iteration should protect the data in the event of an unplanned kinetic disassembly
 
 */

#include <SPI.h>
#include <SD.h>
#include "UBLOX.h"

File datafile;
UBLOX gps(Serial,9600);

#define filenamestart "data_"
#define filenameend ".csv"
String filename;

#define SD_cs_pin 10
#define LED_pin LED_BUILTIN
bool flash = false;

void setup(){
    pinMode(LED_pin, OUTPUT);
    while(!SD.begin(SD_cs_pin)){
        /* SD error, 5 Hz blink */
        flash = !flash;
        digitalWrite(LED_pin, flash);
        delay(100);
    }
    flash = false;
    digitalWrite(LED_pin, flash);

    int index = 0;
    while(SD.exists(filenamestart + String(index) + filenameend)){
        index += 1;
    }
    filename = filenamestart + String(index) + filenameend;

    gps.begin();

    while(!gps.readSensor()){
        /* gps not locked yet, 1 Hz blink */
        digitalWrite(LED_pin, flash);
        delay(500);
    }
    flash = false;
    digitalWrite(LED_pin, flash);
    
}


void loop(){
    if (gps.readSensor()){
        datafile = SD.open(filename, FILE_WRITE);

        datafile.print(gps.getLatitude_deg());
        datafile.print(',');
        datafile.print(gps.getLongitude_deg());
        datafile.print(',');
        datafile.print(gps.getMSLHeight_m());
        datafile.print(',');
        datafile.print(gps.getDownVelocity_ms());
        datafile.print(',');
        datafile.print(gps.getGroundSpeed_ms());
        datafile.print(',');
        datafile.print(gps.getMotionHeading_deg());
        datafile.print(',');
        datafile.print(gps.getTow_ms());
        datafile.print(',');
        datafile.println(millis());
        
        datafile.close();
    }
}
