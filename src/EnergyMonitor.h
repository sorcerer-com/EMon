#ifndef ENERGY_MONITOR_H
#define ENERGY_MONITOR_H


class EnergyMonitor
{
private:
    uint8_t inputPin;

    const uint16_t samplesCount = 1480;
    double irms = 0.0;

    uint16_t counter = 0;
    uint32_t timer = 0;
    uint64_t power = 0;

public:
    double current = 0.0;

    EnergyMonitor(const uint8_t& _inputPin) : inputPin(_inputPin)
    {
    }

    void update()
    {
        irms += calcIrms(samplesCount);
        current = irms;
        if (irms < 0.2) // noise
            irms = 0.0;
        power += round(irms * VoltageMonitor.voltage);
        counter++;
        //DEBUGLOG("EnergyMonitor", "InputPin: %d, Irms: %f, power: %d in %d counts",
        //         inputPin, irms, power, counter);
        irms = 0;
    }

    // returns consumed energy in watts per 0.01 hour
    uint32_t getEnergy(const bool &clear = true)
    {
        if (counter == 0)
            return 0;

        uint32_t delta = millis() - timer;
        // power / ((3600 * 1000) / avgDelta)
        uint64_t energy = power * delta / counter / 3600;
        energy = round(energy / (float)10);

        if (clear)
        {
            DEBUGLOG("EnergyMonitor", "InputPin: %d, Power: %d, Duration: %d, Counter: %d, Energy: %d",
                     inputPin, (uint32_t)power, delta, counter, (uint32_t)energy);

            timer = millis();
            counter = 0;
            power = 0;
        }
        return energy;
    }

private:
    int16_t sampleI;
    // TODO: no ADS
    double offsetI = (double)(2 << 13) * 3.3 / 4.096; // half signed 16bit * (3.3V / 4.096V (ADS))
    double filteredI;
    double sqI, sumI;
    double temp_irms;

    // https://github.com/openenergymonitor/EmonLib
    double calcIrms(uint16_t samplesCount)
    {
        /* Be sure to update this value based on the IC and the gain settings! */
        // TODO: no ADS
        float multiplier = 0.125F; /* ADS1115 @ +/- 4.096V gain (16-bit results) */
        for (uint16_t i = 0; i < samplesCount; i++)
        {
            sampleI = analogRead(inputPin);

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

        temp_irms = sqrt(sumI / samplesCount) * multiplier / 20; // 24;

        //Reset accumulators
        sumI = 0;

        return temp_irms;
    }
};

#endif
