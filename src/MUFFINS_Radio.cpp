#include "MUFFINS_Radio.h"

// For the interrupt functionality of RadioLib
// For some reason, I couldn't properly include this in the header file
// So I had to include it here
// It is something to do with volatile variables
volatile bool _action_done = true;
namespace RadioLib_Interupt
{

/*
If compiling for ESP boards, specify that this function is used within a interrupt routine
and such should be stored in the RAM and not the flash memory
*/
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
  /**
   * @brief Interrupt function for setting action done flag
   */
  void set_action_done_flag(void)
  {
    _action_done = true;
  }
};

Radio::Radio(String component_name, void (*info_function)(String), void (*error_function)(String)) : Component_Base(component_name, info_function, error_function)
{
  _runtime_state.action_status_code = RADIOLIB_ERR_NONE;
  _runtime_state.last_action = Last_Action_Type::Standby;
}

Radio::~Radio()
{
  return;
}

bool Radio::begin(const Config &config)
{
  // Copy the config to the local one
  _config = config;

  // Set the used frequency to the inital one
  _runtime_state.frequency = _config.frequency;

  _radio = new Module(_config.cs, _config.dio1, _config.reset, _config.dio0, *(_config.spi_bus));

  // Try to initialize communication with LoRa
  _runtime_state.action_status_code = _radio.begin();

  // If initialization failed, print error
  if (_runtime_state.action_status_code != RADIOLIB_ERR_NONE)
  {
    error("Initialization failed with status code: " + String(_runtime_state.action_status_code));
    return false;
  }
  // Set interrupt behaviour
  _radio.setPacketReceivedAction(RadioLib_Interupt::set_action_done_flag);
  _runtime_state.last_action = Last_Action_Type::Standby;

  if (!_configure())
  {
    error("Configuration failed!");
    return false;
  }
  info("Configured");

  // Set that radio has been initialized
  set_initialized(true);
  info("Initialized");

  return true;
}

bool Radio::_configure()
{
  if (_radio.setFrequency(_config.frequency) == RADIOLIB_ERR_INVALID_FREQUENCY)
  {
    error("Frequency is invalid: " + String(_config.frequency));
    return false;
  };

  if (_radio.setOutputPower(_config.tx_power) == RADIOLIB_ERR_INVALID_OUTPUT_POWER)
  {
    error("Transmit power is invalid: " + String(_config.tx_power));
    return false;
  };

  if (_radio.setSpreadingFactor(_config.spreading) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR)
  {
    error("Spreading factor is invalid: " + String(_config.spreading));
    return false;
  };

  if (_radio.setCodingRate(_config.coding_rate) == RADIOLIB_ERR_INVALID_CODING_RATE)
  {
    error("Coding rate is invalid: " + String(_config.coding_rate));
    return false;
  };

  if (_radio.setBandwidth(_config.signal_bw) == RADIOLIB_ERR_INVALID_BANDWIDTH)
  {
    error("Signal bandwidth is invalid: " + String(_config.signal_bw));
    return false;
  };

  if (_radio.setSyncWord(_config.sync_word) == RADIOLIB_ERR_INVALID_SYNC_WORD)
  {
    error("Sync word is invalid: " + String(_config.sync_word));
    return false;
  };

  if (_radio.setDio2AsRfSwitch(true) != RADIOLIB_ERR_NONE)
  {
    error("Failed to set DIO2 as RF switch");
    return false;
  }

  return true;
}

bool Radio::transmit_bytes(uint8_t *bytes, size_t length)
{
  if (!initialized())
  {
    return false;
  }

  // if radio did something that is not sending data before and it hasn't timed out. Time it out
  if (!_action_done && _runtime_state.last_action != Last_Action_Type::Transmit)
  {
    _action_done = true;
  }

  // If already transmitting, don't continue
  if (_action_done == false)
  {
    return false;
  }
  else
  {
    // else reset flag
    _action_done = false;
  }

  // Clean up from the previous time
  _radio.finishTransmit();

  // Start transmitting
  _runtime_state.action_status_code = _radio.startTransmit(bytes, length);

  // If transmit failed, print error
  if (_runtime_state.action_status_code != RADIOLIB_ERR_NONE)
  {
    error("Starting transmit failed with status code: " + String(_runtime_state.action_status_code));
    return false;
  }
  // set last action to transmit
  _runtime_state.last_action = Last_Action_Type::Transmit;

  return true;
}

bool Radio::receive_bytes()
{
  if (!initialized())
  {
    return false;
  }

  // If already doing something, don't continue
  if (_action_done == false)
  {
    return false;
  }
  else
  {
    // else reset flag
    _action_done = false;
  }
  // Put into standby to try reading data
  _radio.standby();
  byte received_bytes[256];
  uint16_t data_length = 0;
  if (_runtime_state.last_action == Last_Action_Type::Receive)
  {
    // Try to read received data
    _runtime_state.action_status_code = _radio.readData(received_bytes, 0);

    data_length = _radio.getPacketLength();

    if (_runtime_state.action_status_code != RADIOLIB_ERR_NONE)
    {
      error("Receiving failed with status code: " + String(_runtime_state.action_status_code));
    }

    received_data.bytes = received_bytes;
    received_data.length = data_length;
    received_data.rssi = _radio.getRSSI();
    received_data.snr = _radio.getSNR();
    received_data.frequency = _runtime_state.frequency;

    if (_config.frequency_correction)
    {
      // Frequency correction
      double freq_error = _radio.getFrequencyError() / 1000000.0;
      double new_freq = _runtime_state.frequency - freq_error;
      if (_radio.setFrequency(new_freq) != RADIOLIB_ERR_INVALID_FREQUENCY)
      {
        _runtime_state.frequency = new_freq;
      }
    }
  }
  // Restart receiving TODO add error check for start recieve
  _radio.startReceive();
  _runtime_state.last_action = Last_Action_Type::Receive;

  // If no errors and nothing was received, return false
  if (_runtime_state.action_status_code != RADIOLIB_ERR_NONE || data_length == 0)
  {
    return false;
  }
  return true;
}