#include <ResponsiveAnalogRead.h>

#define SENSOR_PIN 33
#define OFFSET_SAMPLES 10
#define RESOLUTION 4095
#define VREF 3.3
#define SENSIBILITY 0.04
#define FREQUENCY 60
#define MOVING_AVERAGE_SAMPLES 10
#define ACTIVITY_THRESHOLD 0.05

ResponsiveAnalogRead analog(0, true);

uint16_t offset = 0;

const uint32_t period_us = 1000000 / FREQUENCY;

struct MovingAverage
{
  float history[MOVING_AVERAGE_SAMPLES];
  int position = 0;
  float aggregated = 0.0;
  float average = 0.0;
};

struct MovingAverage movingAverage1;
struct MovingAverage movingAverage2;

float movingAverageFilter(struct MovingAverage *data, float newValue)
{
  data->aggregated += newValue - data->history[data->position];
  data->history[data->position] = newValue;
  data->average = (float)data->aggregated / MOVING_AVERAGE_SAMPLES;
  data->position = (data->position + 1) % MOVING_AVERAGE_SAMPLES;
  return (float)data->average;
}

void cleanMovingAverageHistory(struct MovingAverage *data)
{
  for (int i = 0; i <= MOVING_AVERAGE_SAMPLES; i++)
  {
    data->history[i] = 0;
  }
}

uint16_t getSensorZeroOffset()
{
  unsigned long value = 0;

  for (int i = 0; i < OFFSET_SAMPLES; i++)
  {
    value += analogRead(SENSOR_PIN);
  }

  return value / OFFSET_SAMPLES;
}

void setup()
{
  pinMode(SENSOR_PIN, INPUT);

  Serial.begin(115200);
  analog.setAnalogResolution(RESOLUTION + 1);

  cleanMovingAverageHistory(&movingAverage1);
  cleanMovingAverageHistory(&movingAverage2);

  offset = getSensorZeroOffset();
}

void loop()
{
  uint32_t Isum1 = 0, Isum2 = 0, measurements_count = 0;
  int32_t Inow1, Inow2;

  uint32_t t_start_us = micros();
  while (micros() - t_start_us < period_us)
  {
    uint16_t reading = analogRead(SENSOR_PIN);
    analog.update(reading);

    Inow1 = reading - offset;
    Isum1 += Inow1 * Inow1;

    Inow2 = analog.getValue() - offset;
    Isum2 += Inow2 * Inow2;

    measurements_count++;
  }

  float Irms1 = sqrt(Isum1 / measurements_count) / RESOLUTION * VREF / SENSIBILITY;
  float Irms1Average = movingAverageFilter(&movingAverage1, Irms1);

  float Irms2 = sqrt(Isum2 / measurements_count) / RESOLUTION * VREF / SENSIBILITY;
  float Irms2Average = movingAverageFilter(&movingAverage2, Irms2);

  float Ifinal = Irms2Average < ACTIVITY_THRESHOLD ? 0 : Irms2Average;

  Serial.print("Irms:");
  Serial.print(Irms1);
  Serial.print("\tIrmsFiltered:");
  Serial.print(Irms2);
  Serial.print("\tIrmsAvg:");
  Serial.print(Irms1Average);
  Serial.print("\tIrmsFilteredAvg:");
  Serial.print(Irms2Average);
  Serial.print("\tIfinal:");
  Serial.println(Ifinal);

  delay(20);
}