// Copyright 2015 Silicon Laboratories, Inc.

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
//#include <server-implementations.h>

#define ALIAS(x) x##Alias
#include <stdio.h>
#include <stdlib.h>
#include "sensor-control/inc/sensor_control.h"
#include <../B2Bprotocol/B2Bprotocol.h>
#include "app/thread/plugin/udp-debug/udp-debug.c"
//#include "network-management-cli/network-management-cli.c"
//#include "actuator_control/actuator_control.h"
// WARNING: By default, this sample application uses the well-know sensor/sink
// network key as the master key.  This is done for demonstration purposes, so
// that packets will decrypt automatically in Simplicity Studio.  Using
// predefined keys is a significant security risk.  Real devices MUST use
// random keys.  The stack automatically generates random keys if
// emberSetSecurityParameters is not called prior to forming.
//#define USE_RANDOM_MASTER_KEY
#ifdef USE_RANDOM_MASTER_KEY
  #define SET_SECURITY_PARAMETERS_OR_FORM_NETWORK FORM_NETWORK
#else
  static EmberKeyData masterKey = {
    {0x65, 0x6D, 0x62, 0x65, 0x72, 0x20, 0x45, 0x4D,
     0x32, 0x35, 0x30, 0x20, 0x63, 0x68, 0x69, 0x70,}
  };
  #define SET_SECURITY_PARAMETERS_OR_FORM_NETWORK SET_SECURITY_PARAMETERS
#endif

// The client/server sample applications use a fixed network id to simplify
// the join process.
static const uint8_t networkId[EMBER_NETWORK_ID_SIZE] = "client/server";

static const uint8_t commissionerId[] = "server";

static void resumeNetwork(void);
#ifndef USE_RANDOM_MASTER_KEY
	static void setSecurityParameters(void);
#endif
static void formNetwork(void);
static void getCommissioner(void);
static void becomeCommissioner(void);
static void makeAllThreadNodesAddress(void);
static void advertise(void);
static void resetNetworkState(void);

#define ADVERTISEMENT_PERIOD_MS (60 * MILLISECOND_TICKS_PER_SECOND)
#define MAXCLIENTSUPPORTED 10 // Maximum Clients supported at a time
EmberIpv6Address ClientIP[MAXCLIENTSUPPORTED];

// The All Thread Nodes multicast address is filled in once the ULA prefix is
// known.  It is an RFC3306 address with the format ff33:40:<ula prefix>::1.
static EmberIpv6Address allThreadNodes = {
  {0xFF, 0x33, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,}
};
//As of now multicast address then specific to client addresss

#define RFC3306_NETWORK_PREFIX_OFFSET 4

#define MAXCLIENTS 10


static const uint8_t serverAdvertiseUri[] = "server/advertise";
static const uint8_t serverReportUri[] = "server/report";

uint8_t ClientCounts=0;
//EmberIpv6Address ClientListIPS[MAXCLIENTS];
uint8_t ClientListIPS[MAXCLIENTS][EMBER_IPV6_ADDRESS_STRING_SIZE];
//EmberIpv6Address ClientListIPS[MAXCLIENTS];

enum {
  INITIAL                       = 0,
  RESUME_NETWORK                = 1,
  SET_SECURITY_PARAMETERS       = 2,
  FORM_NETWORK                  = 3,
  GET_COMMISSIONER              = 4,
  BECOME_COMMISSIONER           = 5,
  MAKE_ALL_THREAD_NODES_ADDRESS = 6,
  ADVERTISE                     = 7,
  RESET_NETWORK_STATE           = 8,
  ACQUIRE_CLIENT_IP				= 9
  //APPLICATION					= 10
};
static uint8_t state = INITIAL;
EmberEventControl stateEventControl;
static void setNextStateWithDelay(uint8_t nextState, uint32_t delayMs);
static void application();
#define setNextState(nextState)       setNextStateWithDelay((nextState), 0)
#define repeatStateWithDelay(delayMs) setNextStateWithDelay(state, (delayMs))

static const uint8_t *printableNetworkId(void);





bool emIsDefaultGlobalPrefix(const uint8_t *prefix);



static void processClientDataAck(EmberCoapStatus status,
                                 EmberCoapMessage *coap,
                                 void *appData,
                                 uint16_t appDatalength)
{
  // We track the success or failure of reports so that we can determine when
  // we have lost the server.  A series of consecutive failures is the trigger
  // to detach from the current server and find a new one.  Any successfully-
  // transmitted report clears past failures.

}


void emberAfNetworkStatusCallback(EmberNetworkStatus newNetworkStatus,
                                  EmberNetworkStatus oldNetworkStatus,
                                  EmberJoinFailureReason reason)
{
  // This callback is called whenever the network status changes, like when
  // we finish forming a new network or when we lose connectivity.  If we have
  // no network, we form a new one.  If we have a saved network, we try to
  // resume operations on that network.  When we are joined and attached to the
  // network, we establish ourselves as the commissioner and then begin
  // advertising.

  emberEventControlSetInactive(stateEventControl);

  switch (newNetworkStatus) {
  case EMBER_NO_NETWORK:
    if (oldNetworkStatus == EMBER_JOINING_NETWORK) {
      emberAfCorePrintln("ERR: Forming failed: 0x%x", reason);
    }
    setNextState(SET_SECURITY_PARAMETERS_OR_FORM_NETWORK);
    break;
  case EMBER_SAVED_NETWORK:
    setNextState(RESUME_NETWORK);
    break;
  case EMBER_JOINING_NETWORK:
    // Wait for either the "attaching" or "no network" state.
    break;
  case EMBER_JOINED_NETWORK_ATTACHING:
    // Wait for the "attached" state.
    if (oldNetworkStatus == EMBER_JOINED_NETWORK_ATTACHED) {
      emberAfCorePrintln("ERR: Lost connection to network");
    }
    break;
  case EMBER_JOINED_NETWORK_ATTACHED:
    emberAfCorePrintln("%s network \"%s\"",
                       (state == RESUME_NETWORK
                        ? "Resumed operation on"
                        : (state == FORM_NETWORK
                           ? "Formed"
                           : "Rejoined")),
                       printableNetworkId());
    setNextState(GET_COMMISSIONER);
    break;
  case EMBER_JOINED_NETWORK_NO_PARENT:
    // We always form as a router, so we should never end up in the "no parent"
    // state.
    assert(false);
    break;
  default:
    assert(false);
    break;
  }
}

static void resumeNetwork(void)
{
  assert(state == RESUME_NETWORK);

  emberAfCorePrintln("Resuming operation on network \"%s\"", printableNetworkId());

  emberResumeNetwork();
}

void emberResumeNetworkReturn(EmberStatus status)
{
  // This return indicates whether the stack is going to attempt to resume.  If
  // so, the result is reported later as a network status change.  If we cannot
  // even attempt to resume, we just give up and try forming instead.

  assert(state == RESUME_NETWORK);

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Unable to resume: 0x%x", status);
    setNextState(SET_SECURITY_PARAMETERS_OR_FORM_NETWORK);
  }
}

#ifndef USE_RANDOM_MASTER_KEY
static void setSecurityParameters(void)
{
  // Setting a fixed master key is a security risk, but is useful for
  // demonstration purposes.  See the warning above.

  EmberSecurityParameters parameters = {0};

  assert(state == SET_SECURITY_PARAMETERS);

  emberAfCorePrint("Setting master key to ");
  emberAfCoreDebugExec(emberAfPrintZigbeeKey(masterKey.contents));

  parameters.networkKey = &masterKey;
  emberSetSecurityParameters(&parameters, EMBER_NETWORK_KEY_OPTION);
}
#endif

void emberSetSecurityParametersReturn(EmberStatus status)
{
#ifndef USE_RANDOM_MASTER_KEY
  // After setting our security parameters, we can move on to actually forming
  // the network.

  assert(state == SET_SECURITY_PARAMETERS);

  if (status == EMBER_SUCCESS) {
    setNextState(FORM_NETWORK);
  } else {
    emberAfCorePrint("ERR: Setting master key to ");
    emberAfCoreDebugExec(emberAfPrintZigbeeKey(masterKey.contents));
    emberAfCorePrintln(" failed:  0x%x", status);
    setNextState(SET_SECURITY_PARAMETERS);
  }
#endif
}

static void formNetwork(void)
{
  EmberNetworkParameters parameters = {{0}};

  assert(state == FORM_NETWORK);

  emberAfCorePrintln("Forming network \"%s\"", networkId);

  MEMCOPY(parameters.networkId, networkId, sizeof(networkId));
  parameters.nodeType = EMBER_ROUTER;
  parameters.radioTxPower = 3;

  emberFormNetwork(&parameters,
                   (EMBER_NETWORK_ID_OPTION
                    | EMBER_NODE_TYPE_OPTION
                    | EMBER_TX_POWER_OPTION),
                   EMBER_ALL_802_15_4_CHANNELS_MASK);
}

void emberFormNetworkReturn(EmberStatus status)
{
  // This return indicates whether the stack is going to attempt to form.  If
  // so, the result is reported later as a network status change.  Otherwise,
  // we just try again.

  assert(state == FORM_NETWORK);

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Unable to form: 0x%x", status);
    setNextState(SET_SECURITY_PARAMETERS_OR_FORM_NETWORK);
  }
}

static void getCommissioner(void)
{
  assert(state == GET_COMMISSIONER);

  emberGetCommissioner();
}

void emberCommissionerStatusHandler(uint16_t flags,
                                    const uint8_t *commissionerName,
                                    uint8_t commissionerNameLength)
{
  if (state == GET_COMMISSIONER) {
    if (flags == EMBER_NO_COMMISSIONER) {
      setNextState(BECOME_COMMISSIONER);
    } else {
      if (!READBITS(flags, EMBER_AM_COMMISSIONER)) {
        emberAfCorePrint("ERR: Network already has a commissioner");
        if (commissionerName != NULL) {
          emberAfCorePrint(": ");
          uint8_t i;
          for (i = 0; i < commissionerNameLength; i++) {
            emberAfCorePrint("%c", commissionerName[i]);
          }
        }
        emberAfCorePrintln("");
      }
      setNextState(MAKE_ALL_THREAD_NODES_ADDRESS);
    }
  }
}

static void becomeCommissioner(void)
{
  // We want to establishing ourselves as the commissioner so that we are in
  // charge of bringing new devices into the network.

  assert(state == BECOME_COMMISSIONER);

  emberAfCorePrintln("Becoming commissioner \"%s\"", commissionerId);

  emberBecomeCommissioner(commissionerId, sizeof(commissionerId));
}

void emberBecomeCommissionerReturn(EmberStatus status)
{
  // Once we are established as the commissioner, we move on to periodically
  // advertising our presence to other routers in the mesh.

  assert(state == BECOME_COMMISSIONER);

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Became commissioner");
    setNextState(MAKE_ALL_THREAD_NODES_ADDRESS);
  } else {
    emberAfCorePrintln("ERR: Becoming commissioner failed: 0x%x", status);
    setNextState(GET_COMMISSIONER);
  }
}


//Analysize the input packet and send to client requested
void rpiCommand(){

	uint8_t *messagFromUart;
	uint8_t messagFromUartLength;
	packet packetforClient;
	EmberStatus status;
	//packet RequestPack;
	char * messageSendtoClient;
	EmberIpv6Address destIP;
    int len;

    //message From RPi and Analysis
	messagFromUart = emberStringCommandArgument(0, &messagFromUartLength);
	emberAfCorePrintln("Command input %s",messagFromUart);
	packetforClient=FrameAnalysis(messagFromUart);
	messageSendtoClient=formpacket(packetforClient);

	len=strlen(messageSendtoClient);
	emberAfCorePrintln("Msg for Client %s",messageSendtoClient);
	//If a non IP Request dest is an IP
	if(packetforClient.interfaceType!=interface_type_IP){
		 emberIpv6StringToAddress(ClientListIPS[0],&destIP);
		 emberAfCorePrintln("IP %s",ClientListIPS[0]);
		 emberAfCoreDebugExec(emberAfPrintIpv6Address(&destIP));
		 //change the IP to a destination single IP then a
		 status = emberCoapPost(&destIP,serverReportUri,(const uint8_t *)messageSendtoClient,len, processClientDataAck);
	}
	else//else multi cast to all clients available
	{
	     emberAfCoreDebugExec(emberAfPrintIpv6Address(&destIP));
		 //change the IP to a destination single IP then a
		 status = emberCoapPost(&allThreadNodes,serverReportUri,(const uint8_t *)messageSendtoClient,len, processClientDataAck);
	}
	//

	 emberAfCorePrintln("Status 0x%x", status);
}

// expect <join key:1--16> [<eui64:8>]
void expectCommand(void)
{
  uint8_t joinKeyLength;
  uint8_t *joinKey;
  bool hasEui64;
  EmberEui64 eui64;

  joinKey = emberStringCommandArgument(0, &joinKeyLength);
  if (joinKeyLength > EMBER_ENCRYPTION_KEY_SIZE) {
    emberAfAppPrintln("%p: %p", "ERR", "invalid join key");
    return;
  }

  hasEui64 = (emberCommandArgumentCount() > 1);
  if (hasEui64) {
    emberGetEui64Argument(1, &eui64);
    emberSetJoinKey(&eui64, joinKey, joinKeyLength);
    emberSetJoiningMode(EMBER_JOINING_ALLOW_EUI_STEERING, 16);
    emberAddSteeringEui64(&eui64);
  } else {
    emberSetJoinKey(NULL, joinKey, joinKeyLength);
    emberSetJoiningMode(EMBER_JOINING_ALLOW_ALL_STEERING, 1);
  }
  emberSendSteeringData();
}

void emberSendSteeringDataReturn(EmberStatus status)
{
  // The steering data helps bring new devices into our network.

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Sent steering data");
  } else {
    emberAfCorePrintln("ERR: Sending steering data failed: 0x%x", status);
  }
}

static void makeAllThreadNodesAddress(void)
{
  EmberNetworkParameters parameters = {{0}};

  assert(state == MAKE_ALL_THREAD_NODES_ADDRESS);

  emberGetNetworkParameters(&parameters);
  MEMCOPY(allThreadNodes.bytes + RFC3306_NETWORK_PREFIX_OFFSET,
          &parameters.ulaPrefix,
          sizeof(EmberIpv6Prefix));

  setNextState(ADVERTISE);
}





/*


static EmberStatus application(void){
	static char applicationRuncount=0;
	EmberStatus status;
	char messagetemp="Test from Server";
	int len=strlen(messagetemp);
	emberAfCorePrint("IN application");
	if(applicationRuncount<5){
		repeatStateWithDelay(ADVERTISEMENT_PERIOD_MS);
	}
	status = emberCoapPost(&client,
	                        clientReportUri,
							(const uint8_t *)messagetemp,
	                         len,
	                         processClientDataAck);
	emberAfCorePrintln("Data to server 0x%x", status);
	setNextState(ADVERTISE);



}
*/
static EmberIpv6Address globalAddress;
static uint8_t globalAddressString[EMBER_IPV6_ADDRESS_STRING_SIZE] = {0};
static void emberGetDhcpClientReturn(const EmberIpv6Address *address){

	MEMCOPY(&globalAddress, address, sizeof(EmberIpv6Address));
	emberIpv6AddressToString(&globalAddress,
	                           globalAddressString,
	                           sizeof(globalAddressString));
	emberAfCorePrintln("Client address assigned %s", globalAddressString);


}

static void acquireIP(void){

	  EmberStatus status;
	  //Send Request to all clients for CLient IP
	  packet RequestPack;
	  char * messageSendtoClient;
	  EmberIpv6Address local;
	  int len;
	  emberAfCorePrintln("Acquire IP ");
	  RequestPack.commandType=Read_cmd;
	  RequestPack.interfaceType=interface_type_IP;
	  RequestPack.subinterfaceType=subinterface_type_Sensor_Temp;
	  RequestPack.interfaceID=SENSOR_ID0;
	  //Convert multicast IP and attach to the packet
	  //static uint8_t localString[EMBER_IPV6_ADDRESS_STRING_SIZE];
	  messageSendtoClient=formpacket(RequestPack);
	  len=strlen(messageSendtoClient);
	  emberAfCorePrintln("Acquire Client IP Message to all clients=> %s",messageSendtoClient);
	  status = emberCoapPost(&allThreadNodes,serverReportUri,(const uint8_t *)messageSendtoClient,len, processClientDataAck);
	  emberAfCorePrintln("Status of multi-cast request to client=> 0x%x", status);

	  //Need to change this
	  setNextStateWithDelay(ADVERTISE,ADVERTISEMENT_PERIOD_MS);

}

static void advertise(void)
{
  // We peridocally send advertisements to all routers in the mesh.  Unattached
  // clients that hear these advertisements will begin sending data to us.
  EmberStatus status;

  //Advertise using Multicast address to check for clients
  assert(state == ADVERTISE);
  emberAfCorePrint("Advertising to ");
  emberAfCoreDebugExec(emberAfPrintIpv6Address(&allThreadNodes));
  emberAfCorePrintln("");

  status = emberCoapPostNonconfirmable(&allThreadNodes,
                                       serverAdvertiseUri,
                                       NULL, // body
                                       0);   // body length
  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Advertising failed: 0x%x", status);
  }
  //Advertise Complete

  //Complete request for IP


  //write code to  capture the Client IPs from the hanlder ,for now dummy



  //emberGetDhcpClients();
  /*
  networkManagementInfoCommand();
  for (uint8_t i = 0; i < EMBER_MAX_IPV6_ADDRESS_COUNT; i++) {
        EmberIpv6Address address;
        if (emberGetLocalIpAddress(i, &address)) {
          emberAfAppPrint("local ip %u: ", i);
          emberAfAppDebugExec(emberAfPrintIpv6Address(&address));
          emberAfAppPrintln("");
        }
      }
  //End of request send to user
  */
  //repeatStateWithDelay(ADVERTISEMENT_PERIOD_MS);
  //setNextStateWithDelay(ACQUIRE_CLIENT_IP,ADVERTISEMENT_PERIOD_MS);
  setNextStateWithDelay(ADVERTISE,ADVERTISEMENT_PERIOD_MS);
  //setNextStateWithDelay(APPLICATION,ADVERTISEMENT_PERIOD_MS);

}

// advertise
void advertiseCommand(void)
{
  // If we are already advertising, we can manually send a new advertisement
  // using a CLI command.
  if (state == ADVERTISE) {
    advertise();
  }
}

void clientReportHandler(const EmberCoapMessage *request)
{
  // Reports from clients are sent as CoAP POST requests to the "client/report"
  // URI.

  EmberCoapCode responseCode;
  packet RxPacket;
  uint8_t i=0,ClientNotFound;
  static uint8_t IPAddressString[EMBER_IPV6_ADDRESS_STRING_SIZE] = {0};

  /*
  if (state != ADVERTISE) {
    responseCode = EMBER_COAP_CODE_503_SERVICE_UNAVAILABLE;
    emberAfCorePrint("503 ");
  }
	*/
  //message received from Client
  emberAfCorePrintln("Message from Client:(length=%d) %s",
                      request->payloadLength,
                      request->payload);

  //emberAfCoreDebugExec(emberAfPrintIpv6Address(&request->remoteAddress));//
  emberIpv6AddressToString(&request->remoteAddress,
  	                           IPAddressString,
  	                           sizeof(IPAddressString));
  emberAfCorePrintln("Client address assigned %s", IPAddressString);

  RxPacket = FrameAnalysis((char*)request->payload);
  //emberIpv6AddressToString(RxPacket.srcIPAddress,sizeof(RxPacket.srcIPAddress))
  if(RxPacket.interfaceType==interface_type_IP)//If IP in request
	{
       //check for
		i=0;
		ClientNotFound=0;
		emberAfCorePrintln("In IP");

		while(i<MAXCLIENTS && i<=ClientCounts){
			if(!strcmp(IPAddressString,(char *)ClientListIPS[i])){//if Matches
				ClientNotFound =1;
				break;
			}
			i++;
		}
			if(!ClientNotFound){
				MEMCOPY(ClientListIPS[ClientCounts],IPAddressString,sizeof(IPAddressString));
				SendtoRPI((char *)IPAddressString);
				emberAfCorePrintln("No Matched IP found within list");
			}
			else
			{
				emberAfCorePrintln("Matched IP found ,Therefore not stored");
			}
	}

  /*
  if (emberCoapIsSuccessResponse(responseCode)
      || request->localAddress.bytes[0] != 0xFF) { // not multicast
    emberCoapRespond(responseCode, NULL, 0); // no payload
  }
  */
  //send command to client

}

static void resetNetworkState(void)
{
  emberAfCorePrintln("Resetting network state");
  emberResetNetworkState();
}

void emberResetNetworkStateReturn(EmberStatus status)
{
  // If we ever leave the network, we go right back to forming again.  This
  // could be triggered by an external CLI command.

  if (status == EMBER_SUCCESS) {
    emberAfCorePrintln("Reset network state");
  }
}

void emberUdpHandler(const uint8_t *destination,
                     const uint8_t *source,
                     uint16_t localPort,
                     uint16_t remotePort,
                     const uint8_t *payload,
                     uint16_t payloadLength)
{
  // UDP packets for TFTP bootloading are passed through.  Everything else is
  // simply logged and ignored.

  if (localPort == emTftpLocalTid) {
    emProcessTftpPacket(source, remotePort, payload, payloadLength);
  } else if (localPort == TFTP_BOOTLOADER_PORT) {
    emProcessTftpBootloaderPacket(source, payload, payloadLength);
  } else {
    ALIAS(emberUdpHandler)(destination,
                           source,
                           localPort,
                           remotePort,
                           payload,
                           payloadLength);
  }
}

bool emberVerifyBootloadRequest(const TftpBootloaderBootloadRequest *request)
{
  // A real implementation should verify a bootload request to ensure it is
  // valid.  This sample application simply accepts any request.

  return true;
}

void stateEventHandler(void)
{
  emberEventControlSetInactive(stateEventControl);

  // emberAfCorePrintln("State => %d",state);
  switch (state) {
  case RESUME_NETWORK:
    resumeNetwork();
    break;
#ifndef USE_RANDOM_MASTER_KEY
  case SET_SECURITY_PARAMETERS:
    setSecurityParameters();
    break;
#endif
  case FORM_NETWORK:
    formNetwork();
    break;
  case GET_COMMISSIONER:
    getCommissioner();
    break;
  case BECOME_COMMISSIONER:
	becomeCommissioner();
	break;
  case MAKE_ALL_THREAD_NODES_ADDRESS:
      makeAllThreadNodesAddress();
      break;
  case ADVERTISE:
      advertise();
      break;
  //case ACQUIRE_CLIENT_IP:
  //	   acquireIP();
  //	   emberAfCorePrintln("Acquire IP Client ");
  //     break;
  case RESET_NETWORK_STATE:
    resetNetworkState();
    break;
  default:
    assert(false);
  }
}

static void setNextStateWithDelay(uint8_t nextState, uint32_t delayMs)
{
  state = nextState;
  emberEventControlSetDelayMS(stateEventControl, delayMs);
}

static const uint8_t *printableNetworkId(void)
{
  EmberNetworkParameters parameters = {{0}};
  static uint8_t networkId[EMBER_NETWORK_ID_SIZE + 1] = {0};
  emberGetNetworkParameters(&parameters);
  MEMCOPY(networkId, parameters.networkId, EMBER_NETWORK_ID_SIZE);
  return networkId;
}
