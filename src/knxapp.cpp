#include <knxp_platformio.h>
#include "rgbapp.h"
DECLARE_TIMER( YourCodeShoutOut, 5 );
DECLARE_TIMERms(colorFlow, 20);

knxapp knxApp;

const int freq = 5000;
const int resolution = 8;

DECLARE_TIMER( AmpCycle, 1 );
DECLARE_TIMER( knxRestart, 60 );

/*
* PWM channel is hardware, maps onto GPIO pins on ESP32
* RGB channel is software, maps onto a group object
* RGB2PWM channel is software, maps RGB onto a PWM channel
*/

// physical pins indexed by PWM channel

const uint8_t ledPins[16] = { 33, 25, 26, 27, 14, 12, 13, 23, 22, 21, 19, 18, 17, 16, 15, 0 }; // FOR ESP32 DEV BOARD

void knxProgledOn() 
{

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

void knxProgledOff() 
{
    
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

    setCyclicTimer(60*knx.paramByte(0));
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
        // function 0 is on/off
        onOffFunction(ch).callback(callbackOnOff);
        onOffFunction(ch).dataPointType(DPT_Switch);

        // function 1 is on/off feedback
        onOffFeedbackFunction(ch).dataPointType(DPT_Switch);
        onOffFeedbackFunction(ch).value(false);

        // function 2 is RGB set
        rgbSetFunction(ch).callback(callbackRGB);
        rgbSetFunction(ch).dataPointType(DPT_Colour_RGB);

        // function 3 is RGB feedback
        rgbFeedbackFunction(ch).dataPointType(DPT_Colour_RGB);

        // function 4 is RGB run
        rgbRunColorFunction(ch).callback(callbackRGBRun);
        rgbRunColorFunction(ch).dataPointType(DPT_Switch);

    }
    knx.setProgLedOnCallback(knxProgledOn);
    knx.setProgLedOffCallback(knxProgledOff);

    Log.setLevel(LOG_LEVEL_WARNING);
}

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
    // check if any of the RGB channels is in color flow mode

    if( DUE( colorFlow ))
    {
        for(int ch=0 ; ch < maxRGBChannels ; ch++)
        {
            if( rgbRunColorFunction(ch).value() )
            {
                DPT_Color_RGB currentRGB, nextRGB;
                
                getRGBfromGO(rgbFeedbackFunction(ch), &currentRGB);
                nextRGB = nextColor(currentRGB);
                setRGBChannelToColor(ch, nextRGB);
                storeRGBinGO(rgbFeedbackFunction( ch ), nextRGB, false );

            }
        }
    }

#endif

    static double sum = 0.0;
    static int count = 0;
    double moving_avg = 0.0;
    static int WINDOW_SIZE = 5;

    if(DUE(AmpCycle))
    {
        double mVolt = 0.0;
        double Amps = 0.0;
        // analogSetCycles(100);
        analogSetPinAttenuation(39, ADC_11db);
        adcAttachPin(39);
        int readValue = analogRead(39); // 0 - 4095
        mVolt = (readValue / 4096.0) * 3300.0;
        mVolt = mVolt - 1300.0;
        Amps = mVolt / 65 ; // 65 = 1300 / 20
        
        sum += Amps;
        count++;
        if(count > WINDOW_SIZE)
        {
            sum = moving_avg * (WINDOW_SIZE-1) + Amps;
            moving_avg = (double) (sum / (double) WINDOW_SIZE);
        }
        else
        {
            moving_avg = (double) (sum / (double) count);
            Printf(" moving_avg {%d} = %6.1f Amp.\r", count, moving_avg);
        }
        Printf("Sensor value: %d | %6.1f mV | %6.1f Amp. | Avg {5} %6.1f Amp.\r", readValue, mVolt, Amps, moving_avg);

    }
 
    if(DUE(knxRestart))
    {
        knx.restart(knx.individualAddress());
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

