#include "knxapp.h"
#include "FS.h"
#include "SPIFFS.h"

extern bool spiffMountedInSetup;
bool lastColorValid = false;

static DPT_Color_RGB lastColor[maxRGBChannels];
static DPT_Color_RGB onDiskColor[maxRGBChannels];

void dumpLastColor()
{
    for(int i=0 ; i < maxRGBChannels ; i++)
    {
        Printf("lastColor[%d] = %d %d %d\n", i, lastColor[i].R, lastColor[i].G, lastColor[i].B);
    }
}

/**
 * @brief Write lastColor array to EEPROM -- well, SPIFFS actually
 * 
 */
static void writeLastColorToEEPROM()
{
    Printf("Write lastColor to EEPROM\n");

    if(memcmp(lastColor, onDiskColor, sizeof(lastColor)) == 0) {
        Log.verbose("No change in lastColor, not writing to EEPROM\n");
        return;
    }

    File file = SPIFFS.open("/startcolors.rgb", "w");

    if( file.write((uint8_t *)lastColor, sizeof(lastColor)) != sizeof(lastColor) ) {
        Log.warning("Error writing settings file\n"); 
    } else {
        Printf("Settings file written\n");
        memcpy(onDiskColor, lastColor, sizeof(lastColor));
    }
    dumpLastColor();
    file.close();
}

/**
 * @brief read lastColor array from EEPROM -- well, SPIFFS actually
 * 
 */
static bool readLastColorFromEEPROM()
{
    Printf("Read lastColor from EEPROM\n");

    File file = SPIFFS.open("/startcolors.rgb", "r");

    if (!file) {
        Log.warning("No settings file found, using default values\n");
        Printf("No settings file found, using default values\n");
        return false;
    }

    if( file.read((uint8_t *)lastColor, sizeof(lastColor)) == sizeof(lastColor) ) {
        lastColorValid = true;
        memcpy(onDiskColor, lastColor, sizeof(lastColor));
    } else {
        Log.warning("Settings file corrupt, using default values\n");
        Printf("Settings file corrupt, using default values\n"); 
    }
    file.close();
    Printf("lastColorValid = %d\n", lastColorValid);
    dumpLastColor();
    return lastColorValid;
}

void getRGBfromLast(int ch, DPT_Color_RGB &RGB)
{
    // if lastColor is not valid, try to read it from EEPROM

    if(!lastColorValid) {
        if( !readLastColorFromEEPROM() ) {
            Log.warning("No valid lastColor found, using default values\n");
        }
        for(int i=0 ; i < maxRGBChannels ; i++)
        {
            if( !lastColorValid || (lastColor[i].R == 0 && lastColor[i].G == 0 && lastColor[i].B == 0) ) 
            {
                lastColor[i].R = 32;
                lastColor[i].G = 32;
                lastColor[i].B = 32;
            }
        }
        lastColorValid = true;

    }   
    
    RGB = lastColor[ch];
}

void putRGBinLast(int ch, DPT_Color_RGB RGB)
{
    // do not store black as last color
    
    if( RGB.R == 0 && RGB.G == 0 && RGB.B == 0 ) {
        return;
    }

    // do not store when no change

    if(lastColor[ch].R == RGB.R && lastColor[ch].G == RGB.G && lastColor[ch].B == RGB.B)
        return;
    
    lastColor[ch].R = RGB.R;
    lastColor[ch].G = RGB.G;
    lastColor[ch].B = RGB.B;

    writeLastColorToEEPROM();
}

