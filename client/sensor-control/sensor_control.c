/*
 * sensor_control.c
 *
 *  Created on: Oct 31, 2016
 *      Author: Chinmay
 */



#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_HAL
#include "thread-bookkeeping.h"
#include "si7021/efr32_inc/humidity-temperature-si7021.h"
#include "i2c-driver/efr32_inc/i2c-driver.h"
#include "inc/sensor_control.h"

uint32_t readHumidity(){

	uint8_t errorCode=10;
	uint32_t relativeHumidity=0;
	//float relativeHumidity=0;
	uint8_t buffer[DEFAULT_READ_BUFFER_SIZE];

	//we started read before initilization, so we did not send any measurement command.
	//so we redo that again here
	  if (startReadBeforeInitialization) {
	    startReadBeforeInitialization = false;
	    return ERR_READ_SENSOR;
	  } else {
	    //emberEventControlSetInactive(  emberAfPluginHumiditySi7021ReadEventControl);

	    errorCode = halI2cReadBytes(SI_7021_ADDR,&buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],READ_BYTE_SIZE_WITH_CHECKSUM);
	    if (errorCode == I2C_DRIVER_ERR_NONE) {
	     //emberAfGuaranteedPrintln(" Humidity measurement LSB : MSB %ld:%ld ,\n Error Code: %d",buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_1],errorCode,0);
		  //halHumidityReadingCompleteCallback(0, false);
		  // return;
	      relativeHumidity=0;
	      //emberAfGuaranteedPrintln("Before value (RAW) = %5d",relativeHumidity, 0);
	      relativeHumidity = HIGH_LOW_TO_INT(buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],buffer[SI_7021_READ_BUFFER_LOCATION_BYTE_1]);
		    //Relative Humidity in % is calculated as the following:
	      	//let D = data from Si7021;
			//%RH = ((125 * D) / 65536) - 6;
			//
			//to calculate the Relative Humidity in Centi Percentage:
			//Centi%RH = (((125 * D) / 65536) - 6) * 100
			//         = (((100 * 125 * D) / 65536) - 600);
	      /*
	  	    relativeHumidity = relativeHumidity * 100;
		    relativeHumidity = relativeHumidity * 125;
		    relativeHumidity = relativeHumidity >> 16;
		    relativeHumidity = relativeHumidity - 600;
			measureInprogress = false;
			emberAfGuaranteedPrintln("Measure humidity %5f % ",relativeHumidity,0);
	       */
			  //emberAfGuaranteedPrintln("Humidity Value (RAW) = %5d",relativeHumidity, 0);
			  // Adjusted formula
			  relativeHumidity = 125 * relativeHumidity;//
			  //emberAfGuaranteedPrintln("Humidity Value (*125) = %5d",relativeHumidity, 0);
			  relativeHumidity = (relativeHumidity >> 16);
			  //emberAfGuaranteedPrintln("Humidity (/655356) = %5d",relativeHumidity, 0);
			  relativeHumidity -= 6;
			  emberAfGuaranteedPrintln("Humidity Measured = %5d %%",relativeHumidity, 0);
			  measureInprogress = false;
			  return relativeHumidity;
			//halHumidityReadingCompleteCallback((uint16_t)relativeHumidity, true);
	    }
	    else
	    {
	    	emberAfGuaranteedPrintln("Error in measurement  Humidity Code %d  ",errorCode,0);
	    	return ERR_READ_SENSOR;
	    }
	  }
}



/**************************************************************
*Read Temperature Reading
*	Added for normal usage , without pulgins and call backs
*
*	Measuring Temperature (depend on type )
*
****************************************************************/

uint32_t readTemperature(unsigned char type)
{
	uint8_t errorCode=0;
	uint8_t tempcommand=0;
	uint8_t readBuffer[DEFAULT_READ_BUFFER_SIZE];
	uint32_t tempCal=0;
	uint32_t temperatureMilliC =0;
	int check =0;
    	//Sequence for reading the temperature data
		readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0]=0;
		readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_1]=0;
		if(!type){
			tempcommand = SI_7021_READ_TEMPERATURE;//Set the command to read only after running humidity measurement
		}
		else
		{
			tempcommand = SI_7021_MEASURE_TEMPERATURE;//Set the command to measure and read the temperature
		}
		errorCode = halI2cWriteBytes(SI_7021_ADDR, &tempcommand, SEND_COMMAND_SIZE);// Write on for address of temperature sensor
	    if (errorCode == I2C_DRIVER_ERR_NONE) {// if no error in I2C
	    //halTemperatureReadingCompleteCallback(0, false);
		//emberAfGuaranteedPrintln("Reading from temp sensor ,Errcode %d",errorCode, 1);
		errorCode = halI2cReadBytes(SI_7021_ADDR,&readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],READ_BYTE_SIZE_TEMPERATURE);
		if (errorCode == I2C_DRIVER_ERR_NONE) {
		 //halTemperatureReadingC
		 //completeCallback(SI_7021_READ_BUFFER_LOCATION_BYTE_0,true);
		 //emberAfGuaranteedPrintln("Raw Temp Sensor LSB:MSB %d : %d Errorcode %d \n",readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_1],readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],errorCode, 0);
		}
		else
		{
			emberAfGuaranteedPrintln("Error in measurement temp Code %d  ",errorCode,1);
			return ERR_READ_SENSOR;
		}

		  //temperature (milliC) = 1000 * (175.72 * ADC /65536 - 46.85)
		  //                     = (1000 * ( 5624 * ADC / 8192 - 11994 )) / 256
		  tempCal=0;
		  //emberAfGuaranteedPrintln("Before value (RAW) = %5d",tempCal, 0);
		  tempCal = HIGH_LOW_TO_INT(readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_1]);

		  //emberAfGuaranteedPrintln("Raw Temp Sensor Exchange  MSB:LSB %d : %d",readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_1],readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0], 0);
		 /*
		  emberAfGuaranteedPrintln("Temp Raw: %d",tempCal);
		  tempCal = 5624 * tempCal;//5624 * ADC
		  emberAfGuaranteedPrintln("Temp (5624): %d \n",tempCal);
		  tempCal = tempCal >> 13;// shift by 8192, 2^13
		  emberAfGuaranteedPrintln("Temp div(8192): %d",tempCal);
		  tempCal = tempCal - 11994;//- 11994
		  emberAfGuaranteedPrintln("Temp sub(11994):%d",tempCal);
		  tempCal = (tempCal * 1000) >> 8;//multiple by 1000 and divide by 256
		  emberAfGuaranteedPrintln("Temp mul by 1000/256: %d",tempCal);
		*/

		  //emberAfGuaranteedPrintln("Temp Value (RAW) = %5d",tempCal, 0);
		  // Adjusted formula
		  tempCal = 176 * tempCal;//
		  //emberAfGuaranteedPrintln("Temp Value (*176) = %5d",tempCal, 0);
		  tempCal = (tempCal >> 16);
		  //emberAfGuaranteedPrintln("Temp Value (/655356) = %5d",tempCal, 0);
		  tempCal -= 47;
		  //emberAfGuaranteedPrintln("Temp Value (-47) = %5d",tempCal, 0);
		  temperatureMilliC = tempCal;
		  emberAfGuaranteedPrintln("Temperature = %5d C",temperatureMilliC, 0);
		  return temperatureMilliC;
		}
	    else
		{
			emberAfGuaranteedPrint("Error in measurement  temp Code %d  ",errorCode,0);
			return ERR_READ_SENSOR;
		}
}


void setHeater(unsigned char heaterON)
{
	uint8_t errorCode=0;
	uint8_t tempCommand=0,tempCommandValue=0;
	uint8_t writeBuffer[DEFAULT_WRITE_BUFFER_SIZE];
	uint8_t readBuffer[DEFAULT_READ_BUFFER_SIZE];
	uint32_t tempCal=0;
	uint32_t temperatureMilliC =0;
	int check=0;
    	///Read the user register value
		tempCommand = SI_7021_USER_REG_READ;
		errorCode = halI2cWriteBytes(SI_7021_ADDR, &tempCommand, SEND_COMMAND_SIZE);// Address for reading  the user register value
		if(errorCode == I2C_DRIVER_ERR_NONE)
		{
			//read from user reg
			emberAfGuaranteedPrintln("Try to read user value" ,0);
			errorCode = halI2cReadBytes(SI_7021_ADDR,&readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],READ_BYTE_SIZE_USER_REGISTER);
			if(errorCode == I2C_DRIVER_ERR_NONE)
			{
				emberAfGuaranteedPrintln("read user reg ",0);
				//update the value for heater on or off
				tempCommandValue = readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0];
				if(heaterON){

						tempCommandValue |= 0x04;//Set the command to read only after running humidity measurement
						emberAfGuaranteedPrintln("Set On heater in Function= %d ",tempCommandValue,0);
					}
					else
					{
						tempCommandValue &= ~(0x04);//Set the command to measure and read the temperature
						emberAfGuaranteedPrintln("Set Off heater in Function =%d ",tempCommandValue,0);
					}
			}
			else
			{
				emberAfGuaranteedPrintln("Error in Reading the User Register Code %d  ",errorCode,0);
				return;
			}
		}
		else
		{
			emberAfGuaranteedPrintln("Error in Addressing  the User Register Code %d  ",errorCode,0);
			return;
		}

		//Set the user register for heater to ON
		writeBuffer[0] = SI_7021_USER_REG_WRITE;// set the type
		writeBuffer[1] = tempCommandValue;//set the data to be written
		errorCode = halI2cWriteBytes(SI_7021_ADDR, &writeBuffer[0], SEND_COMMAND_WITH_VALUE_SIZE);// Write on for address of temperature sensor with a data
	    if (errorCode == I2C_DRIVER_ERR_NONE) {// if no error in I2C
			emberAfGuaranteedPrintln("Set Heater Function ,Errcode: %d",errorCode, 0);
			/*errorCode = halI2cReadBytes(SI_7021_ADDR,&readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],READ_BYTE_SIZE_TEMPERATURE);
			if (errorCode == I2C_DRIVER_ERR_NONE) {
			 //halTemperatureReadingC
			 //completeCallback(SI_7021_READ_BUFFER_LOCATION_BYTE_0,true);
			 emberAfGuaranteedPrintln("Raw Temp Sensor LSB:MSB %d : %d Errorcode %d \n",readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_1],readBuffer[SI_7021_READ_BUFFER_LOCATION_BYTE_0],errorCode, 0);*/
		}
		else
		{
			emberAfGuaranteedPrintln("Error in Setting Heater Code %d  ",errorCode,0);
		}
}



void initilizationSmartHome()
{

	  //Initialize I2C
	   halI2cInitialize();
	   //halSetLed(BOARDLED0);
	   emberAfGuaranteedPrintln("I2C Initialized", 0);

	  //Initialize Temperature and humidity
	   //halTemperatureInit();
	   halHumidityInit();
	   InitRelayPins();//Initialize Actuator
	   halToggleLed(BOARDLED0);
}


void startSensors()
{
	halHumidityStartRead();
}




uint32_t fetchSensorData(char sensorType){

	uint32_t returnValue=0;
	emberAfGuaranteedPrintln("In Sensor Read Type %d",sensorType, 0);

	switch(sensorType){
	case SENSOR_TYPE_RH_Si7021 :

								halHumidityStartRead();
								halCommonDelayMilliseconds(100);
								halHumidityStartRead();
								halCommonDelayMilliseconds(100);
								returnValue=readHumidity();
								break;

	case SENSOR_TYPE_TEMP_Si7021:

								halTemperatureStartRead();
								halCommonDelayMilliseconds(100);
								halTemperatureStartRead();
								halCommonDelayMilliseconds(100);
								returnValue=readTemperature(0);
								break;
	default:
								returnValue = ERR_READ_SENSOR;
								break;
	}

	return returnValue;

}











