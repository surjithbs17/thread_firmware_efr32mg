/*
 * B2Bprotocol.c
 *
 *  Created on: Nov 21, 2016
 *      Author: Chinmay Shah
 *
 *      Assumption
 *      1) Different packet contents segregated with blank space
 *
 *Start Byte|Main Source    |Command      |interface    |sub interface     |interface Id        |IP         |data                |STOP
	$        0 - Ras Pi	      Read - 0     Sensor -0	 Sensor_temp -0     Sensor#0	        <Optional>   <Sensor Read Data>   '#'
             1 - Client       Reply -1	   Act -1		 Sensor_hum  -1
             2 - Server       Write -2	   Acquire IP -2 Act         -2
             /Bouder Router   Execute -3

 *
 *
 *
 *
 *
 *
 */
#include "../B2Bprotocol/B2Bprotocol.h"
#include "sensor-control/inc/sensor_control.h"
#include <string.h>
#include <thread-bookkeeping.h>
#include <stdio.h>
#include "hal/hal.h"
#include "thread-debug-print.h"

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_HAL
#include EMBER_AF_API_COMMAND_INTERPRETER2
#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#endif
#include EMBER_AF_API_TFTP
#include EMBER_AF_API_TFTP_BOOTLOADER
#ifdef CORTEXM3_EFM32_MICRO
  #include "tempdrv.h"
#endif

static EmberIpv6Address ClientIP;

char *  formpacket(packet sendPack){
	static char sendMessage[500];
	sprintf(sendMessage,"%c %d %d %d %d %d %d %c",
				START_BYTE,//
				sendPack.mainSourceType,
				sendPack.commandType,
				sendPack.interfaceType,
				sendPack.subinterfaceType,
				sendPack.interfaceID,
				sendPack.data,
				STOP_BYTE
			);//convert into a single packet
	emberAfCorePrintln("Form packet Message %s",sendMessage);
	return sendMessage;

}

packet splitPacket(char inputMessage[]){
	packet splitPacket;
	sscanf(inputMessage,"%c %d %d %d %d %d %d %c",
					&splitPacket.start,
					&splitPacket.mainSourceType,
					&splitPacket.commandType,
					&splitPacket.interfaceType,
 					&splitPacket.subinterfaceType,
					&splitPacket.interfaceID,
					&splitPacket.data,
					&splitPacket.stop
				);//convert into a single packet

	emberAfCorePrintln("Split complete ");
	printPacket(splitPacket);
	return splitPacket;
}


void printPacket (packet printPacket){

	emberAfCorePrintln("Packet contents=>\n src=>%d source=>%d command=>%d \n interface=>%d \n subinter=>%d \n interID=>%d \n \n data=>%d \n",
					printPacket.mainSourceType,
					printPacket.commandType,
					printPacket.interfaceType,
					printPacket.subinterfaceType,
					printPacket.interfaceID,
					printPacket.data
				);//convert in
}


uint32_t ReadSensor(SubInterface_TypeDef subinterface){
	uint32_t sensorData=0;
	emberAfCorePrintln("sub interface %d",subinterface);

	switch(subinterface){

		case(subinterface_type_Sensor_Temp):
				//fetchSensorData(SENSOR_TYPE_RH_Si7021)
			//Basic Calling Temperature and humidity measurement

				if ((sensorData=fetchSensorData(SENSOR_TYPE_TEMP_Si7021))==ERR_READ_SENSOR)
				{
					emberAfCorePrintln("Error in measurement");

				}
				else
				{
					emberAfCorePrintln("Temperature Measured %d",sensorData);
				}

				break;
		case(subinterface_type_Sensor_Hum):

				if ((sensorData=fetchSensorData(SENSOR_TYPE_RH_Si7021))==ERR_READ_SENSOR)
				{
					emberAfCorePrintln("Error in measurement");
				}
				else
				{
					emberAfCorePrintln("Humidity Measured %d",sensorData);
				}

				break;

		default:
				emberAfCorePrintln("Temperature Error ");



	}


	return sensorData;
}


uint32_t ReadCommand(packet RxPacket, PacketStatus_TypeDef error_type){
		int32_t retdata=0;

		emberAfCorePrintln("Type of Interface %d",RxPacket.interfaceType);

		if(error_type == packet_success){	//Check the Type of Sensor

			switch(RxPacket.interfaceType){
				case(interface_type_Actuator):
										emberAfCorePrintln("Actuator");
										//data=ReadSensor(RxPacket.subinterfaceType);
										break;
				case(interface_type_Sensor):
										emberAfCorePrintln("Sensor");
										retdata=ReadSensor(RxPacket.subinterfaceType);
										break;
				default:
						error_type=packet_interface_err;
						emberAfCorePrintln("Default choice ");
			}

		}


		return retdata;
}


//start|source type|commandType|interfaceType|subinterfaceType|interfaceID|destnIPAddress|srcIPAddress|data|stop
packet FrameAnalysis(char RxMessage[]){

	packet RxPacket;
	packet TxPacket;
	PacketStatus_TypeDef error_type=packet_success;
	char *TxMessage;

	// Message Received
	emberAfCorePrintln("In Frame Analysis");
	emberAfCorePrintln("Split %s",RxMessage);
	//split the packet received
	RxPacket=splitPacket(RxMessage);


	//emberAfCorePrintln("Print Rx packet");
	//printPacket(RxPacket);//print the ex packet
	//copy the packet data that remains same
	if(error_type == packet_success){

		//copy initial same contents of Rx to Tx
			TxPacket.commandType=RxPacket.commandType;
			TxPacket.interfaceType=RxPacket.interfaceType;
			TxPacket.subinterfaceType=RxPacket.subinterfaceType;
			TxPacket.interfaceID=RxPacket.interfaceID;
			//strcpy(TxPacket.destnIPAddress,RxPacket.srcIPAddress);
			//strcpy(TxPacket.srcIPAddress,RxPacket.destnIPAddress);

	}

	//emberAfCorePrintln("Print Rx packet");
	//printPacket(RxPacket);//print the ex packet
	if (RxPacket.mainSourceType==source_type_RasPi){
		emberAfCorePrintln("Message from Rasberry Pi");
		TxPacket.mainSourceType=source_type_Server;
		TxPacket.data=0;
		//strcpy(RxPacket.srcIPAddress,ClientListIPS[0]);
	}
	else if (RxPacket.mainSourceType==source_type_Server)
	{
		//Print the split Message
		emberAfCorePrintln("Command Type %d",RxPacket.commandType);
		//emberAfCorePrintln("Command Type int form %d  ... %d",(int)RxPacket.commandType,((int)RxPacket.commandType-48));
		switch(RxPacket.commandType){
		//switch(((int)RxPacket.commandType-48)){
			case(Read_cmd):
							if(RxPacket.interfaceType==(interface_type_IP)){
									emberAfCorePrintln("Read IP");
									TxPacket.commandType=Reply_cmd;
									TxPacket.data=0;//dummy data
							}else//other type where only sensor read is conducted
							{
								TxPacket.data=ReadCommand(RxPacket,error_type);
							}
							break;
			case(Reply_cmd):
							emberAfCorePrintln("Call Reply");
							break;
			case(Write_cmd):
							emberAfCorePrintln("Call Write");
							break;

			case(Execute_cmd):
							emberAfCorePrintln("Call Execute Command");
							break;
			default:
							error_type=packet_command_err;
							emberAfCorePrintln("Can't decode Command Type ");
		}
		TxPacket.mainSourceType=source_type_Client;
	}
	else if (RxPacket.mainSourceType==source_type_Client){
		// message from client to send to PI
		emberAfCorePrintln("Message from Client");
		TxPacket.mainSourceType=source_type_Server;
		TxMessage=formpacket(TxPacket);
		emberAfCorePrintln("Server:%s",TxMessage);

	}


	emberAfCorePrintln("Completed the Frame Analysis");

	return TxPacket;
}
