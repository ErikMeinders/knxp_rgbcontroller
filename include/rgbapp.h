#include <knxapp.h>


void getRGBfromGO(GroupObject& go, DPT_Color_RGB *rgb);

void storeRGBinGO(GroupObject& go, DPT_Color_RGB rgb, bool publish);

void setRGBChannelToColor(int rgbCh, DPT_Color_RGB rgbValue);

DPT_Color_RGB nextColor(DPT_Color_RGB currentColor);

void callbackOnOff(GroupObject& go);

void callbackRGB(GroupObject& go);


void callbackRGBRun(GroupObject& go);