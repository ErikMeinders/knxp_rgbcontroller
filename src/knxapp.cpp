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


// physical pins mapped to led channels

// const uint8_t ledPins[16] = { 2, 4, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36 };
const uint8_t ledPins[16] = { 99, 99, LED_BUILTIN, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99 }; // FOR NOW

// RGB channels

typedef struct _RGBChannel{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGBChannel;

const RGBChannel RGB2PWMchannel[maxChannels] = { {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9, 10, 11}, {12, 13, 14} };

void knxapp::setup()
{

    setCyclicTimer(6*knx.paramByte(0));
    setGroupObjectCount( maxFunctions * maxChannels );

    // setup all PWM channels with the same frequency and resolution

    for( int pwmChannel=0; pwmChannel<16; pwmChannel++ )
    {
        Log.trace("Setup PWM channel %d ", pwmChannel);
        ledcSetup(pwmChannel, freq, resolution);
        Log.trace("\n");
    }

    // attach the channel to the GPIO to be controlled
    for( int pwmChannel=0; pwmChannel<16; pwmChannel++ )
    {
        Log.trace("Attach PWM channel %d to GPIO %d ", pwmChannel, ledPins[pwmChannel]);
        ledcAttachPin(ledPins[pwmChannel], pwmChannel);
        Log.trace("\n");
    }

    // attach the callback functions to the group objects and set the DPT
    for(int ch=0 ; ch < maxChannels ; ch++)
    {
        onOffFunction(ch).callback(callbackOnOff);
        onOffFunction(ch).dataPointType(DPT_Switch);

        onOffFeedbackFunction(ch).dataPointType(DPT_Switch);

        rgbSetFunction(ch).callback(callbackRGB);
        rgbSetFunction(ch).dataPointType(DPT_Colour_RGB);

        rgbFeedbackFunction(ch).dataPointType(DPT_Colour_RGB);

    }
    knx.ledPin(30); // TEMPORARY SETTING !!!
    
}

unsigned long xx=0;

void knxapp::loop()
{
    
}

void knxapp::status()
{
    _knxapp::status();

    for(int ch=0 ; ch < maxChannels ; ch++)
    {
        uint8_t * rgb = rgbFeedbackFunction(ch).valueRef();
        bool on = onOffFeedbackFunction(ch).value();
        Printf("Channel %d is %s color %02x %02x %02x\n",  ch, on ? "ON " : "Off", rgb[0], rgb[1], rgb[2]);
    }
}

// helper functions for KNX RGB Data Type

/**
 * @brief Get RGB channel from group object
 * 
 * @param go 
 * @return DPT_Color_RGB 
 */
DPT_Color_RGB getRGBfromGO(GroupObject& go)
{
    DPT_Color_RGB rgb;
    uint8_t * rgbValue = go.valueRef();
    rgb.R = rgbValue[0];
    rgb.G = rgbValue[1];
    rgb.B = rgbValue[2];
    return rgb;
}

/**
 * @brief Store RGB value in group object
 * 
 * @param go 
 * @param rgb 
 */
void storeRGBinGO(GroupObject& go, DPT_Color_RGB rgb)
{
    uint8_t * rgbValue = go.valueRef();
    rgbValue[0] = rgb.R;
    rgbValue[1] = rgb.G;
    rgbValue[2] = rgb.B;
    knx.loop();
    delay(2);
    
}

/**
 * @brief Set 3 PWM channels to RGB value
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

    Log.trace("  Set RED   PWM channel %d to %d \n", RGB2PWMchannel[rgbCh].red,     red);
    ledcWrite(RGB2PWMchannel[rgbCh].red,    red);

    Log.trace("  Set GREEN PWM channel %d to %d \n", RGB2PWMchannel[rgbCh].green,   green);
    ledcWrite(RGB2PWMchannel[rgbCh].green,  green);

    Log.trace("  Set BLUE  PWM channel %d to %d \n", RGB2PWMchannel[rgbCh].blue,    blue);
    ledcWrite(RGB2PWMchannel[rgbCh].blue,   blue);

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
            rgb = getRGBfromGO(rgbFeedbackFunction( rgbCh ));
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
        rgbFeedbackFunction( rgbCh ).value( rgbFeedbackFunction( rgbCh ).value() ); // so that the feedback is sent
        
    } else 
        setRGBChannelToColor( rgbCh, RGB_DARK);

    onOffFeedbackFunction( rgbCh ).value( go.value() );
    knx.loop();
}

/**
 * @brief callback function for RGB group object that handles RGB
 * 
 * @param go 
 */
void callbackRGB(GroupObject& go)
{
    int rgbCh = goToRGBChannel(go);
    DPT_Color_RGB rgb =  getRGBfromGO(go);
    
    Log.trace("[Callback] change color of RGB channel %d  [%x %x %x]\n", rgbCh, rgb.R, rgb.G, rgb.B);

    setRGBChannelToColor( rgbCh, rgb );

    storeRGBinGO(rgbFeedbackFunction( rgbCh ), rgb );
    rgbFeedbackFunction( rgbCh ).value( rgbFeedbackFunction( rgbCh ).value() ); // so that the feedback is sent

    
    onOffFeedbackFunction( rgbCh ).value( rgb.R != 0 || rgb.G != 0 || rgb.B != 0 );
    knx.loop();
    delay(2);
    
}