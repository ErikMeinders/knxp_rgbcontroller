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

    void setRGBChannelToColor(int rgbch, const char* value);
    

};

extern knxapp knxApp;

#define maxChannels               5
#define maxFunctions              4

#define onOffFunctionNr           0
#define onOffFeedbackFunctionNr   1
#define rgbSetFunctionNr          2
#define rgbFeedbackFunctionNr     3

#define onOffFunction(ch)         knx.getGroupObject(ch+maxChannels*onOffFunctionNr+1)
#define onOffFeedbackFunction(ch) knx.getGroupObject(ch+maxChannels*onOffFeedbackFunctionNr+1)
#define rgbSetFunction(ch)        knx.getGroupObject(ch+maxChannels*rgbSetFunctionNr+1)
#define rgbFeedbackFunction(ch)   knx.getGroupObject(ch+maxChannels*rgbFeedbackFunctionNr+1)

#define goToRGBChannel(go)        ((go.asap()-1) % maxChannels )

#define parameterChannelStartWithLastColor(n) 1

#define parameterChannelStartColor(n) knx.paramData(n * 3 + 1)

void callbackOnOff(GroupObject& go);
void callbackRGB(GroupObject& go);


typedef struct _dptcrgb {
    uint8_t R, G, B;
} DPT_Color_RGB;

#endif
