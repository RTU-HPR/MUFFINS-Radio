#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include <MUFFINS_Component_Base.h>

class Radio : public Component_Base
{
public:
  struct Config
  {
    float frequency;
    int cs;
    int dio0;
    int dio1;
    int reset;
    int sync_word;
    int tx_power; // dBm
    int spreading;
    int coding_rate;
    float signal_bw; // kHz
    bool frequency_correction;
    SPIClass *spi_bus; // Example &SPI
  };

  struct Received_Data
  {
    uint8_t *bytes;
    uint16_t length;
    float rssi;
    float snr;
    double frequency;
  };

private:
  enum Last_Action_Type
  {
    Transmit,
    Receive,
    Standby,
  };

  struct Runtime_State
  {
    double frequency;
    int action_status_code;
    Last_Action_Type last_action;
  } _runtime_state;

  // Radio object
  SX1268 _radio = new Module(-1, -1, -1, -1);

  // Local config object
  Config _config;

  Module *_module;

  /**
   * @brief Configure radio module modulation parameters (frequency, power, etc.) for exact things that are set check the function
   *
   * @return true If configured successfully
   */
  bool _configure();

public:
  /**
   * @brief Construct a new Radio object
   */
  Radio(String component_name = "Radio", void (*info_function)(String) = nullptr, void (*error_function)(String) = nullptr);

  /**
   * @brief Destroy the Radio object
   */
  ~Radio();

  Received_Data received_data;

  /**
   * @brief
   *
   * @param config Radio config file to be used
   * @return true If configured successfully
   */
  bool begin(const Config &config);

  /**
   * @brief Send a byte array message over the radio
   *
   * @param msg Bytes to send
   * @param length Byte array length
   * @return true If transmit was successful
   */
  bool transmit_bytes(uint8_t *bytes, size_t length);

  /**
   * @brief Read any received data as bytes
   * @return true If received data
   */
  bool receive_bytes();

  /**
   * @brief Reconfigure radio module modulation parameters (frequency, power, etc.) during runtime for exact things that are set check the function
   *
   * @return true If configured successfully
   */
  bool reconfigure(const Config &config);
};