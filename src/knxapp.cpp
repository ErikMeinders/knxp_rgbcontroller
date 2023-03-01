#include <knxp_platformio.h>

DECLARE_TIMER( YourCodeShoutOut, 5 );

knxapp knxApp;



const int freq = 5000;
const int resolution = 8;

/*
* PWM channel is hardware, maps onto GPIO pins on ESP32
* RGB channel is software, maps onto a group object
* RGB2PWM channel is software, maps RGB onto a PWM channel
*/


// physical pins indexed by PWM channel

// const uint8_t ledPins[16] = { 2, 4, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36 };
const uint8_t ledPins[16] = { 99, 99, LED_BUILTIN, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99 }; // FOR NOW

// RGB channels indexed by RGB channel to find 3 PWM channels

const RGBChannel RGB2PWMchannel[maxRGBChannels] = { {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9, 10, 11}, {12, 13, 14} };

void knxapp::setup()
{

    setCyclicTimer(6*knx.paramByte(0));
    setGroupObjectCount( maxFunctions * maxRGBChannels );

    // setup all PWM channels with the same frequency and resolution

    for( int pwmChannel=0; pwmChannel<16; pwmChannel++ )
    {
        Log.trace("Setup PWM channel %d ", pwmChannel);
        ledcSetup(pwmChannel, freq, resolution);
        Log.trace("\n");
    }

    // attach the PWMchannels to the GPIO to be controlled
    for( int pwmChannel=0; pwmChannel<16; pwmChannel++ )
    {
        Log.trace("Attach PWM channel %d to GPIO %d ", pwmChannel, ledPins[pwmChannel]);
        ledcAttachPin(ledPins[pwmChannel], pwmChannel);
        Log.trace("\n");
    }

    // attach the callback functions to the group objects and set the DPT
    for(int ch=0 ; ch < maxRGBChannels ; ch++)
    {
        onOffFunction(ch).callback(callbackOnOff);
        onOffFunction(ch).dataPointType(DPT_Switch);

        onOffFeedbackFunction(ch).dataPointType(DPT_Switch);
        onOffFeedbackFunction(ch).value(false);

        rgbSetFunction(ch).callback(callbackRGB);
        rgbSetFunction(ch).dataPointType(DPT_Colour_RGB);

        rgbFeedbackFunction(ch).dataPointType(DPT_Colour_RGB);

    }
    knx.ledPin(30); // TEMPORARY SETTING !!!
    
}

void knxapp::loop()
{
    
}

void knxapp::status()
{
    _knxapp::status();

    for(int ch=0 ; ch < maxRGBChannels ; ch++)
    {
        GroupObject& go = rgbFeedbackFunction(ch);
        DPT_Color_RGB rgb; 
        bool on = onOffFeedbackFunction(ch).value();

        getRGBfromGO(go, &rgb);
        Printf("Channel %d is %s color %02x %02x %02x\n",  ch, on ? "ON " : "Off", rgb.R, rgb.G, rgb.B);
    }
}

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

    Log.trace("Try to retrieve RGB in %d bytes from GO# %d ", go.valueSize(), go.asap());

    rgb->R = rgbValue->R;
    rgb->G = rgbValue->G;
    rgb->B = rgbValue->B;

    Log.trace("  RGB %d %d %d\n", rgb->R, rgb->G, rgb->B);
}

/**
 * @brief Store RGB value in group object
 * 
 * @param go 
 * @param rgb 
 */
void storeRGBinGO(GroupObject& go, DPT_Color_RGB rgb)
{
    DPT_Color_RGB *  rgbValue = (DPT_Color_RGB *) go.valueRef();

    Log.trace("Try to store RGB in %d bytes in GO# %d\n", go.valueSize(), go.asap());

    Log.trace("  RGB IN %d %d %d\n", rgb.R, rgb.G, rgb.B);
    rgbValue->R = rgb.R;
    rgbValue->G = rgb.G;
    rgbValue->B = rgb.B;
    go.objectWritten();
    knx.loop();
    delay(20);

    DPT_Color_RGB rgb2;
    getRGBfromGO(go, &rgb2);
    Log.trace("  RGB RT %d %d %d\n", rgb2.R, rgb2.G, rgb2.B);
 
}

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

/**
 * @brief callback function for RGB group object that handles On/Off 
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
            getRGBfromGO(rgbFeedbackFunction( rgbCh ), &rgb);
            Log.trace("  Colour retrieved from last known Feedback R %d G %d B %d\n", rgb.R, rgb.G, rgb.B);
        }
        else
        {
            rgb.R = knx.paramByte(rgbCh*3+1);
            rgb.G = knx.paramByte(rgbCh*3+2);
            rgb.B = knx.paramByte(rgbCh*3+3);

            Log.trace("  Colour retrieved from Parameters R %d G %d B %d\n", rgb.R, rgb.G, rgb.B);
        }

        // 'Emergency'-value: if all channels are 0, set to white dimmed - otherwise the LEDs will be off when turning on

        if ( rgb.R == 0 && rgb.G == 0 && rgb.B == 0 )
            rgb = RGB_WHITE_DIMMED;

        setRGBChannelToColor( rgbCh, rgb );
        storeRGBinGO(rgbFeedbackFunction( rgbCh ), rgb );
        
    } else 
        setRGBChannelToColor( rgbCh, RGB_DARK);

    onOffFeedbackFunction( rgbCh ).value( go.value() );
    knx.loop();
    delay(20);
}

/**
 * @brief callback function for RGB group object that handles RGB
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

    storeRGBinGO(rgbFeedbackFunction( rgbCh ), rgb );

    // side-effect of sending color could be the on/off state changes !
    onOffFeedbackFunction( rgbCh ).value( rgb.R != 0 || rgb.G != 0 || rgb.B != 0 );
    knx.loop();
    delay(2);
    
}