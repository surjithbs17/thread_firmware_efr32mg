/*
 * B2Bprotocl.h
 *
 *  Created on: Nov 22, 2016
 *      Author: Chinmay
 */

#ifndef B2BPROTOCOL_B2BPROTOCOL_H_
#define B2BPROTOCOL_B2BPROTOCOL_H_


#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_HAL
#include "thread-bookkeeping.h"


#define EMBER_IPV6_ADDRESS_STRING_SIZE 40
//#define startTypeLenght 2
//define commandTypeLenght 2
//#define interfaceTypeLenght 3
//#define sensorTypeLenght 3
//#define sensorIDLenght
#define dataLenght 2
//define stopTypeLenght 2

#define START_BYTE '$'
#define STOP_BYTE '#'
#define MAXPACKETSIZE 100
#define MAXIPSIZE 20

//Sensor IDs supported
#define SENSOR_DEFAULT_ID 0x00
#define SENSOR_ID0 0x00
#define SENSOR_ID1 0x01
#define SENSOR_ID2 0x02
#define SENSOR_ID3 0x03
#define SENSOR_ID4 0x04

//structure from frame for packet data (Response)
typedef struct packetFormat{
	unsigned char start;
	unsigned char mainSourceType;
	unsigned char commandType;
	unsigned char interfaceType;
	unsigned char subinterfaceType;
	unsigned char interfaceID;
	uint8_t destnIPAddress[EMBER_IPV6_ADDRESS_STRING_SIZE];
	uint8_t srcIPAddress[EMBER_IPV6_ADDRESS_STRING_SIZE];
	uint32_t data;
	unsigned char stop;
}packet;

//Source Type
typedef enum SourceTypeEnum{
	 source_type_RasPi,//Source is Rasberry Pi
	 source_type_Client,//Source is Client
	 source_type_Server//Source is Server

}SourceTypeDef;


// Each packet data  Location
typedef enum PacketLocationEnum{
	loc_packet_start,
	loc_cmd_type,
	loc_interface_type,
	loc_sub_interface_type,
	loc_interface_ID,
	loc_dest_IPAddress,
	loc_src_IPAddress,
	loc_data,
	stop_loc
}PacketLocation_TypeDef;
//	unsigned char subinterfaceType;


//Packet Type
typedef enum PacketTypeEnum{
	 packet_type_read,
	 packet_type_reply,
	 packet_type_command,
	 packet_type_write
}Packet_TypeDef;


//Interface Type
typedef enum SubInterfaceTypeEnum{
	  subinterface_type_Sensor_Temp,
	  subinterface_type_Sensor_Hum,
	  subinterface_type_Actuator,
	  subinterface_type_Actuator_LED

}SubInterface_TypeDef;

//Interface Type
typedef enum InterfaceTypeEnum{
	interface_type_Sensor,
	interface_type_Actuator,
	interface_type_IP

}Interface_TypeDef;

//Command Type supported
typedef enum CommandType{
	Read_cmd,
	Reply_cmd,
	Write_cmd,
	Execute_cmd

 }CommandSupport_TypeDef;


 //Sensor Type supported
 typedef enum SensorType{
	 Si7021_temperature,
	 Si7021_Humidity,
	 Actuator_Light,


 }SensorSupport_TypeDef;

typedef enum packetStatus{
  //PACKET DCODE
  packet_success,
  packet_err,
  packet_command_err,
  packet_interface_err,
  packet_subinterface_err,
  packet_interfaceId_err
 }PacketStatus_TypeDef;

//Control and status of Actuator/Relay
typedef enum actuatorStatus{
   //PACKET DCODE
   act_ON,//Actuator is ON
   act_OFF,//Actuator is OFF
   act_ERR//Actuator ERROR

}actuatorStatus_TypeDef;

//Function Declarations

void printPacket (packet printPacket);
packet FrameAnalysis(char *RxMessage);
packet splitPacket(char *inputMessage);
uint32_t ReadSensor(SubInterface_TypeDef subinterfaceType);
uint32_t ReadCommand(packet RxPacket, PacketStatus_TypeDef error_type);
char *  formpacket(packet sendPack);

uint8_t  ExecuteCommand(packet RxPacket, PacketStatus_TypeDef error_type);
uint8_t actuateSubinterface(packet RxPacket);

#endif /* B2BPROTOCOL_B2BPROTOCOL_H_ */

