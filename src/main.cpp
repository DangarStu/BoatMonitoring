// Outboard Cooling Control

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
#include <Wire.h>
#include "DFRobot_INA219.h"

#include "secrets.h" // Git ignored file with Pushover APIKEY and USER

using namespace sensesp;

reactesp::ReactESP app;

// Setpoint: the target temperature we are trying to maintain
// +273.15 to convert to Kelvin.
float min_voltage = 12;

// The maximum amount the temperature can get above the setpoint
// before sounding the alarm
const float alarm_margin = 10;

// GPIO number to use for the alarm buzzer.
// This is the pin to send Vext out from the optocoupler.
const uint8_t ALARM_PIN = 33;

// GPIO number to use for the coolant sensor input
const uint8_t THERMISTOR_PIN = 36;

// Store the alarm state so it can be written to Signal K
// An easily acted upon by Signal K
uint8_t alarm_pin_state = LOW;

// Record the last time the pump went off
unsigned long last_change = 0;

// Revise the following two paramters according to actula reading of the INA219 and the multimeter
// for linearly calibration
float ina219Reading_mA = 1000;
float extMeterReading_mA = 1000;

// Create an instance of the INA219 wattmeter to monitor battery bank 1.
DFRobot_INA219_IIC battery_bank_1(&Wire, 0x40);

// The setup function performs one-time application initialization.
void setup() {
  #ifndef SERIAL_DEBUG_DISABLED
    SetupSerialDebug(115200);
  #endif

  pinMode(ALARM_PIN, OUTPUT);

  /* while(battery_bank_1.begin() != true) {
      Serial.println("INA219 begin failed.");
      delay(2000);
  } */ 

  battery_bank_1.linearCalibrate(ina219Reading_mA, extMeterReading_mA);

  // Construct the global SensESPApp() object
  SensESPAppBuilder builder;

  sensesp_app = (&builder)
    ->set_wifi(SSID, WIFI_PASSOWRD)
    ->set_hostname("monitor") // Visit http://monitor.local to configure
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
  auto* analog_input = new AnalogInput(THERMISTOR_PIN, 200, "", 4096);

  // ------------------------------------------------------------------------------
  // A Lamba function to activate the high temperature
  // alarm if current_temp is error_margin above setpoint
  // ------------------------------------------------------------------------------
  auto temp_alarm = [](float input, float setpoint, float error_margin) -> float {
    uint8_t new_state;
    // debugD("Current temperature is %F", input);

    // Determine how long since the last change of state
    int time_since_change = (millis() - last_change);

    if (input > setpoint + error_margin) {
      new_state = HIGH;
    } else {
      new_state = LOW;
    }

    if (new_state != alarm_pin_state) {
      // Store the current time at this change of state.
      last_change = millis();

      if (new_state == HIGH) { 
        debugD("Coolant over temp!");
      } else {
        debugD("Coolant back below temp.");
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
  auto temp_alarm_lambda = new LambdaTransform<float, float, float, float>(
        temp_alarm,
        min_voltage,
        alarm_margin,
        temp_alarm_lambda_param_data,
        "/coolant/temp/alarm_margin");
  // ------------------------------------------------------------------------------

  // Read the AnalogVoltage, smooth it with a MovingAverage, Calculate the resistance
  // of the thermistor using a VoltageDivider, convert that resistance to a temperature
  // using the TemperatureInterpolator, calibrate it with a Linear transform, pass
  // the temperature into the PID Controller to figure out what to do, control the
  // pump speed using that information then send the pump speed to Signal K.
  analog_input->connect_to(new AnalogVoltage())
    ->connect_to(new MovingAverage(20, 1.0, ""))
    ->connect_to(new VoltageDividerR2(R2, Vin, "/coolant/temp/sender"))
    ->connect_to(new Linear(1.0, 0, "/coolant/temp/calibrate"))
    ->connect_to(temp_alarm_lambda)
    ->connect_to(new SKOutputNumeric<float>(
      "vessels.tinny.propulsion.outboard.coolantTemperature",
      "",
      new SKMetadata("K", "The current coolant temperature"))
    )
    ->connect_to(new SKOutputNumeric<float>(
      "vessels.tinny.propulsion.outboard.revolutions",
      "",
      new SKMetadata("8BitDAC", "The current drive speed of the coolant pump."))
    );

  // A RepeatSensor that fetches the state of the coolant temperature alarm.
  auto alarm_state = new RepeatSensor<uint8_t>(sample_time, []() -> uint8_t {
    return (alarm_pin_state);   
  });

  // Connect to this sensor to send the output to Signal K
  alarm_state->connect_to(new SKOutputNumeric<float>(
      "vessels.renko.electrical.batteries.house.voltage.alarm",
      "",
      new SKMetadata("Boolean", "The state of the house bank voltage alarm."))
    );

  // A RepeatSensor that fetches the voltage of battery 1.
  auto battery_bank_1_monitor = new RepeatSensor<float>(sample_time, []() -> float {
    return (battery_bank_1.getBusVoltage_V());   
  });

  // Connect to this sensor to send the output to Signal K
  battery_bank_1_monitor->connect_to(new SKOutputNumeric<float>(
      "vessels.renko.electrical.batteries.house.voltage",
      "",
      new SKMetadata("V", "The voltage of the house battery bank."))
    );

  Serial.println(WiFi.status());
  Serial.println(WiFi.localIP());
  

  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");
  Wire.begin();

  while(!nDevices) {
    for(address = 8; address < 120; address++ ) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();

      if (error == 0) {
        Serial.print("I2C device found at address 0x");
        if (address<16) {
          Serial.print("0");
        }
        Serial.println(address,HEX);
        nDevices++;
      }
      else if (error==4) {
        Serial.print("Unknow error at address 0x");
        if (address<16) {
          Serial.print("0");
        }
        Serial.println(address,HEX);
      }    
    }
        Serial.println("No I2C devices found\n");
  }

  battery_bank_1.begin();

  Serial.println("Battery voltage is:");
  Serial.println(battery_bank_1.getBusVoltage_V());

  // Start networking and other SensESP internals
  sensesp_app->start();
}

void loop() { 
  app.tick(); 
}