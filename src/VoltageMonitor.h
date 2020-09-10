#ifndef VOLTAGE_MONITOR_H
#define VOLTAGE_MONITOR_H

class VoltageMonitorClass
{
private:
    const uint8_t inputPin = 27;

    const uint16_t samplesCount = 1480;
    const uint8_t batchesCount = 148;
    uint8_t batchIdx = 0;

public:
    double voltage = 220;

    void update()
    {
        // TODO: revise need of batching
        // to align with EnergyMonitor
        batchIdx++;
        if (batchIdx < batchesCount)
            return;

        voltage = calcVrms(samplesCount * 4);
        //DEBUGLOG("EnergyMonitor", "Voltage: %f, time: %d", voltage, timer);
        batchIdx = 0;
    }

private:
    int16_t sampleV;
    double offsetV = (double)(2 << 13) * 3.3 / 4.096; // half signed 16bit * (3.3V / 4.096V (ADS))
    double filteredV;
    double sqV, sumV;
    double temp_vrms;

    // https://github.com/openenergymonitor/EmonLib
    double calcVrms(uint16_t samplesCount)
    {
        /* Be sure to update this value based on the IC and the gain settings! */
        float multiplier = 0.125F; /* ADS1115 @ +/- 4.096V gain (16-bit results) */
        for (uint16_t i = 0; i < samplesCount; i++)
        {
            sampleV = analogRead(inputPin);

            // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset,
            // then subtract this - signal is now centered on 0 counts.
            offsetV = (offsetV + (sampleV - offsetV) / 1024);
            filteredV = sampleV - offsetV;
            //filteredV = sampleV * multiplier;

            // Root-mean-square method current
            // 1) square current values
            sqV = filteredV * filteredV;
            // 2) sum
            sumV += sqV;
        }

        temp_vrms = sqrt(sumV / samplesCount) * multiplier * 21.5;

        //Reset accumulators
        sumV = 0;

        return temp_vrms;
    }
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EEPROM)
VoltageMonitorClass VoltageMonitor;
#endif

#endif
