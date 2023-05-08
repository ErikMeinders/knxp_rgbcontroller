#include <knxapp.h>

void setAllEnabledChannelsToRGB(DPT_Color_RGB rgb);

void getRGBfromGO(GroupObject& go, DPT_Color_RGB& rgb);

void putRGBinGO(GroupObject& go, DPT_Color_RGB rgb, bool publish);

void putRGBinHW(int rgbCh, DPT_Color_RGB rgbValue);

void nextColor(DPT_Color_RGB currentColor, DPT_Color_RGB &nextColor);

void callbackOnOff(GroupObject& go);

void callbackRGB(GroupObject& go);

void callbackRGBRun(GroupObject& go);

void startColorFlow(int ch);
void stopColorFlow(int ch);

void rgbColorFlowPattern0();
void rgbColorFlowPattern1();

void rgbStatus();