#include <knxp_platformio.h>
#include "rgbapp.h"
#include "FS.h"
#include "SPIFFS.h"

DECLARE_TIMER( YourCodeShoutOut, 5 );
DECLARE_TIMERms(colorFlow, 20);

knxapp knxApp;

bool spiffMountedInSetup = false;

#ifdef KNX_NO_AUTOMATIC_GLOBAL_INSTANCE
Esp32Platform knxPlatform(&Serial2);
Bau07B0 knxBau(knxPlatform);
KnxFacade<Esp32Platform, Bau07B0> knx(knxBau);
#endif

// PWM constants

const int freq = 5000;
const int resolution = 8;

DECLARE_TIMER( AmpCycle, 5 );

#ifdef RUN_TEST_PATTERN
DECLARE_TIMER( ColorChange, 5 );
DPT_Color_RGB loopRGB; 
int loopColor = 0;
#endif

/*
* PWM channel is hardware, maps onto GPIO pins on ESP32
* RGB channel is software, maps onto a group object
* RGB2PWM channel is software, maps RGB onto a PWM channel
*/

// physical pins indexed by PWM channel

#ifndef DEVBOARD_LEDS
const uint8_t ledPins[16] = {   33, 25, 26, 
                                27, 14, 12, 
                                13, 23, 22, 
                                21, 19, 18, 
                                17, 16, 15, 
                                0 };  
#else
const uint8_t ledPins[16] = { 17, 16, 15, 27, 14, 12, 13, 23, 22, 21, 19, 18, 0, 0, 0, 0 };  
#endif

void setAllEnabledChannelsToRGB(DPT_Color_RGB rgb)
{
    for(int ch=0 ; ch < maxRGBChannels ; ch++)
    {
        if(parameterChannelEnabled(ch))
            putRGBinHW( ch , rgb );
    }
}
void knxProgledOn() 
{
    DPT_Color_RGB progModeColor = { 0, 0, 128 }; 

    setAllEnabledChannelsToRGB(progModeColor);
}

void knxProgledOff() 
{
    DPT_Color_RGB noProg = { 0, 64, 0 }; 

    setAllEnabledChannelsToRGB(noProg);
}

void knxapp::setup()
{
    // watchdog reset

#ifndef NO_HEARTBEAT
    pinMode(PIN_RESETWATCHDOG, OUTPUT);
    for(int i=0; i<3; i++)
    {
        digitalWrite(PIN_RESETWATCHDOG, LOW);
        delay(100);
        digitalWrite(PIN_RESETWATCHDOG, HIGH);
        delay(100);
    }
#endif

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

    delay(5000); // allow for telnet to connect;

    for(int ch=0 ; ch < maxRGBChannels ; ch++)
    {
        delay(5);

        // setup DPT for all functions, no callback if channel is not enabled

        Log.verbose("Setup RGB channel %d\n", ch);
        
        // function 0 is on/off

        if (parameterChannelEnabled(ch))
            onOffFunction(ch).callback(callbackOnOff);

        onOffFunction(ch).dataPointType(DPT_Switch);
        
        // function 1 is on/off feedback

        onOffFeedbackFunction(ch).dataPointType(DPT_Switch);
        
        // function 2 is RGB set

        if(parameterChannelEnabled(ch))
            rgbSetFunction(ch).callback(callbackRGB);

        rgbSetFunction(ch).dataPointType(DPT_Colour_RGB);

        // function 3 is RGB feedback

        rgbFeedbackFunction(ch).dataPointType(DPT_Colour_RGB);

        // function 4 is RGB Color Flow

        if(parameterChannelEnabled(ch))
            rgbColorFlowFunction(ch).callback(callbackRGBRun);

        rgbColorFlowFunction(ch).dataPointType(DPT_Switch);

    }
    
     // process ETS parameters
    
    setCyclicTimer(60*knx.paramByte(0));
    setGroupObjectCount( maxFunctions * maxRGBChannels );

    spiffMountedInSetup = SPIFFS.begin();

    if(spiffMountedInSetup)
    {
        Log.notice("SPIFFS mounted in setup\n");
    } else {
        Log.error("SPIFFS mount in setup failed\n");
    }

    Log.setLevel(4);
}

void knxapp::loop()
{

    // _knxapp::loop();
   
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
          putRGBinHW(0, loopRGB);  
          putRGBinHW(1, loopRGB);
          putRGBinHW(2, loopRGB);
          putRGBinHW(3, loopRGB);
          putRGBinHW(4, loopRGB); 
       }
#else
    // check if any of the RGB channels is in color flow mode

    if( DUE( colorFlow )) 
    {
        for(int ch=0 ; ch < maxRGBChannels ; ch++)
        {
            if( rgbColorFlowFunction(ch).value() && parameterChannelEnabled(ch))
            {
                DPT_Color_RGB currentRGB, nextRGB;
                
                getRGBfromGO(rgbFeedbackFunction(ch), currentRGB);
                nextColor(currentRGB, nextRGB);
                putRGBinHW(ch, nextRGB);
                putRGBinGO(rgbFeedbackFunction( ch ), nextRGB, false );

            }
        }
    }

#endif

    static double sum = 0.0;
    static int count = 0;
    static double moving_avg = 0.0;
    static int WINDOW_SIZE = 5;

#ifdef CURRENT_SENSOR_PIN
    if(DUE(AmpCycle) && false)
    {
        double mVolt = 0.0;
        double Amps = 0.0;
        // analogSetCycles(100);
        analogSetPinAttenuation(CURRENT_SENSOR_PIN, ADC_11db);
        adcAttachPin(CURRENT_SENSOR_PIN);
        int readValue = analogRead(CURRENT_SENSOR_PIN); // 0 - 4095
        mVolt = (readValue / 4096.0) * 3300.0;
        mVolt = mVolt - 1300.0;
        Amps = mVolt / 65 ; // 65 = 1300 / 20
        
        if(count >= WINDOW_SIZE)
        {
            sum = moving_avg * (WINDOW_SIZE-1) + Amps;
            moving_avg = sum /  WINDOW_SIZE;
        } else {
            sum += Amps;
            count++;

            moving_avg = sum / count;
        }
        Printf("Sensor value: %d | %6.1f mV | %4.1f A | Moving Avg: %4.1f A\r", readValue, mVolt, Amps, moving_avg);

    }
#endif

    
}

void knxapp::status()
{
    _knxapp::status();

    rgbStatus();
}

