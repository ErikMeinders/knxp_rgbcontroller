#ifndef KNXAPP_H
#define KNXAPP_H

#include <knxp_platformio.h>

// your application override of the _knxapp class methods

class knxapp : public _knxapp
{
  public:
    // void progress(int step, const char *msg);  // callback for knxp_platformio

    // void  pinsetup();    // before anything else
    // void  conf();        // after Wifi, NTP and Telnet

    void setup() ;          // after KNX configuration before KNX start  
    void loop();            // application loop

    // char* hostname();    // callback by knxp_platformio | default: HOSTNAME / "knx_device"
    void  status();      // callback for additonal status information Menu item 'S'

    // void setRGBChannelToColor(int rgbch, const char* value);
    

};

extern knxapp knxApp;

#define maxRGBChannels            5
#define maxFunctions              4

#define onOffFunctionNr           0
#define onOffFeedbackFunctionNr   1
#define rgbSetFunctionNr          2
#define rgbFeedbackFunctionNr     3

#define onOffFunction(ch)         knx.getGroupObject(ch+maxRGBChannels*onOffFunctionNr+1)
#define onOffFeedbackFunction(ch) knx.getGroupObject(ch+maxRGBChannels*onOffFeedbackFunctionNr+1)
#define rgbSetFunction(ch)        knx.getGroupObject(ch+maxRGBChannels*rgbSetFunctionNr+1)
#define rgbFeedbackFunction(ch)   knx.getGroupObject(ch+maxRGBChannels*rgbFeedbackFunctionNr+1)

#define goToRGBChannel(go)        ((go.asap()-1) % maxRGBChannels )

#define parameterChannelStartWithLastColor(n) !(knx.paramByte(17) & (1 << n))

#define parameterChannelStartColor(n) knx.paramData(n * 3 + 1)

/**
 * @brief struct holding the RGB values as Data Point Type 232.600
 * 
 */
typedef struct _dptcrgb {
    uint8_t  R,
             G,
             B;
} DPT_Color_RGB;

/**
 * @brief struct holding the 3 PWM channels that make up one RGB channel
 * 
 */
typedef struct _RGBChannel{
    uint8_t   redPWMChannel;
    uint8_t greenPWMChannel;
    uint8_t  bluePWMChannel;
} RGBChannel;

void getRGBfromGO(GroupObject& go, DPT_Color_RGB* rgb );
void storeRGBinGO(GroupObject& go, DPT_Color_RGB rgb);
void setRGBChannelToColor(int rgbCh, DPT_Color_RGB rgbValue);


void callbackOnOff(GroupObject& go);
void callbackRGB(GroupObject& go);

#endif
