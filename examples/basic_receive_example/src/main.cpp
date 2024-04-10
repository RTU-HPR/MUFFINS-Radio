#include <Arduino.h>
#include <MUFFINS_Radio.h>

const int SENSOR_POWER_ENABLE_PIN = 17;
const int SPI0_RX = 4;
const int SPI0_TX = 3;
const int SPI0_SCK = 2;

Radio radio;

Radio::Config radio_config{
    .frequency = 434.5,
    .cs = 5,
    .dio0 = 8,
    .dio1 = 9,
    .reset = 7,
    .sync_word = 0xF4,
    .tx_power = 22,
    .spreading = 11,
    .coding_rate = 8,
    .signal_bw = 62.5,
    .frequency_correction = false,
    .spi_bus = &SPI,
};

int counter = 0;

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(1000);
  }

  pinMode(SENSOR_POWER_ENABLE_PIN, OUTPUT_12MA);
  digitalWrite(SENSOR_POWER_ENABLE_PIN, HIGH);

  if (SPI.setRX(SPI0_RX) && SPI.setTX(SPI0_TX) && SPI.setSCK(SPI0_SCK))
  {
    SPI.begin();
  }

  if (!radio.begin(radio_config))
  {
    while (1)
      ;
  };
}

void loop()
{
  if (radio.receive_bytes())
  {
    Serial.print("Received: ");
    for (int i = 0; i < radio.received_data_bytes.data_length; i++)
    {
      Serial.print(radio.received_data_bytes.bytes[i], HEX);
      Serial.print(" ");
    }
  }
}