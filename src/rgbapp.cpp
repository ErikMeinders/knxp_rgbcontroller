#include "rgbapp.h"
#include "lastcolor.h"
#include "FS.h"
#include "SPIFFS.h"

extern bool spiffMountedInSetup;

bool colorFlowRunning[maxRGBChannels] = { false, false, false, false, false };

// RGB channels indexed by RGB channel to find 3 PWM channels

const RGBChannel RGB2PWMchannel[maxRGBChannels] = { 
    { 0,  1,  2 }, 
    { 3,  4,  5 }, 
    { 6,  7,  8 }, 
    { 9, 10, 11 }, 
    {12, 13, 14 }
};

// helper functions for KNX RGB Data Type


/**
 * @brief Get RGB channel from group object
 * 
 * @param go
 * @param rgb
 */
void getRGBfromGO(GroupObject& go, DPT_Color_RGB& rgb)
{
    DPT_Color_RGB * rgbValue = (DPT_Color_RGB *) go.valueRef();

    Log.verbose("Retrieve RGB in %d bytes from GO# %d ", go.valueSize(), go.asap());

    rgb.R = rgbValue->R;
    rgb.G = rgbValue->G;
    rgb.B = rgbValue->B;

    Log.verbose("  RGB %d %d %d\n", rgb.R, rgb.G, rgb.B);
}

/**
 * @brief Store RGB value in group object
 * 
 * @param go 
 * @param rgb 
 */
void putRGBinGO(GroupObject& go, DPT_Color_RGB rgb, bool publish)
{
    DPT_Color_RGB *  rgbValue = (DPT_Color_RGB *) go.valueRef();

    Log.trace("Store RGB in %d bytes in GO# %d\n", go.valueSize(), go.asap());
    Log.trace("  RGB IN %d %d %d\n", rgb.R, rgb.G, rgb.B);
 
    rgbValue->R = rgb.R;
    rgbValue->G = rgb.G;
    rgbValue->B = rgb.B;

    if (publish) {
        go.objectWritten();
    }

    //return; // trust

    // compare! when not trusting ;-)

    DPT_Color_RGB rgb2;
    getRGBfromGO(go, rgb2);
    Log.trace("  RGB RT %d %d %d\n", rgb2.R, rgb2.G, rgb2.B);
 
}

/**
 * @brief Get RGB from params
 * 
 * @param rgbCh
 * @param rgb
 */
void getRGBfromParams(int rgbCh, DPT_Color_RGB& rgb)
{
    rgb.R = knx.paramByte(rgbCh*3+1);
    rgb.G = knx.paramByte(rgbCh*3+2);
    rgb.B = knx.paramByte(rgbCh*3+3);
}

/**
 * @brief function to determine the next color in the color wheel
 * 
 * @param colorRGB 
 * @return DPT_Color_RGB 
 */
void nextColor(DPT_Color_RGB colorRGB, DPT_Color_RGB &nextRGB)
{
    nextRGB.R = colorRGB.R;
    nextRGB.G = colorRGB.G;
    nextRGB.B = colorRGB.B;

    if( colorRGB.R == 255 && colorRGB.G == 0 && colorRGB.B < 255 )
    {
        nextRGB.B++;
    }
    else if( colorRGB.R > 0 && colorRGB.G == 0 && colorRGB.B == 255 )
    {
        nextRGB.R--;
    }
    else if( colorRGB.R == 0 && colorRGB.G < 255 && colorRGB.B == 255 )
    {
        nextRGB.G++;
    }
    else if( colorRGB.R == 0 && colorRGB.G == 255 && colorRGB.B > 0 )
    {
        nextRGB.B--;
    }
    else if( colorRGB.R < 255 && colorRGB.G == 255 && colorRGB.B == 0 )
    {
        nextRGB.R++;
    }
    else if( colorRGB.R == 255 && colorRGB.G > 0 && colorRGB.B == 0 )
    {
        nextRGB.G--;
    } else
    {
        nextRGB.R = 255;
        nextRGB.G = 0;
        nextRGB.B = 0;
    }
};

/**
 * @brief Set the 3 PWM channels in RGB channel to RGB value
 * 
 * @param rgbCh - RGB channel (0..4)
 * @param rgbValue
*/
void putRGBinHW(int rgbCh, DPT_Color_RGB rgbValue)
{
    // split value into RGB components

    uint32_t red     = rgbValue.R;
    uint32_t green   = rgbValue.G;
    uint32_t blue    = rgbValue.B;
    
    Log.trace("Set RGB channel %d to %d %d %d\n", rgbCh, red, green, blue);

    ledcWrite(RGB2PWMchannel[rgbCh].redPWMChannel,    red);
    ledcWrite(RGB2PWMchannel[rgbCh].greenPWMChannel,  green);
    ledcWrite(RGB2PWMchannel[rgbCh].bluePWMChannel,   blue);
   
}

/**
 * @brief Get the RGB value from the 3 PWM channels in RGB channel
 * 
 * @param rgbCh - RGB channel (0..4)
 * @param rgbValue
*/
void getRGBfromHW(int rgbCh, DPT_Color_RGB& rgbValue)
{
    uint32_t red     = ledcRead(RGB2PWMchannel[rgbCh].redPWMChannel);
    uint32_t green   = ledcRead(RGB2PWMchannel[rgbCh].greenPWMChannel);
    uint32_t blue    = ledcRead(RGB2PWMchannel[rgbCh].bluePWMChannel);

    rgbValue.R = red    > 255 ? 255 : red;
    rgbValue.G = green  > 255 ? 255 : green;
    rgbValue.B = blue   > 255 ? 255 : blue;
}

// KNX callback functions

/**
 * @brief callback function for group object that handles On/Off 
 * 
 * @param go 
 */
void callbackOnOff(GroupObject& go)
{
    int rgbCh = goToRGBChannel(go);
    
    DPT_Color_RGB   RGB_DARK = { 0, 0, 0 },
                    RGB_START_DEFAULT = { 64, 128, 255 },
                    rgb = {0,0,0};

    uint8_t turnOn = go.value();

    Log.trace("[Callback] turn RGB channel %d %s\n",  rgbCh, turnOn ? "On" : "Off");

    if ( turnOn )
    {          
        if( parameterChannelStartWithDefaultColor(rgbCh) )
        {
            getRGBfromParams(rgbCh, rgb);
            Log.trace("  ON: Colour retrieved from Parameters R: %d G: %d B: %d\n", rgb.R, rgb.G, rgb.B);
        } else
        {
            getRGBfromLast(rgbCh, rgb);
            Log.trace("  ON: Colour retrieved from lastColor  R: %d G: %d B: %d\n", rgb.R, rgb.G, rgb.B);
        }

        // 'Emergency'-value: if all channels are 0, set to RGB_START_DEFAULT - otherwise the LEDs will be off when turning on

        if ( rgb.R == 0 && rgb.G == 0 && rgb.B == 0 )
            rgb = RGB_START_DEFAULT;

        putRGBinHW( rgbCh, rgb );
        putRGBinGO(rgbFeedbackFunction( rgbCh ), rgb, true );
        putRGBinLast(rgbCh,rgb);

    } else {
        
        // stop colorRun if running

        if( rgbColorFlowFunction(rgbCh).value() ) stopColorFlow(rgbCh);

        // store current color in lastColor and save to EEPROM

        getRGBfromHW(rgbCh, rgb);
        putRGBinLast(rgbCh, rgb);

        // set RGB channel to dark & update feedback object
        
        putRGBinHW( rgbCh, RGB_DARK);
        putRGBinGO(rgbFeedbackFunction( rgbCh ), RGB_DARK, true );
        
    }

    onOffFeedbackFunction( rgbCh ).value( go.value() );
    
}

/**
 * @brief callback function for group object that handles RGB setting
 * 
 * @param go 
 */
void callbackRGB(GroupObject& go)
{
    int rgbCh = goToRGBChannel(go);
    DPT_Color_RGB rgb;
    
    getRGBfromGO(go, rgb);
    Log.trace("[Callback] change color of RGB channel %d  [%x %x %x]\n", rgbCh, rgb.R, rgb.G, rgb.B);

    if( rgb.R == 0 && rgb.G == 0 && rgb.B == 0 ) return;

    putRGBinHW( rgbCh, rgb );
    putRGBinLast(rgbCh, rgb);
    putRGBinGO(rgbFeedbackFunction( rgbCh ), rgb, true );

    // side-effect of sending color could be the on/off state changes !

    onOffFeedbackFunction( rgbCh ).value( 1 );
    
}

void startColorFlow(int rgbCh)
{
    if(colorFlowRunning[rgbCh]) return;

    Log.trace("  RGB flow ON, set channel ON\n");
    
    colorFlowRunning[rgbCh] = true;
    onOffFeedbackFunction( rgbCh ).value(true);
}

void stopColorFlow(int rgbCh)
{
    if(!colorFlowRunning[rgbCh]) return;

    Log.trace("  RGB flow turned OFF\n");

    colorFlowRunning[rgbCh] = false;
    rgbColorFlowFunction(rgbCh).value(false);
}

/**
 * @brief callback function for group object that handles RGB Run
 * 
 * @param go 
 */
void callbackRGBRun(GroupObject& go)
{
    int rgbCh = goToRGBChannel(go);

    Log.trace("[Callback] RGB run %s for channel %d\n", go.value() ? "ON" : "OFF", rgbCh );
    if( (bool) go.value() != colorFlowRunning[rgbCh] )
    {
        Log.trace("  Value changed\n");

        // when RGB run is activated, make sure to set channel to ON

        if( (bool) go.value() )
        {
            startColorFlow(rgbCh);
  
        } else
        {
            DPT_Color_RGB rgb;

            stopColorFlow(rgbCh);
            
            getRGBfromHW(rgbCh, rgb);
            Printf("Stopping color flow with color from HW: %02x %02x %02x\n", rgb.R, rgb.G, rgb.B);
            putRGBinLast(rgbCh, rgb);
            putRGBinGO(rgbFeedbackFunction( rgbCh ), rgb, false );

        }
    } else {
        Log.trace("  Value unchanged\n");
       
    }
}

void rgbStatus()
{
    for(int ch=0 ; ch < maxRGBChannels ; ch++)
    {
        DPT_Color_RGB rgb; 

        GroupObject& go = rgbFeedbackFunction(ch);
        bool on = onOffFeedbackFunction(ch).value();

        Printf("\n[%s] Channel %d is %s Flow is %s\n",  parameterChannelEnabled(ch) ? "Enabled" : "DISABLED", ch, on ? "ON " : "Off", colorFlowRunning[ch] ? "ON " : "Off");
        Printf("Start from default color %s\n", parameterChannelStartWithDefaultColor(ch) ? "true" : "false");
        
        if(parameterChannelStartWithDefaultColor(ch))
        {
            getRGBfromParams(ch, rgb);
            Printf("Inital color %02x %02x %02x\n", rgb.R, rgb.G, rgb.B);
        } else {
            getRGBfromLast(ch, rgb);
            Printf("LastColor    %02x %02x %02x\n",  rgb.R, rgb.G, rgb.B);
        }
        getRGBfromGO(go, rgb);
        Printf("GO FB color  %02x %02x %02x\n", rgb.R, rgb.G, rgb.B);
        getRGBfromHW(ch, rgb);
        Printf("HW color     %02x %02x %02x\n", rgb.R, rgb.G, rgb.B);
    }

    if(spiffMountedInSetup)
    {
        Printf("SPIFFS mounted in setup\n");
        // list SPIFFS content
        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while(file){
            Printf("  %s %d\n", file.name(), file.size());
            file = root.openNextFile();
        }
    } else {
        Printf("SPIFFS mount in setup failed\n");
    }
}