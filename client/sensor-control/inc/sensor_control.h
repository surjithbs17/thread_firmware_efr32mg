/*
 * sensor_control.h
 *
 *  Created on: Oct 31, 2016
 *      Author: Chinmay
 */

#ifndef SENSOR_CONTROL_INC_SENSOR_CONTROL_H_
#define SENSOR_CONTROL_INC_SENSOR_CONTROL_H_


//Sensor
#define ERR_READ_SENSOR 255//
#define SENSOR_TYPE_TEMP_Si7021 0
#define SENSOR_TYPE_RH_Si7021	1


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


//functions
uint32_t readHumidity();
uint32_t readTemperature(unsigned char type);
void setHeater(unsigned char heaterON);
void initilizationSmartHome();
uint32_t fetchSensorData(char sensorType);



#endif /* SENSOR_CONTROL_INC_SENSOR_CONTROL_H_ */
