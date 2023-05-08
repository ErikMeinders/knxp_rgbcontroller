#include "knxapp.h"
#include "amps.h"

#ifdef CURRENT_SENSOR_PIN
static double sum = 0.0;
static int count = 0;
static double moving_avg = 0.0;
static int WINDOW_SIZE = 5;

void updateAmps()
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
    Printf("Sensor value: %d | %6.1f mV | %4.1f A | Moving Avg: %4.1f A\n", readValue, mVolt, Amps, moving_avg);

    // in due time report value in GO
    // ampsFeedbackFunction.value(Amps);
}
#endif