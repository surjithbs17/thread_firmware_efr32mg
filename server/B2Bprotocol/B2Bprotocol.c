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

//Convert Packet into String
char *  formpacket(packet sendPack){
	static char sendMessage[500];
	static EmberIpv6Address local;
	//form the packet
	sprintf(sendMessage,"%c %d %d %d %d %d %d %c",
				START_BYTE,//$
				sendPack.mainSourceType,//Source
				sendPack.commandType,
				sendPack.interfaceType,
				sendPack.subinterfaceType,
				sendPack.interfaceID,
				//sendPack.destnIPAddress,
				//sendPack.srcIPAddress,
				sendPack.data,
				STOP_BYTE
			);//convert into a single packet
	emberAfCorePrintln("Message formed %s",sendMessage);
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
					//splitPacket.destnIPAddress,
					//splitPacket.srcIPAddress,
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

	emberAfCorePrintln("Packet contents=>\n mainsrc=>%d command=>%d \n interface=>%d \n subinter=>%d \n interID=>%d\n data=>%d \n",
					printPacket.mainSourceType,
					printPacket.commandType,
					printPacket.interfaceType,
					printPacket.subinterfaceType,
					printPacket.interfaceID,
					//printPacket.destnIPAddress,
					//printPacket.srcIPAddress,
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

void ReplyCommand(packet RxPacket){
	int32_t data=0;
	uint8_t i=0,ClientNotFound;

	emberAfCorePrintln("Type of Interface %d",RxPacket.interfaceType);

	//if(error_type == packet_success){	//Check the Type of Sensor

		switch(RxPacket.interfaceType){
			case(interface_type_Actuator):
									emberAfCorePrintln("Actuator");
									//data=ReadSensor(RxPacket.subinterfaceType);
									break;
			case(interface_type_Sensor):
									emberAfCorePrintln("Sensor");
									//data=ReadSensor(RxPacket.subinterfaceType);
									break;
			case(interface_type_IP):
									emberAfCorePrintln("In IP");

									break;

			default:
					emberAfCorePrintln("Incorrect Interface type ");
		}

	//}


}

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


	//copy the packet data that remains same
	if(error_type == packet_success){

		//copy initial same contents of Rx to Tx
			TxPacket.commandType=RxPacket.commandType;
			TxPacket.mainSourceType =RxPacket.mainSourceType;
			TxPacket.interfaceType=RxPacket.interfaceType;
			TxPacket.subinterfaceType=RxPacket.subinterfaceType;
			TxPacket.interfaceID=RxPacket.interfaceID;
			TxPacket.data=RxPacket.data;
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
							ReplyCommand(RxPacket);
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
		printPacket(TxPacket);
		TxMessage=formpacket(TxPacket);
		emberAfCorePrintln("MFPI:%s",TxMessage);
	}





	emberAfCorePrintln("Completed the Frame Analysis");

	return TxPacket;
}
