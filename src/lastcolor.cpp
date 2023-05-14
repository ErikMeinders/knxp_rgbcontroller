#include "knxapp.h"
#include "FS.h"
#include "SPIFFS.h"

extern bool spiffMountedInSetup;
bool lastColorValid[maxRGBChannels] = {false,false,false,false,false};

static DPT_Color_RGB lastColor[maxRGBChannels];
static DPT_Color_RGB onDiskColor[maxRGBChannels];

static const DPT_Color_RGB RGB_Error =      { 128, 8, 8 };
static const DPT_Color_RGB RGB_NO_SPIFFS =  { 8, 8, 128 };

void dumpLastColor()
{
    for(int i=0 ; i < maxRGBChannels ; i++)
    {
        Printf("lastColor[%d] = %d %d %d [%s]\n", i, lastColor[i].R, lastColor[i].G, lastColor[i].B, lastColorValid[i] ? "valid" : "invalid");
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
        Log.verbose("-- No change in lastColor, not writing to EEPROM\n");
        return;
    }

    File file = SPIFFS.open("/startcolors.rgb", "w");

    // write colors, one line per RGB channel

    for(int ch=0 ; ch < maxRGBChannels ; ch++)
    {
        file.printf("%d %d %d\n", lastColor[ch].R, lastColor[ch].G, lastColor[ch].B);
    }
    file.close();
    dumpLastColor();

}

/**
 * @brief helper function to read a line from a file
 * 
 * @param file 
 * @param buf 
 * @param buflen 
 * @return int number of characters read
 */
static int readLn(File &file, char *buf, int buflen)
{
    int i = 0;
    char c;

    while( file.available() && (c = file.read()) != '\n' && i < buflen-1 ) {
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

/**
 * @brief read lastColor array from EEPROM -- well, SPIFFS actually
 * 
 */
static void readLastColorFromEEPROM()
{
    char line[16];

    Printf("Read lastColor from EEPROM\n");

    File file = SPIFFS.open("/startcolors.rgb", "r");

    if (!file) {
        Log.warning("No settings file found, using default values\n");
        Printf("No settings file found, using default values\n");
        return;
    }

    // read colors, one line per RGB channel

    for(int ch=0 ; ch < maxRGBChannels ; ch++)
    {
        if(readLn(file, line, sizeof(line)))
        {
            int R, G, B;

            sscanf(line, "%d %d %d", &R, &G, &B);
            Printf("Read RGB[%d] %d %d %d\n",ch, R, G, B)
            lastColor[ch].R = R;
            lastColor[ch].G = G;
            lastColor[ch].B = B;
            lastColorValid[ch] = true;

            if(ch == maxRGBChannels-1) {
                memcpy(onDiskColor, lastColor, sizeof(lastColor));
            }
        } else {
            Log.warning("Settings file corrupt, using default values\n");
            Printf("Settings file corrupt, using default values\n"); 
            lastColor[ch] = RGB_Error;
            lastColorValid[ch] = true;
            break;
        }
    }
    file.close();
    dumpLastColor();
}

/**
 * @brief initialize lastColor array
 * 
 */
void initLastColors()
{
    spiffMountedInSetup = SPIFFS.begin();

    if(spiffMountedInSetup) {
        readLastColorFromEEPROM();
    } else {
        // SPIFFS not mounted, use default values
        for(int ch=0 ; ch < maxRGBChannels ; ch++)
        {
            lastColor[ch] = RGB_NO_SPIFFS;
        }
    }
}

/**
 * @brief Get the RGB from Last 
 * 
 * @param ch 
 * @param RGB 
 */
void getRGBfromLast(int ch, DPT_Color_RGB &RGB)
{
    // if lastColor is not valid report ERROR color

    if(!lastColorValid[ch]) 
    {
        RGB = RGB_Error;
    } else  {
        RGB = lastColor[ch];
    }
}

/**
 * @brief Put the RGB in Last 
 * 
 * @param ch 
 * @param RGB 
 */
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

    lastColorValid[ch] = true;

    writeLastColorToEEPROM();
}

