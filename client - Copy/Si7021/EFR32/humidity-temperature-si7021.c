// Copyright 2015 Silicon Laboratories, Inc.                                  80
//------------------------------------------------------------------------------

#include PLATFORM_HEADER
#include "stack/include/ember-types.h"
#include "stack/include/event.h"
#include "hal/hal.h"
#include "hal/micro/i2c-driver.h"
#include "hal/micro/micro.h"
#include "../si7021/efr32_inc/humidity.h"
#include "hal/micro/temperature.h"
#include "../si7021/efr32_inc/humidity-temperature-si7021.h"

#define EMBER_AF_PLUGIN_HUMIDITY_SI7021
#define DEFAULT_MEASUREMENT_ACCURACY ADC_12_BIT_CONV_TIME_MS

// If printing is enabled, we will print some diagnostic information about the
// most recent reset and also during runtime.  On some platforms, extended
// diagnostic information is available.
#if defined(EMBER_AF_API_SERIAL) && defined(EMBER_AF_PRINT_ENABLE)
  #ifdef EMBER_AF_API_DIAGNOSTIC_CORTEXM3
    #include EMBER_AF_API_DIAGNOSTIC_CORTEXM3
  #endif
  static void printResetInformation(void);
  #define PRINT_RESET_INFORMATION printResetInformation
  #define emberAfGuaranteedPrint(...) \
    emberSerialGuaranteedPrintf(APP_SERIAL, __VA_ARGS__)
  #define emberAfGuaranteedPrintln(format, ...) \
    emberSerialGuaranteedPrintf(APP_SERIAL, format "\r\n", __VA_ARGS__);
#else
  #define PRINT_RESET_INFORMATION()
  #define emberAfGuaranteedPrint(...)
  #define emberAfGuaranteedPrintln(...)
#endif








//------------------------------------------------------------------------------
// Plugin events
EmberEventControl emberAfPluginHumiditySi7021InitEventControl;
EmberEventControl emberAfPluginHumiditySi7021ReadEventControl;
EmberEventControl emberAfPluginTemperatureSi7021InitEventControl;
EmberEventControl emberAfPluginTemperatureSi7021ReadEventControl;

//------------------------------------------------------------------------------
// Plugin private event handlers

// At this point, I2C driver is guaranteed to have initialized, so it is safe
// to call the i2c based init function
void emberAfPluginHumiditySi7021InitEventHandler(void)
{
  halHumidityInit();
  emberEventControlSetInactive(emberAfPluginHumiditySi7021InitEventControl);
}

void emberAfPluginTemperatureSi7021InitEventHandler(void)
{
  halTemperatureInit();
  emberEventControlSetInactive(emberAfPluginTemperatureSi7021InitEventControl);
}

//------------------------------------------------------------------------------
// Plugin defined callbacks

// The init callback, which will be called by the framework on init.
void emberAfPluginHumiditySi7021InitCallback(void)
{
  emberEventControlSetDelayMS(emberAfPluginHumiditySi7021InitEventControl,
                              SI_7021_INIT_DELAY_MS);
}

void emberAfPluginTemperatureSi7021InitCallback(void)
{
  emberEventControlSetDelayMS(emberAfPluginTemperatureSi7021InitEventControl,
                              SI_7021_INIT_DELAY_MS);
}

// The read event callback, which will be called after the conversion result is
// ready to be read.
void emberAfPluginTemperatureSi7021ReadEventHandler(void)
{
  uint8_t errorCode;
  uint8_t command;
  int32_t temperatureMilliC = 0;
  int32_t temperatureCalculation;
  uint8_t buffer[DEFAULT_READ_BUFFER_SIZE];

  if (startReadBeforeInitialization) {
    startReadBeforeInitialization = false;
    halTemperatureStartRead(); 
  } else {
    emberEventControlSetInactive(emberAfPluginTemperatureSi7021ReadEventControl);
    command = SI_7021_READ_TEMPERATURE;
    errorCode = halI2cWriteBytes(SI_7021_ADDR, &command, SEND_COMMAND_SIZE);
    if (errorCode != I2C_DRIVER_ERR_NONE) {
      halTemperatureReadingCompleteCallback(0, false);
    }
    errorCode = halI2cReadBytes(SI_7021_ADDR,
                                &buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],
                                READ_BYTE_SIZE_TEMPERATURE);
    if (errorCode != I2C_DRIVER_ERR_NONE) {
      halTemperatureReadingCompleteCallback(
          SI_7021_READ_BUFFER_LOCATION_BYTE_0,
          false);
    } else {
      //temperature (milliC) = 1000 * (175.72 * ADC /65536 - 46.85)
      //                     = (1000 * ( 5624 * ADC / 8192 - 11994 )) / 256
      temperatureCalculation = HIGH_LOW_TO_INT(
                                 buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],
                                 buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_1]);
      temperatureCalculation = 5624 * temperatureCalculation;
      temperatureCalculation = temperatureCalculation >> 13;
      temperatureCalculation = temperatureCalculation - 11994;
      temperatureCalculation = (temperatureCalculation * 1000) >> 8;
      temperatureMilliC = temperatureCalculation;
    }    
    measureInprogress = false;
    halTemperatureReadingCompleteCallback(temperatureMilliC, true);
  }
}

// The read event callback, which will be called after the conversion result is
// ready to be read.
void emberAfPluginHumiditySi7021ReadEventHandler(void)
{
  uint8_t errorCode;
  uint32_t relativeHumidity;
  uint8_t buffer[DEFAULT_READ_BUFFER_SIZE];

  //we started read before initilization, so we did not send any measurement command.
  //so we redo that again here
  if (startReadBeforeInitialization) {
    startReadBeforeInitialization = false;
    halHumidityStartRead(); 
  } else {
    emberEventControlSetInactive(
        emberAfPluginHumiditySi7021ReadEventControl);

    errorCode = halI2cReadBytes(SI_7021_ADDR, 
                                &buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],
                                READ_BYTE_SIZE_WITH_CHECKSUM);
    if (errorCode != I2C_DRIVER_ERR_NONE) {
      halHumidityReadingCompleteCallback(0, false);
      return;
    }
    
    relativeHumidity = HIGH_LOW_TO_INT(buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],
                                       buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_1]);
    //Relative Humidity in % is calculated as the following:
    //let D = data from Si7021;
    //%RH = ((125 * D) / 65536) - 6;
    //
    //to calculate the Relative Humidity in Centi Percentage:
    //Centi%RH = (((125 * D) / 65536) - 6) * 100
    //         = (((100 * 125 * D) / 65536) - 600);
    relativeHumidity = relativeHumidity * 100;
    relativeHumidity = relativeHumidity * 125;
    relativeHumidity = relativeHumidity >> 16;
    relativeHumidity = relativeHumidity - 600;
    measureInprogress = false;
    halHumidityReadingCompleteCallback((uint16_t)relativeHumidity, true);
  }
}

//------------------------------------------------------------------------------
// Plugin public functions

// All init functionality requires the I2C plugin to be initialized, which is
// not guaranteed to be the case by the time this function executes.  As a
// result, I2C based init functionality needs to execute from task context.
void halTemperatureInit(void)
{
  si7021Init();
}

void halHumidityInit(void)
{
  si7021Init();
}

void si7021Init(void)
{
  uint8_t registerSetting;
  uint8_t resetMsg = SI_7021_RESET;
  //initialized =false;
  if (initialized == false) {
    // Send the reset command to the temperature sensor
    if (halI2cWriteBytes(SI_7021_ADDR, &resetMsg, SEND_COMMAND_SIZE) 
        != I2C_DRIVER_ERR_NONE) {
      return;
    }

#ifdef EMBER_AF_PLUGIN_HUMIDITY_SI7021
    switch (DEFAULT_MEASUREMENT_ACCURACY) {
    case ADC_BIT_12:
      measurementReadyDelayMs = ADC_12_BIT_CONV_TIME_MS;
      registerSetting = ADC_BIT_SETTING_12;
      break;
    case ADC_BIT_11:
      measurementReadyDelayMs = ADC_11_BIT_CONV_TIME_MS;
      registerSetting = ADC_BIT_SETTING_11;
      break;
    case ADC_BIT_10:
      measurementReadyDelayMs = ADC_10_BIT_CONV_TIME_MS;
      registerSetting = ADC_BIT_SETTING_10;
      break;
    case ADC_BIT_8:
      measurementReadyDelayMs = ADC_8_BIT_CONV_TIME_MS;
      registerSetting = ADC_BIT_SETTING_8;
      break;
    default:
      break;
    }
    halCommonDelayMilliseconds(10); //delay 10ms after reset
    humidityAccuracySet(registerSetting);
#endif
    initialized = true;
  }
}

uint8_t si7021StartRead(void)
{
  uint8_t errorCode,command;
  measureInprogress =false;
  errorCode = I2C_DRIVER_ERR_NONE;
  command = SI_7021_MEASURE_RELATIVEHUMIDITY;
  if (measureInprogress == false) {
    errorCode = halI2cWriteBytes(SI_7021_ADDR, &command, SEND_COMMAND_SIZE);
    if (errorCode != I2C_DRIVER_ERR_NONE) {
    } else {
      measureInprogress = true;
    }
  }
  return errorCode;
}

//******************************************************************************
// Perform all I2C transactions of measure command to SI7021 so that it will
// start a single conversion immediately and kick off a event for humidity data 
// read event
//******************************************************************************
void halHumidityStartRead(void)
{
  if (initialized == false) {
    si7021Init();
    startReadBeforeInitialization = true;
  } else {
	  si7021StartRead();
  }
  //emberEventControlSetDelayMS(emberAfPluginHumiditySi7021ReadEventControl,measurementReadyDelayMs);
}

//******************************************************************************
// Perform all I2C transactions of measure command to SI7021 so that it will
// start a single conversion immediately and kick off a event for temperature
// data read event
//******************************************************************************
void halTemperatureStartRead(void)
{
  if (initialized == false) {
    si7021Init();
    startReadBeforeInitialization = true;
  } else {
    si7021StartRead();
  }
  //emberEventControlSetDelayMS(emberAfPluginTemperatureSi7021ReadEventControl,measurementReadyDelayMs);
}

static uint8_t humidityAccuracySet(uint8_t measureAccuracy)
{
  uint8_t errorCode, currentValue;
  uint8_t writeBuffer[DEFAULT_WRITE_BUFFER_SIZE];

  currentValue = DEFAULT_USER_SETTING;
  writeBuffer[SI_7021_WRITE_BUFFER_COMMAND_LOCATION] = SI_7021_USER_REG_READ;
  
  halI2cWriteBytes(SI_7021_ADDR, writeBuffer, SEND_COMMAND_SIZE);
  errorCode = halI2cReadBytes(SI_7021_ADDR, 
                              &currentValue,
                              READ_BYTE_SIZE_USER_REGISTER);
  if (errorCode != I2C_DRIVER_ERR_NONE) {
    return errorCode;
  }
 
  writeBuffer[SI_7021_WRITE_BUFFER_COMMAND_LOCATION] = SI_7021_USER_REG_WRITE;

  // ADCbits[0]: USER REGISTER bit 0 
  // ADCbits[1]: USER REGISTER bit 7 
  // USER REGISTER (7 bit): X X X X X X X
  //                        |           |
  //                     ADCBit[1]   ADCBit[0]
  writeBuffer[SI_7021_WRITE_BUFFER_VALUE_LOCATION] = (currentValue & (0x7E)) |
              (((measureAccuracy & 0x2) << 6) | 
              ((measureAccuracy & 0x1)));
  errorCode = halI2cWriteBytes(SI_7021_ADDR,
                               writeBuffer,
                               DEFAULT_WRITE_BUFFER_SIZE);
  return errorCode;
}




