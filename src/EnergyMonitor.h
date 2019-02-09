#ifndef MY_EMON_H
#define MY_EMON_H

#include "ADS1015.h"

class EnergyMonitor
{
  private:
    ADS1115 &ads;
    uint8_t channel = 0;

    const uint16_t samplesCount = 1480;
    const uint8_t batchesCount = 148;
    uint8_t batchIdx = 0;
    double irms = 0.0;

    uint16_t counter = 0;
    uint32_t timer = 0;
    uint64_t power = 0;

  public:
    const uint8_t voltage = 230; // V
    double current = 0.0;

    EnergyMonitor(ADS1115 &_ads, uint8_t _channel) : ads(_ads)
    {
        channel = _channel;
        timer = millis();
    }

    void update()
    {
        irms += calcIrms(samplesCount / batchesCount);
        batchIdx++;
        current = (current + irms / batchIdx) / 2;
        if (batchIdx < batchesCount)
            return;
        irms /= batchesCount;
        current = irms;
        if (irms < 0.1) // noise
            irms = 0.0;
        power += round(irms * voltage);
        counter++;
        //DEBUGLOG("EnergyMonitor", "Channel: %d, Irms: %f, power: %d in %d counts",
        //         channel, irms, power, counter);
        irms = 0;
        batchIdx = 0;
    }

    // returns consumed energy in watts per 0.01 hour
    uint32_t getEnergy(const bool &clear = true)
    {
        if (counter == 0)
            return 0;

        uint32_t delta = millis() - timer;
        // power / ((3600 * 1000) / avgDelta)
        uint64_t energy = power * delta / counter / 3600;
        DEBUGLOG("EnergyMonitor", "Channel: %d, Power: %d, Duration: %d, Counter: %d, Energy: %d",
                 channel, (uint32_t)power, delta, counter, (uint32_t)energy);
        energy = round(energy / (float)10);

        if (clear)
        {
            timer = millis();
            counter = 0;
            power = 0;
        }
        return energy;
    }

  private:
    int16_t sampleI;
    double offsetI = 2 << 13; // half signed 16bit
    double filteredI;
    double sqI, sumI;
    double temp_irms;

    // https://github.com/openenergymonitor/EmonLib
    double calcIrms(uint16_t samplesCount)
    {
        /* Be sure to update this value based on the IC and the gain settings! */
        float multiplier = 0.125F; /* ADS1115 @ +/- 4.096V gain (16-bit results) */
        for (uint16_t i = 0; i < samplesCount; i++)
        {
            sampleI = ads.readADC_SingleEnded(channel);

            // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset,
            // then subtract this - signal is now centered on 0 counts.
            offsetI = (offsetI + (sampleI - offsetI) / 1024);
            filteredI = sampleI - offsetI;
            //filteredI = sampleI * multiplier;

            // Root-mean-square method current
            // 1) square current values
            sqI = filteredI * filteredI;
            // 2) sum
            sumI += sqI;
        }

        temp_irms = sqrt(sumI / samplesCount) * multiplier / 26;

        //Reset accumulators
        sumI = 0;

        return temp_irms;
    }
};

#endif
