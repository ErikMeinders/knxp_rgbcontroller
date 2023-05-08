#include <knxp_platformio.h>
#include "rgbapp.h"
#include "amps.h"
#include "FS.h"
#include "SPIFFS.h"

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

#ifdef CURRENT_SENSOR_PIN
DECLARE_TIMER( AmpCycle, 30 );
#endif

#ifdef RUN_TEST_PATTERN
DECLARE_TIMER( ColorChange, 5 );
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


/**
 * @brief callback for programming mode LED feedback (ON)
 * 
 */
void knxProgledOn() 
{
    DPT_Color_RGB progModeColor = { 0, 0, 128 }; 

    setAllEnabledChannelsToRGB(progModeColor);
}

/**
 * @brief callback for programming mode LED feedback (OFF)
 * 
 */
void knxProgledOff() 
{
    DPT_Color_RGB noProg = { 0, 64, 0 }; 

    setAllEnabledChannelsToRGB(noProg);
}

/**
 * @brief override of the _knxapp::setup() method
 * 
 */
void knxapp::setup()
{
    // watchdog reset

#ifndef NO_HEARTBEAT
    pinMode(PIN_RESETWATCHDOG, OUTPUT);
    for(int i=0; i<3; i++)
    {
        digitalWrite(PIN_RESETWATCHDOG, LOW);
        delay(30);
        digitalWrite(PIN_RESETWATCHDOG, HIGH);
        delay(30);
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

#ifdef TELNET_CONNECT_DELAY
    delay(TELNET_CONNECT_DELAY); // allow for telnet to connect;
#endif

    // attach the callback functions to the group objects and set the DPT

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

    knx.setProgLedOffCallback(knxProgledOff);
    knx.setProgLedOnCallback(knxProgledOn);

#ifndef TELNET_CONNECT_DELAY
    Log.setLevel(2);
#endif

}

/**
 * @brief override of the _knxapp::loop() method
 * 
 */
void knxapp::loop()
{

#ifndef RUN_TEST_PATTERN

    if(DUE( colorFlow )) 
        rgbColorFlowPattern1();

#else
    
    if( DUE( ColorChange ) )
        rgbColorFlowPattern0();
        
#endif

#ifdef CURRENT_SENSOR_PIN

    if(DUE(AmpCycle) )
        updateAmps();

#endif

    
}

/**
 * @brief override of the _knxapp::status() method
 * 
 */
void knxapp::status()
{
    _knxapp::status();

    rgbStatus();
}

