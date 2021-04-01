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
    * 5 Hz LED blink indicates an SD card error
    * 1 Hz blink indicates gps doesn't have a lock yet (lock should take ~30s)
    * Each time the gps has a new valid packet, it is read
    * Relevant data is extracted and written to the file in csv format:
        * UTC timestamp
        * lat, long - degrees
        * mean altitude above sea level - m
        * vertical speed - m/s
        * ground speed - m/s
        * ground track - degrees
    * SD write buffer is flushed each iteration to protect data
 
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


    datafile = SD.open(filename, FILE_WRITE);
}


void loop(){
    if (gps.readSensor()){
        
        char utc_buffer[32];
        utc_from_gps(gps, utc_buffer);

        datafile.print(utc_buffer);
        datafile.print(',');
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
        datafile.println(gps.getMotionHeading_deg());

        
        datafile.flush();
    }
}

void utc_from_gps(UBLOX gps, char* outStr){
    /* yyyy-mm-dd--hh-mm-ss--nnnnnnnnn */
    snprintf_P(outStr, 32, 
    PSTR("%u-%02u-%02u--%02u-%02u-%02u--%09ld"), 
    gps.getYear(), gps.getMonth(), gps.getDay(),
    gps.getHour(), gps.getMin(), gps.getSec(),
    gps.getNanoSec());
}