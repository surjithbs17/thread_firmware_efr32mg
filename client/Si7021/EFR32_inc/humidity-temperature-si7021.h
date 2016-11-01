/*
 * humidity-temperature-si7021.h
 *
 *  Created on: Oct 28, 2016
 *      Author: Chinmay
 *
 *      Base from Silicon Labs (EMber Znet Stack PLugin)
 */

#ifndef SI7021_EFR32_INC_HUMIDITY_TEMPERATURE_SI7021_H_
#define SI7021_EFR32_INC_HUMIDITY_TEMPERATURE_SI7021_H_

#define SI_7021_INIT_DELAY_MS                  50
#define SI_7021_USER_REG_WRITE               0xE6
#define SI_7021_USER_REG_READ                0xE7
#define SI_7021_MEASURE_RELATIVEHUMIDITY     0xF5

#define SI_7021_MEASURE_TEMPERATURE          0xF3
#define SI_7021_READ_TEMPERATURE             0xE0

#define SI_7021_RESET             0xFE

#define SI_7021_WRITE_BUFFER_COMMAND_LOCATION 0
#define SI_7021_WRITE_BUFFER_VALUE_LOCATION   1
#define SI_7021_READ_BUFFER_LOCATION_BYTE_0   0
#define SI_7021_READ_BUFFER_LOCATION_BYTE_1   1

#define ADC_BIT_12    12
#define ADC_BIT_11    11
#define ADC_BIT_10    10
#define ADC_BIT_8     8

#define ADC_BIT_SETTING_12  0
#define ADC_BIT_SETTING_11  3
#define ADC_BIT_SETTING_10  2
#define ADC_BIT_SETTING_8   1

#define ADC_12_BIT_CONV_TIME_MS       20
#define ADC_11_BIT_CONV_TIME_MS       15
#define ADC_10_BIT_CONV_TIME_MS       10
#define ADC_8_BIT_CONV_TIME_MS         5

#define SI_7021_ADDR (0x40<<1)

#define READ_BYTE_SIZE_WITH_CHECKSUM  3
#define READ_BYTE_SIZE_TEMPERATURE    2
#define READ_BYTE_SIZE_USER_REGISTER  1
#define SEND_COMMAND_SIZE        1
#define SEND_COMMAND_WITH_VALUE_SIZE  2
#define DEFAULT_READ_BUFFER_SIZE      4
#define DEFAULT_WRITE_BUFFER_SIZE     2
#define DEFAULT_USER_SETTING       0x3A

#ifdef EMBER_AF_PLUGIN_HUMIDITY_SI7021
#define DEFAULT_MEASUREMENT_ACCURACY \
  EMBER_AF_PLUGIN_HUMIDITY_SI7021_MEASUREMENT_ACCURACY
#endif

static uint8_t measurementReadyDelayMs = ADC_12_BIT_CONV_TIME_MS;
static bool initialized = false;
static bool measureInprogress = false;
static bool startReadBeforeInitialization = false;
static uint8_t humidityAccuracySet(uint8_t measureAccuracy);
void si7021Init(void);
uint8_t si7021StartRead(void);
void halTemperatureInit(void);
void halHumidityInit(void);
void halHumidityStartRead(void);
void halTemperatureStartRead(void);

//Added on 10/28 -chinmay.shah@colorado.edu
//void readTemperature(void);
#endif /* SI7021_EFR32_HUMIDITY_TEMPERATURE_SI7021_H_ */

