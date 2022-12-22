#include <Wire.h>
#include <Arduino.h>
#include "sensesp/sensors/analog_input.h"
#include "sensesp/sensors/digital_input.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp/system/lambda_consumer.h"
#include "sensesp_app_builder.h"
#include "sensesp/transforms/analogvoltage.h"
#include "sensesp/transforms/linear.h"
#include "sensesp/transforms/voltagedivider.h"
#include "sensesp/transforms/moving_average.h"

#include "secrets.h" // Git ignored file

using namespace sensesp;

reactesp::ReactESP app;

// Setpoint: the target temperature we are trying to maintain
// +273.15 to convert to Kelvin.
float Setpoint = 80.0 + 273.15;

// The maximum amount the temperature can get above the setpoint
// before sounding the alarm
const float alarm_margin = 10;

// GPIO number to use for the alarm buzzer.
// This is the pin to send Vext out from the optocoupler.
const uint8_t ALARM_PIN = 33;

// GPIO number to use for the high water alarm.
// This is the pin to send Vext out from the optocoupler.
const uint8_t HIGH_WATER_PIN = 33;

// Store the alarm state so it can be written to Signal K
// An easily acted upon by Signal K
uint8_t alarm_pin_state = LOW;

// Record the last time the pump went off
unsigned long last_change = 0;

// The setup function performs one-time application initialization.
void setup() {
  #ifndef SERIAL_DEBUG_DISABLED
    SetupSerialDebug(115200);
  #endif

  pinMode(ALARM_PIN, OUTPUT);

  // Construct the global SensESPApp() object
  SensESPAppBuilder builder;

  sensesp_app = (&builder)
    ->set_wifi(SSID, WIFI_PASSOWRD)
    ->set_hostname("outboard") // Visit http://outboard.local to configure
    ->get_app();

  // The sample time to set both the PID and Setpoint forwarder to
  const unsigned int sample_time = 1000;

  // Voltage sent into the voltage divider circuit that includes the analog
  // sender
  const float Vin = 3.29;

  // The resistance, in ohms, of the fixed resistor (R1) in the voltage divider
  // circuit
  const float R2 = 330.0;

  // An AnalogInput gets the value from the microcontroller's AnalogIn pin,
  // which is a value from 0 to 4095.
  auto* high_water_input = new DigitalInputChange(HIGH_WATER_PIN, PULLUP, CHANGE);


  // ------------------------------------------------------------------------------
  // A Lamba function to activate the high temperature
  // alarm if current_temp is error_margin above setpoint
  // ------------------------------------------------------------------------------
  auto battery_alarm = [](float input, float minimum_voltage) -> float {
    uint8_t new_state;
    // debugD("Current temperature is %F", input);

    // Determine how long since the last change of state
    int time_since_change = (millis() - last_change);

    if (input < minimum_voltage) {
      new_state = HIGH;
    } else {
      new_state = LOW;
    }

    if (new_state != alarm_pin_state) {
      // Store the current time at this change of state.
      last_change = millis();

      if (new_state == HIGH) { 
        debugD("Battery under minimum voltage");
      } else {
        debugD("Battery back above minimum voltage.");
      }

      alarm_pin_state = new_state;
      digitalWrite(ALARM_PIN, alarm_pin_state);
    }

    if (alarm_pin_state == HIGH && time_since_change > 3000) {
      // Pump has been on for more than five minutes, sound
      // the super bad alarm
    }

    // Pass the input value on
    return input;
  };

  // This is an array of parameter information, providing the keys and
  // descriptions required to display user-friendly values in the configuration
  // interface. These are two extra input parameters to drive_motor() above
  // and beyond the default 'input' parameter passed in by the chained value
  // producer.
  const ParamInfo* temp_alarm_lambda_param_data = new ParamInfo[1]{
        {"alarm_margin", "Alarm margin"}};

  // This is a LambaTransform that calls sounds the over-temp alarm
  // if the current temperature is ever a set level over the setpoint.
  auto bilge_alarm_lambda = new LambdaTransform<float, float, float>(
        minimum_voltage,
        temp_alarm_lambda_param_data,
        "/coolant/temp/alarm_margin");
  // ------------------------------------------------------------------------------

  // Read the state of the high-water float switch
  high_water_input
    ->connect_to(bilge_alarm_lambda)
    ->connect_to(new SKOutputNumeric<float>(
      "vessels.renko.bilge",
      "",
      new SKMetadata("K", "The current coolant temperature"))
    );

  // A RepeatSensor that fetches the state of the coolant temperature alarm.
  auto alarm_state = new RepeatSensor<uint8_t>(sample_time, []() -> uint8_t {
    return (alarm_pin_state);   
  });

  // Connect to this sensor to send the output to Signal K
  alarm_state->connect_to(new SKOutputNumeric<float>(
      "vessels.tinny.propulsion.outboard.coolant.alarm",
      "",
      new SKMetadata("Boolean", "The state of the over temp alarm."))
    );
  
  // Start networking and other SensESP internals
  sensesp_app->start();
}