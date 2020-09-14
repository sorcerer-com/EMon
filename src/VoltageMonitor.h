#ifndef VOLTAGE_MONITOR_H
#define VOLTAGE_MONITOR_H

class VoltageMonitorClass
{
private:
    const uint8_t inputPin = 36;          // VP pin
    const uint16_t samplesDuration = 200; // ms // TODO: check which is the best value

public:
    double voltage = 220;

    void update()
    {
        voltage = calcVrms();
        //DEBUGLOG("EnergyMonitor", "Voltage: %f, time: %d", voltage, timer);
    }

private:
    int32_t sampleV;
    double offsetV = 1780; //(double)(2 << 10); // half of the max 12bit number
    double filteredV;
    double sqV, sumV;
    double temp_vrms;

    // https://github.com/openenergymonitor/EmonLib
    double calcVrms()
    {
        uint32_t start = millis();
        uint16_t samplesCount = 0;
        while (millis() - start < samplesDuration)
        {
            sampleV = 0;
            for (int i = 0; i < 100; i++) // fix ADC noise
                sampleV += analogRead(inputPin);
            sampleV /= 100;
            samplesCount++;

            // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset,
            // then subtract this - signal is now centered on 0 counts.
            offsetV = (offsetV + (sampleV - offsetV) / 1024);
            filteredV = sampleV - offsetV;

            // Root-mean-square method current
            // 1) square current values
            sqV = filteredV * filteredV;
            // 2) sum
            sumV += sqV;
            //delay(100); // for sensor calibration
        }

        temp_vrms = sqrt(sumV / samplesCount) * 0.725;// TODO: my sensor; * 0.665; - new sensor with better calibration

        //Reset accumulators
        sumV = 0;

        return temp_vrms;
    }
};

#if !defined(NO_GLOBAL_INSTANCES)
VoltageMonitorClass VoltageMonitor;
#endif

#endif
