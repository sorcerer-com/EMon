#ifndef ENERGY_MONITOR_H
#define ENERGY_MONITOR_H

class EnergyMonitor
{
private:
    const uint8_t inputPin;
    const uint16_t samplesDuration = 200; // ms // TODO: check which is the best value

    double irms = 0.0;
    uint16_t counter = 0;
    uint32_t timer = 0;
    uint64_t power = 0;

public:
    double current = 0.0;

    EnergyMonitor(const uint8_t &_inputPin) : inputPin(_inputPin)
    {
    }

    void update()
    {
        irms += calcIrms();
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
#ifndef REMOTE_DEBUG // only on serial debug
            DEBUGLOG("EnergyMonitor", "InputPin: %d, Power: %d, Duration: %d, Counter: %d, Energy: %d",
                     inputPin, (uint32_t)power, delta, counter, (uint32_t)energy);
#endif

            timer = millis();
            counter = 0;
            power = 0;
        }
        return energy;
    }

private:
    int32_t sampleI;
    double offsetI = 1805; //(double)(2 << 10); // half of the max 12bit number
    double filteredI;
    double sqI, sumI;
    double temp_irms;

    // https://github.com/openenergymonitor/EmonLib
    double calcIrms()
    {
        uint32_t start = millis();
        uint16_t samplesCount = 0;
        while (millis() - start < samplesDuration)
        {
            sampleI = 0;
            for (int i = 0; i < 100; i++) // fix ADC noise
                sampleI += analogRead(inputPin);
            sampleI /= 100;
            samplesCount++;

            // Digital low pass filter extracts the 2.5 V or 1.65 V dc offset,
            // then subtract this - signal is now centered on 0 counts.
            offsetI = (offsetI + (sampleI - offsetI) / 1024);
            filteredI = sampleI - offsetI;

            // Root-mean-square method current
            // 1) square current values
            sqI = filteredI * filteredI;
            // 2) sum
            sumI += sqI;
        }

        temp_irms = sqrt(sumI / samplesCount) / 41.4; // * 30(amps per 1V) * 3.3V / 4096(ADC)

        //Reset accumulators
        sumI = 0;

        return temp_irms;
    }
};

#endif
