/*
 * B2Bprotocol.c
 *
 *  Created on: Nov 21, 2016
 *      Author: Chinmay Shah
 *
 *      Assumption
 *      1) Different packet contents segregated with blank space
 *
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
//#include "app/thread/plugin/udp-debug/udp-debug.c"



// If printing is enabled, we will print some diagnostic information about the
// most recent reset and also during runtime.  On some platforms, extended
// diagnostic information is available.

//  #define PRINT_RESET_INFORMATION()
//  #define emberAfGuaranteedPrint(...)
//  #define emberAfCorePrintln(...)


//packet TxPacket;

char *  formpacket(packet sendPack){
	//unsigned char sendMessage[MAXPACKETSIZE];
	static char sendMessage[500];
	sprintf(sendMessage,"%c %d %d %d %d %s %s %d %c",
				START_BYTE,//
				sendPack.commandType,
				sendPack.interfaceType,
				sendPack.subinterfaceType,
				sendPack.interfaceID,
				sendPack.destnIPAddress,
				sendPack.srcIPAddress,
				sendPack.data,
				STOP_BYTE
			);//convert into a single packet

	//emberAfCorePrintln("Message formed %s",tempMessage);
	//strcpy(sendMessage,tempMessage);
	emberAfCorePrintln("Message formed %s",sendMessage);

	return sendMessage;

}


packet splitPacket(char inputMessage[]){

	/*
	char* value;
	char* string;
	char* concat = "Hello World";

	//sscanf(concat,"%s %s",value,string);
	//sprintf()
	*/
	packet splitPacket;

	char s1[100],s2[100],s3[100];
	char inMessage[20] ="LOW POWER SYSTEM";
	//emberAfCorePrintln("in Split ext%s",inputMessage);
	//emberAfCorePrintln("in Split int%s",inMessage);

	//sscanf(inputMessage,"%s %s %d %d",splitPacket.destnIPAddress,splitPacket.srcIPAddress,&splitPacket.commandType,&splitPacket.interfaceID);
	//sscanf(inputMessage,"%s|%s",&s1,&s2);
	//sscanf(inMessage,"%s %s %s",s1,s2,s3);


	//emberAfCorePrintln("after Split Ex 1=>%s 2=>%s integers 3 %d 4%d",splitPacket.destnIPAddress,splitPacket.srcIPAddress,splitPacket.commandType,splitPacket.interfaceID);
	//emberAfCorePrintln("After Split In s1>%s,s2>%s s3>%s",s1,s2,s3,0);
	//char messageRX[100]="0 0 0 0 234:234:234:29 234:234:2342:22 30";
	sscanf(inputMessage,"%c %d %d %d %d %s %s %d %c",
					&splitPacket.start,
					&splitPacket.commandType,
					&splitPacket.interfaceType,
 					&splitPacket.subinterfaceType,
					&splitPacket.interfaceID,
					splitPacket.destnIPAddress,
					splitPacket.srcIPAddress,
					&splitPacket.data,
					&splitPacket.stop
				);//convert into a single packet

	/*
	else
	{
		emberAfCorePrintln("Success print ",0);
		printPacket(splitPacket);
	}
	*/
	emberAfCorePrintln("Split complete ");
	printPacket(splitPacket);
	return splitPacket;
}


void printPacket (packet printPacket){

	char startByte,stopByte;

	emberAfCorePrintln("Packet contents=>\n command=>%d \n interface=>%d \n subinter=>%d \n interID=>%d \n destIP=>%s \n srcIP=>%s \n data=>%d \n",
					printPacket.commandType,
					printPacket.interfaceType,
					printPacket.subinterfaceType,
					printPacket.interfaceID,
					printPacket.destnIPAddress,
					printPacket.srcIPAddress,
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
		int32_t data=0;
		emberAfCorePrintln("Type of Interface %d",RxPacket.interfaceType);

		if(error_type == packet_success){	//Check the Type of Sensor

			switch(RxPacket.interfaceType){
				case(interface_type_Actuator):
										emberAfCorePrintln("Actuator");
										//data=ReadSensor(RxPacket.subinterfaceType);
										break;
				case(interface_type_Sensor):
										emberAfCorePrintln("Sensor");
										data=ReadSensor(RxPacket.subinterfaceType);
										break;
				default:
						error_type=packet_interface_err;
						emberAfCorePrintln("Default choice ");
			}

		}


		return data;
}


packet FrameAnalysis(char RxMessage[]){

	packet RxPacket;
	packet TxPacket;
	PacketStatus_TypeDef error_type=packet_success;

	// Message Received
	emberAfCorePrintln("In Frame Analysis");
	emberAfCorePrintln("Split %s",RxMessage);
	//split the packet received
	RxPacket=splitPacket(RxMessage);


	emberAfCorePrintln("Print Rx packet");
	printPacket(RxPacket);//print the ex packet



	//Print the split Message
	emberAfCorePrintln("Command Type %c",RxPacket.commandType);
	//emberAfCorePrintln("Command Type int form %d  ... %d",(int)RxPacket.commandType,((int)RxPacket.commandType-48));
	switch(RxPacket.commandType){
	//switch(((int)RxPacket.commandType-48)){
		case(Read_cmd):
						emberAfCorePrintln("Call Read");
						TxPacket.data=ReadCommand(RxPacket,error_type);
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




	//copy the packet if packet decode is success
	if(error_type == packet_success){

		//copy initial same contents of Rx to Tx
			TxPacket.commandType=RxPacket.commandType;
			TxPacket.interfaceType=RxPacket.interfaceType;
			TxPacket.subinterfaceType=RxPacket.subinterfaceType;
			TxPacket.interfaceID=RxPacket.interfaceID;
			strcpy(TxPacket.destnIPAddress,RxPacket.srcIPAddress);
			strcpy(TxPacket.srcIPAddress,RxPacket.destnIPAddress);

	}
	emberAfCorePrintln("Completed the Frame Analysis");

	return TxPacket;
}
