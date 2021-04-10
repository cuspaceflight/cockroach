/*  Daniel Fearn for CUSF 30/03/21 */

/*  Project Cockroach

    SD card SPI bus connected as follows:
    * SS - pin 10
    * MOSI - pin 11
    * MISO - pin 12
    * SCK - pin 13

    MAX-M8Q gps connected as follows:
    * TP - pin 5
    * RX0 - pin 1 / TX
    * TX0 - pin 0 / RX (default Serial pins)
    
    Uses Bolder Flight Systems UBLOX by Brian Taylor
    https://github.com/bolderflight/UBLOX
    Available from Arduino Library Manager

    Summary:
    * 5 Hz LED blink indicates an SD card error
    * 1 Hz blink indicates gps doesn't have a lock yet (lock should take ~30s)
    * Once gps is locked, a new file is created in /data
    * File name is <timestamp>.csv
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

#define filepath "data"
#define filenameend ".csv"
char filename[41];

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

    gps.begin();

    while(!gps.readSensor()){
        /* gps not locked yet, 1 Hz blink */
        digitalWrite(LED_pin, flash);
        delay(500);
    }
    flash = false;
    digitalWrite(LED_pin, flash);

    char utc_buffer[32];
    utc_from_gps(gps, utc_buffer);
    snprintf_P(filename, 41, PSTR("%s/%s%s"), filepath, utc_buffer, filenameend);
    datafile = SD.open(filename, FILE_WRITE);
}


void loop(){
    if (gps.readSensor()){
        
        char utc_buffer[32];
        utc_from_gps(gps, utc_buffer);

        /* snprintf_P does not support floats/doubles directly in
        this implementation, so they must be formatted to strings 
        individually with dtostrf */

        /* 6 d.p. of precision ~10cm i.e. 999.999999 */
        char latit[11];
        dtostrf(gps.getLatitude_deg(), 10, 6, latit);
        char longit[11];
        dtostrf(gps.getLongitude_deg(), 10, 6, longit);

        /* 99999.9 */
        char mslheight[8];
        dtostrf(gps.getMSLHeight_m(), 7, 1, mslheight);

        /* 999.9 */
        char vertvelocity[6];
        dtostrf(gps.getDownVelocity_ms(), 5, 1, vertvelocity);
        
        char groundspeed[6];
        dtostrf(gps.getGroundSpeed_ms(), 5, 1, groundspeed);

        char groundtrack[6];
        dtostrf(gps.getMotionHeading_deg(), 5, 1, groundtrack);

        /* individual strings now formatted to the csv line */
        char line_buffer[80];
        snprintf_P(line_buffer, 80, 
        PSTR("%s,%s,%s,%s,%s,%s,%s"),
        utc_buffer, latit, longit, mslheight,
        vertvelocity, groundspeed, groundtrack);

        /* dtostrf pads with spaces, we want leading zeros */
        replace_spaces(line_buffer);

        datafile.println(line_buffer);
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

void replace_spaces(char* input){
  uint8_t i = 0;
  while (input[i] != '\0'){
    if(input[i] == ' ') input[i] = '0';
    i++;  
  }
}