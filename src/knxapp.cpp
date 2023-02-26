#include <knxp_platformio.h>

DECLARE_TIMER( YourCodeShoutOut, 5 );

knxapp knxApp;

unsigned long xx=0;

void knxapp::loop()
{
    if (DUE( YourCodeShoutOut ))
    {
        Log.verbose("Log Your loop %l\n", xx);
        xx=0;
    }
    xx++;
    if(xx % 1000 == 0)
    {
        Log.verbose("Log Your loop %l\n", xx);
    }
}

const int freq = 5000;
const int resolution = 8;

/*
* PWM channel is hardware, maps onto GPIO pins on ESP32
* RGB channel is software, maps onto a group object
* RGB2PWM channel is software, maps RGB onto a PWM channel
*/


// physical pins mapped to led channels

const uint8_t ledPins[16] = { 2, 4, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36 };

// RGB channels

typedef struct _RGBChannel{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGBChannel;

const RGBChannel RGB2PWMchannel[maxChannels] = { {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9, 10, 11}, {12, 13, 14} };

void knxapp::setup()
{

    setCyclicTimer(60*knx.paramByte(0));
    setGroupObjectCount( maxFunctions * maxChannels );

    // setup all PWM channels with the same frequency and resolution

    for( int pwmChannel=0; pwmChannel<16; pwmChannel++ )
    {
          ledcSetup(pwmChannel, freq, resolution);
    }

    // attach the channel to the GPIO to be controlled
    for( int pwmChannel=0; pwmChannel<16; pwmChannel++ )
    {
          ledcAttachPin(ledPins[pwmChannel], pwmChannel);
    }

    // attach the callback functions to the group objects and set the DPT

    for(int ch=0 ; ch < maxChannels ; ch++)
    {
        onOffFunction(ch).dataPointType(DPT_Switch);
        onOffFeedbackFunction(ch).dataPointType(DPT_Switch);
        rgbSetFunction(ch).dataPointType(DPT_Colour_RGB);
        rgbFeedbackFunction(ch).dataPointType(DPT_Colour_RGB);

        onOffFunction(ch).callback(callbackOnOff);
        rgbSetFunction(ch).callback(callbackRGB);

    }

}

void setRGBChannelToColor(int rgbch, const char* value)
{
    // split value into RGB components

    int red = value[0];
    int green = value[1];
    int blue = value[2];

    ledcWrite(RGB2PWMchannel[rgbch].red, red);
    ledcWrite(RGB2PWMchannel[rgbch].green, green);
    ledcWrite(RGB2PWMchannel[rgbch].blue, blue);

}

void callbackOnOff(GroupObject& go)
{
    int rgbch = goToRGBChannel(go);

    if ( (uint8_t) go.value() == 0)
        setRGBChannelToColor( rgbch, "\0\0\0" );
    else
        setRGBChannelToColor( rgbch, (const char*) parameterChannelStartColor(rgbch) );

    onOffFeedbackFunction( rgbch ).value( go.value() );
}

void callbackRGB(GroupObject& go)
{
    // find the rgb channel for this GO

    int rgbch = goToRGBChannel(go);

    setRGBChannelToColor( rgbch,  go.value() );

    // find feedback GO and set feedback value

    rgbFeedbackFunction( rgbch ).value( go.value() );

    // save new color setting as default if so desired

    int purple = 0xFF00FF00;

    if( parameterChannelStartWithLastColor(rgbch) ){
        memcpy(parameterChannelStartColor(rgbch), & purple, 3);
    }
}

void knxapp::status()
{
    _knxapp::status();

    for(int ch=0 ; ch < maxChannels ; ch++)
    {
        int rgb = rgbFeedbackFunction(ch).value();
        bool on = onOffFeedbackFunction(ch).value();
        Printf("Channel %d is %s color %06x\n",  ch, on ? "ON" : "Off", rgb);
    }
}


