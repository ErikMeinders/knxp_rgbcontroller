#include "rgbapp.h"

DPT_Color_RGB lastColor[maxRGBChannels];
bool colorFlowRunning[maxRGBChannels];

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
void getRGBfromGO(GroupObject& go, DPT_Color_RGB *rgb)
{
    DPT_Color_RGB * rgbValue = (DPT_Color_RGB *) go.valueRef();

    Log.verbose("Retrieve RGB in %d bytes from GO# %d ", go.valueSize(), go.asap());

    rgb->R = rgbValue->R;
    rgb->G = rgbValue->G;
    rgb->B = rgbValue->B;

    Log.verbose("  RGB %d %d %d\n", rgb->R, rgb->G, rgb->B);
}

/**
 * @brief Store RGB value in group object
 * 
 * @param go 
 * @param rgb 
 */
void storeRGBinGO(GroupObject& go, DPT_Color_RGB rgb, bool publish)
{
    DPT_Color_RGB *  rgbValue = (DPT_Color_RGB *) go.valueRef();

    Log.verbose("Store RGB in %d bytes in GO# %d\n", go.valueSize(), go.asap());

    Log.verbose("  RGB IN %d %d %d\n", rgb.R, rgb.G, rgb.B);
    rgbValue->R = rgb.R;
    rgbValue->G = rgb.G;
    rgbValue->B = rgb.B;

    if (publish) {
        go.objectWritten();
        knx.loop();
        delay(1);
    }

    return; // trust

    // compare! when not trusting ;-)

    DPT_Color_RGB rgb2;
    getRGBfromGO(go, &rgb2);
    Log.trace("  RGB RT %d %d %d\n", rgb2.R, rgb2.G, rgb2.B);
 
}

/**
 * @brief function to determine the next color in the color wheel
 * 
 * @param colorRGB 
 * @return DPT_Color_RGB 
 */
DPT_Color_RGB nextColor(DPT_Color_RGB colorRGB)
{
    static DPT_Color_RGB nextRGB;

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
    
    return nextRGB;
};

/**
 * @brief Set the 3 PWM channels in RGB channel to RGB value
 * 
 * @param rgbCh - RGB channel (0..4)
 * @param rgbValue
*/
void setRGBChannelToColor(int rgbCh, DPT_Color_RGB rgbValue)
{
    // split value into RGB components

    uint8_t red     = rgbValue.R;
    uint8_t green   = rgbValue.G;
    uint8_t blue    = rgbValue.B;
    
    Log.trace("Set RGB channel %d to %d %d %d\n", rgbCh, red, green, blue);

    Log.trace("  Set RED   PWM channel %d to %d \n", RGB2PWMchannel[rgbCh].redPWMChannel,     red);
    ledcWrite(RGB2PWMchannel[rgbCh].redPWMChannel,    red);

    Log.trace("  Set GREEN PWM channel %d to %d \n", RGB2PWMchannel[rgbCh].greenPWMChannel,   green);
    ledcWrite(RGB2PWMchannel[rgbCh].greenPWMChannel,  green);

    Log.trace("  Set BLUE  PWM channel %d to %d \n", RGB2PWMchannel[rgbCh].bluePWMChannel,    blue);
    ledcWrite(RGB2PWMchannel[rgbCh].bluePWMChannel,   blue);

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
                    RGB_WHITE_DIMMED = { 64, 64, 64 };

    const uint8_t turnOn = go.value(Dpt(1,1));
    
    Log.trace("[Callback] turn RGB channel %d %s\n",  rgbCh, turnOn ? "On" : "Off");

    if ( turnOn )
    {    
        DPT_Color_RGB rgb;
        
        if( parameterChannelStartWithLastColor(rgbCh) )
        {
            rgb = lastColor[rgbCh];
            Log.trace("  ON Colour retrieved from Set function R %d G %d B %d\n", rgb.R, rgb.G, rgb.B);
        }
        else
        {
            rgb.R = knx.paramByte(rgbCh*3+1);
            rgb.G = knx.paramByte(rgbCh*3+2);
            rgb.B = knx.paramByte(rgbCh*3+3);

            Log.trace("  ON Colour retrieved from Parameters R %d G %d B %d\n", rgb.R, rgb.G, rgb.B);
        }

        // 'Emergency'-value: if all channels are 0, set to white dimmed - otherwise the LEDs will be off when turning on

        if ( rgb.R == 0 && rgb.G == 0 && rgb.B == 0 )
            rgb = RGB_WHITE_DIMMED;

        setRGBChannelToColor( rgbCh, rgb );
        storeRGBinGO(rgbFeedbackFunction( rgbCh ), rgb, true );
        
    } else {
        getRGBfromGO(rgbFeedbackFunction( rgbCh ), &lastColor[rgbCh]);

        setRGBChannelToColor( rgbCh, RGB_DARK);
        storeRGBinGO(rgbFeedbackFunction( rgbCh ), RGB_DARK, true );
        rgbRunColorFunction( rgbCh ).value(0);
    }

    onOffFeedbackFunction( rgbCh ).value( go.value() );
    knx.loop();
    delay(20);
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
    
    getRGBfromGO(go, &rgb);
    
    Log.trace("[Callback] change color of RGB channel %d  [%x %x %x]\n", rgbCh, rgb.R, rgb.G, rgb.B);

    setRGBChannelToColor( rgbCh, rgb );

    storeRGBinGO(rgbFeedbackFunction( rgbCh ), rgb, true );

    // side-effect of sending color could be the on/off state changes !
    onOffFeedbackFunction( rgbCh ).value( rgb.R != 0 || rgb.G != 0 || rgb.B != 0 );
    knx.loop();
    delay(2);
    
}

/**
 * @brief callback function for group object that handles RGB Run
 * 
 * @param go 
 */
void callbackRGBRun(GroupObject& go)
{
    int rgbCh = goToRGBChannel(go);

    Log.trace("[Callback] RGB run %s for channel %d\n", go.value() ? "ON" : "OFF", rgbCh);
    if( (bool) go.value() != colorFlowRunning[rgbCh] )
    {

        colorFlowRunning[rgbCh] = go.value();

        // when RGB run is activated, make sure to set channel to ON

        if( (bool) go.value() )
        {
            Log.trace("  RGB run ON, set channel ON\n");
            DPT_Color_RGB currentRGB, nextRGB;
            
            // fetch the next color to make sure to set feedback to valid color from the wheel
            // from the feedback group object

            getRGBfromGO(rgbFeedbackFunction(rgbCh), &currentRGB);
            nextRGB = nextColor(currentRGB);
            setRGBChannelToColor(rgbCh, nextRGB);
            storeRGBinGO(rgbFeedbackFunction( rgbCh ), nextRGB, true );

            onOffFeedbackFunction( rgbCh ).value(1);

            knx.loop();
            delay(2);    
        } else
        {
            rgbFeedbackFunction(rgbCh).objectWritten();
            
        }
        
    }

}
