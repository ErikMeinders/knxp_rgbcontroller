#include <knxp_platformio.h>

DECLARE_TIMER( YourCodeShoutOut, 5 );

knxapp knxApp;

DPT_Color_RGB lastColor[maxRGBChannels];

const int freq = 5000;
const int resolution = 8;

DECLARE_TIMER( AmpCycle, 1 );

/*
* PWM channel is hardware, maps onto GPIO pins on ESP32
* RGB channel is software, maps onto a group object
* RGB2PWM channel is software, maps RGB onto a PWM channel
*/


// physical pins indexed by PWM channel

// const uint8_t ledPins[16] = { 2, 4, 5, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36 };
const uint8_t ledPins[16] = { 33, 25, 26, 27, 14, 12, 13, 23, 22, 21, 19, 18, 17, 16, 15, 0 }; // FOR ESP32 DEV BOARD

// RGB channels indexed by RGB channel to find 3 PWM channels

const RGBChannel RGB2PWMchannel[maxRGBChannels] = { {0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {9, 10, 11}, {12, 13, 14} };

void ledOn() {

    DPT_Color_RGB loopRGB; 
    loopRGB.R = 0;
    loopRGB.G = 0;
    loopRGB.B = 128;

    setRGBChannelToColor(0, loopRGB);
    setRGBChannelToColor(1, loopRGB);
    setRGBChannelToColor(2, loopRGB);
    setRGBChannelToColor(3, loopRGB);
    setRGBChannelToColor(4, loopRGB);
}

void ledOff() {
    
    DPT_Color_RGB loopRGB; 
    loopRGB.R = 0;
    loopRGB.G = 0;
    loopRGB.B = 0;

    setRGBChannelToColor(0, loopRGB);
    setRGBChannelToColor(1, loopRGB);
    setRGBChannelToColor(2, loopRGB);
    setRGBChannelToColor(3, loopRGB);
    setRGBChannelToColor(4, loopRGB);
}

void knxapp::setup()
{
    // watchdog reset

    pinMode(0, OUTPUT);
    for(int i=0; i<3; i++)
    {
        digitalWrite(0, LOW);
        delay(100);
        digitalWrite(0, HIGH);
        delay(100);
    }

    // analogSetPinAttenuation(39, ADC_2_5db);

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
        if (ledPins[pwmChannel])
        {
            Log.trace("Attach PWM channel %d to GPIO %d ", pwmChannel, ledPins[pwmChannel]);
            ledcAttachPin(ledPins[pwmChannel], pwmChannel);
            Log.trace("\n");
        }
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
        rgbRunColorFunction(ch).dataPointType(DPT_Switch);

    }
    // knx.ledPin(30); // TEMPORARY SETTING when using LED_BUILDIN for PWM tests !!!
    knx.setProgLedOnCallback(ledOn);
    knx.setProgLedOffCallback(ledOff);

    Log.setLevel(LOG_LEVEL_WARNING);
}

#ifdef RUN_TEST_PATTERN
DECLARE_TIMER( ColorChange, 5 );
DPT_Color_RGB loopRGB; 
int loopColor = 0;
#endif

DPT_Color_RGB nextColor(DPT_Color_RGB colorRGB)
{
    DPT_Color_RGB nextRGB;

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

void knxapp::loop()
{

#ifdef RUN_TEST_PATTERN
       if( DUE( ColorChange ) )
       {
          loopColor = (loopColor + 1) % 5;
          switch(loopColor)
          {
            case 0:
                loopRGB.R = 0;
                loopRGB.G = 0;
                loopRGB.B = 0;
                break;
            case 1:
                loopRGB.R = 255;
                loopRGB.G = 0;
                loopRGB.B = 0;
                break;
            case 2:
                loopRGB.R = 0;
                loopRGB.G = 255;
                loopRGB.B = 0;
                break;
            case 3:
                loopRGB.R = 0;
                loopRGB.G = 0;
                loopRGB.B = 255;
                break;
            case 4:
                loopRGB.R = 255;
                loopRGB.G = 255;
                loopRGB.B = 255;
                break;
          }
          setRGBChannelToColor(0, loopRGB);  
          setRGBChannelToColor(1, loopRGB);
          setRGBChannelToColor(2, loopRGB);
          setRGBChannelToColor(3, loopRGB);
          setRGBChannelToColor(4, loopRGB); 
       }
#else
    // check if any of the RGB channels should be changed
    for(int ch=0 ; ch < maxRGBChannels ; ch++)
    {
        knx.loop();
        if( rgbRunColorFunction(ch).value() )
        {
            DPT_Color_RGB currentRGB, nextRGB;
            
            getRGBfromGO(rgbFeedbackFunction(ch), &currentRGB);
            nextRGB = nextColor(currentRGB);
            setRGBChannelToColor(ch, nextRGB);
            storeRGBinGO(rgbFeedbackFunction( ch ), nextRGB, false );

        }
        delay(1);
    }
#endif

    if(DUE(AmpCycle))
    {
        double Voltage = 0;
        double VRMS = 0;
        double AmpsRMS = 0;
        int Watt = 0;
        int mVperAmp = 100;
        // analogSetCycles(100);
        analogSetPinAttenuation(39, ADC_11db);
        adcAttachPin(39);
        int readValue = analogRead(39);
        Voltage = readValue * 3.3 / 4096.0;

        Printf("Sensor value: %d, voltage: %f -- ", readValue, Voltage);

        VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
        AmpsRMS = ((VRMS * 1000)/mVperAmp)-0.3; //0.3 is the error I got for my sensor
    
        Printf(" %f Amps RMS  ---  ", AmpsRMS);
        Watt = (AmpsRMS*240/1.2);
        // note: 1.2 is my own empirically established calibration factor
        // as the voltage measured at D34 depends on the length of the OUT-to-D34 wire
        // 240 is the main AC power voltage – this parameter changes locally
        Printf(" %d Watts\n", Watt);
    }
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

    Log.trace("Retrieve RGB in %d bytes from GO# %d ", go.valueSize(), go.asap());

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
void storeRGBinGO(GroupObject& go, DPT_Color_RGB rgb, bool publish)
{
    DPT_Color_RGB *  rgbValue = (DPT_Color_RGB *) go.valueRef();

    Log.trace("Store RGB in %d bytes in GO# %d\n", go.valueSize(), go.asap());

    Log.trace("  RGB IN %d %d %d\n", rgb.R, rgb.G, rgb.B);
    rgbValue->R = rgb.R;
    rgbValue->G = rgb.G;
    rgbValue->B = rgb.B;

    if (publish) go.objectWritten();
    knx.loop();
    delay(20);

    // compare!
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

    storeRGBinGO(rgbFeedbackFunction( rgbCh ), rgb, true );

    // side-effect of sending color could be the on/off state changes !
    onOffFeedbackFunction( rgbCh ).value( rgb.R != 0 || rgb.G != 0 || rgb.B != 0 );
    knx.loop();
    delay(2);
    
}