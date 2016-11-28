
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

#define ALIAS(x) x##Alias
#include "app/thread/plugin/udp-debug/udp-debug.c"
#include "sensor-control/inc/sensor_control.h"
#include <../B2Bprotocol/B2Bprotocol.h>

// WARNING: This sample application generates a human-readable (i.e., ASCII)
// join key based on a simple hash of the EUI64 of the node.  This method is
// used because it provides a predictable yet reasonably unique key suitable
// for a development kit node.  Real devices MUST NOT use join keys that are
// based on the EUI64.  Random join keys MUST be used for real devices, with
// the keys typically programmed during product manufacturing and printed on
// the devices themselves for access by the user.

// The client/server sample applications use a fixed network id to simplify
// the join process.
static const uint8_t networkId[EMBER_NETWORK_ID_SIZE] = "client/server";

static void resumeNetwork(void);
static void joinNetwork(void);
static void getJoinKey(uint8_t *joinKey, uint8_t *joinKeyLength);
static void waitForServerAdvertisement(void);
static void attachToServer(const EmberIpv6Address *newServer);
static void reportDataToServer(void);
static int32_t getTemp_mC(void);
static void detachFromServer(void);
static void resetNetworkState(void);

static EmberIpv6Address server;
static uint8_t failedReports;
#define REPORT_FAILURE_LIMIT 3
#define WAIT_PERIOD_MS   (30 * MILLISECOND_TICKS_PER_SECOND)
#define REPORT_PERIOD_MS (10 * MILLISECOND_TICKS_PER_SECOND)

static const uint8_t clientReportUri[] = "client/report";

enum {
  INITIAL                       = 0,
  RESUME_NETWORK                = 1,
  JOIN_NETWORK                  = 2,
  WAIT_FOR_SERVER_ADVERTISEMENT = 4,
  REPORT_DATA_TO_SERVER         = 5,
  WAIT_FOR_DATA_CONFIRMATION    = 6,
  RESET_NETWORK_STATE           = 7,
};
static uint8_t state = INITIAL;
EmberEventControl stateEventControl;
static void setNextStateWithDelay(uint8_t nextState, uint32_t delayMs);
#define setNextState(nextState)       setNextStateWithDelay((nextState), 0)
#define repeatState()                 setNextStateWithDelay(state, 0)
#define repeatStateWithDelay(delayMs) setNextStateWithDelay(state, (delayMs))

static const uint8_t *printableNetworkId(void);

void emberAfNetworkStatusCallback(EmberNetworkStatus newNetworkStatus,
                                  EmberNetworkStatus oldNetworkStatus,
                                  EmberJoinFailureReason reason)
{
  // This callback is called whenever the network status changes, like when
  // we finish joining to a network or when we lose connectivity.  If we have
  // no network, we try joining to one.  If we have a saved network, we try to
  // resume operations on that network.  When we are joined and attached to the
  // network, we wait for an advertisement and then begin reporting.

  emberEventControlSetInactive(stateEventControl);

  switch (newNetworkStatus) {
  case EMBER_NO_NETWORK:
    if (oldNetworkStatus == EMBER_JOINING_NETWORK) {
      emberAfCorePrintln("ERR: Joining failed: 0x%x", reason);
    }
    setNextState(JOIN_NETWORK);
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
                        : (state == JOIN_NETWORK
                           ? "Joined"
                           : "Rejoined")),
                       printableNetworkId());
    // TODO: For a brief interruption in connectivity, the client could attempt
    // to continue reporting to its previous server, rather than wait for a new
    // server.
    setNextState(WAIT_FOR_SERVER_ADVERTISEMENT);
    break;
  case EMBER_JOINED_NETWORK_NO_PARENT:
    // We always join as a router, so we should never end up in the "no parent"
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
  // even attempt to resume, we just give up and try joining instead.

  assert(state == RESUME_NETWORK);

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Unable to resume: 0x%x", status);
    setNextState(JOIN_NETWORK);
  }
}

static void joinNetwork(void)
{
  // When joining a network, we look for one specifically with our network id.
  // The commissioner must have our join key for this to succeed.

  EmberNetworkParameters parameters = {{0}};
  uint8_t joinKey[EMBER_ENCRYPTION_KEY_SIZE + 1] = {0};

  assert(state == JOIN_NETWORK);

  MEMCOPY(parameters.networkId, networkId, sizeof(networkId));
  parameters.nodeType = EMBER_ROUTER;
  parameters.radioTxPower = 3;
  getJoinKey(joinKey, &parameters.joinKeyLength);
  MEMCOPY(parameters.joinKey, joinKey, parameters.joinKeyLength);

  emberAfCorePrint("Joining network \"%s\" with EUI64 ", networkId);
  emberAfCoreDebugExec(emberAfPrintBigEndianEui64(emberEui64()));
  emberAfCorePrintln(" and join key \"%s\"", joinKey);

  emberJoinNetwork(&parameters,
                   (EMBER_NETWORK_ID_OPTION
                    | EMBER_NODE_TYPE_OPTION
                    | EMBER_TX_POWER_OPTION
                    | EMBER_JOIN_KEY_OPTION),
                   EMBER_ALL_802_15_4_CHANNELS_MASK);
}

static void getJoinKey(uint8_t *joinKey, uint8_t *joinKeyLength)
{
  // Using a join key generated from the EUI64 of the node is a security risk,
  // but is useful for demonstration purposes.  See the warning above.  This
  // hash generates alphanumeric characters in the ranges 0--9 and A--U.  The
  // Thread specification disallows the characters I, O, Q, and Z, for
  // readability.  I, O, and Q are therefore remapped to V, W, and X.  Z is not
  // generated, so it is not remapped.

  const EmberEui64 *eui64;

  eui64 = emberEui64();
  for (*joinKeyLength = 0; *joinKeyLength < EUI64_SIZE; (*joinKeyLength)++) {
    joinKey[*joinKeyLength] = ((eui64->bytes[*joinKeyLength] & 0x0F)
                               + (eui64->bytes[*joinKeyLength] >> 4));
    joinKey[*joinKeyLength] += (joinKey[*joinKeyLength] < 10 ? '0' : 'A' - 10);
    if (joinKey[*joinKeyLength] == 'I') {
      joinKey[*joinKeyLength] = 'V';
    } else if (joinKey[*joinKeyLength] == 'O') {
      joinKey[*joinKeyLength] = 'W';
    } else if (joinKey[*joinKeyLength] == 'Q') {
      joinKey[*joinKeyLength] = 'X';
    }
  }
}

void emberJoinNetworkReturn(EmberStatus status)
{
  // This return indicates whether the stack is going to attempt to join.  If
  // so, the result is reported later as a network status change.  Otherwise,
  // we just try again.

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Unable to join: 0x%x", status);
    repeatState();
  }
}

static void waitForServerAdvertisement(void)
{
  // Once on the network, we wait for a server to advertise itself.  We
  // periodically print a message while waiting to prove we are alive.

  assert(state == WAIT_FOR_SERVER_ADVERTISEMENT);

  emberAfCorePrintln("Waiting for an advertisement from a server");

  repeatStateWithDelay(WAIT_PERIOD_MS);
}

void serverAdvertiseHandler(const EmberCoapMessage *request)
{
  // Advertisements from servers are sent as CoAP POST requests to the
  // "server/advertise" URI.  When we receive an advertisement, we attach to
  // the that sent it and beginning sending reports.

  EmberCoapCode responseCode;

  if (state != WAIT_FOR_SERVER_ADVERTISEMENT) {
    responseCode = EMBER_COAP_CODE_503_SERVICE_UNAVAILABLE;
  } else {
    attachToServer(&request->remoteAddress);
    responseCode = EMBER_COAP_CODE_204_CHANGED;
  }

  if (emberCoapIsSuccessResponse(responseCode)
      || request->localAddress.bytes[0] != 0xFF) { // not multicast
    emberCoapRespond(responseCode, NULL, 0); // no payload
  }
}

static void attachToServer(const EmberIpv6Address *newServer)
{
  // We attach to a server in response to an advertisement (or a CLI command).
  // Once we have a server, we begin reporting periodically.  We start from a
  // clean state with regard to failed reports.

  assert(state == WAIT_FOR_SERVER_ADVERTISEMENT);

  MEMCOPY(&server, newServer, sizeof(EmberIpv6Address));
  failedReports = 0;

  emberAfCorePrint("Attached to server at ");
  emberAfCoreDebugExec(emberAfPrintIpv6Address(newServer));
  emberAfCorePrintln("");

  setNextState(REPORT_DATA_TO_SERVER);
}

// attach <server>
void attachCommand(void)
{
  // If we are waiting for a server, we can manually attach to one using a CLI
  // command.

  if (state == WAIT_FOR_SERVER_ADVERTISEMENT) {
    EmberIpv6Address newServer = {{0}};
    if (emberGetIpv6AddressArgument(0, &newServer)) {
      attachToServer(&newServer);
    } else {
      emberAfCorePrintln("ERR: Invalid ip");
    }
  }
}

static void processServerDataAck(EmberCoapStatus status,
                                 EmberCoapMessage *coap,
                                 void *appData,
                                 uint16_t appDatalength)
{
  // We track the success or failure of reports so that we can determine when
  // we have lost the server.  A series of consecutive failures is the trigger
  // to detach from the current server and find a new one.  Any successfully-
  // transmitted report clears past failures.

  if (state == WAIT_FOR_DATA_CONFIRMATION) {
    if (status == EMBER_COAP_MESSAGE_ACKED
        || status == EMBER_COAP_MESSAGE_RESPONSE) {
      failedReports = 0;
    } else {
      failedReports++;
      emberAfCorePrintln("ERR: Report timed out - failure %u of %u",
                         failedReports,
                         REPORT_FAILURE_LIMIT);
    }
    if (failedReports < REPORT_FAILURE_LIMIT) {
      setNextStateWithDelay(REPORT_DATA_TO_SERVER, REPORT_PERIOD_MS);
    } else {
      detachFromServer();
    }
  }
}

static void reportDataToServer(void)
{
  // We peridocally send data to the server.  The data is the temperature,
  // measured in 10^-3 degrees Celsius.  The success or failure of the reports
  // is tracked so we can determine if the server has disappeared and we should
  // find a new one.

  EmberStatus status;
  int32_t data;
  uint8_t senddata;
  //char messageRX[]="chinmay surjith asc ascasca";
  char messageRX[50]="# 0 0 0 0 234:234:234:24 234:234:2342:22 32 $";
  char *messageTx=NULL;
  char *tempMessage = "test data with changes";
  //uint8_t tempdata=0;
  //char messageRX[1000]="$|0|0|0|0|234:234:234:24|234:234:2342:22|#";
  assert(state == REPORT_DATA_TO_SERVER);
  packet TxPacket;




  uint32_t sensorData=0;
      if ((sensorData=fetchSensorData(SENSOR_TYPE_TEMP_Si7021))==ERR_READ_SENSOR)
      {
    	  emberAfCorePrintln("Error in measurement");
      }
      else
      {
    	  emberAfCorePrintln("Temperature Measured%d",sensorData);

      }
  data = getTemp_mC();
  data = sensorData;

  //Frame Analysis of input message
  emberAfCorePrint("Frame Analysis  ");
  TxPacket=FrameAnalysis(messageRX);
  emberAfCorePrint("Tx Packet");
  printPacket(TxPacket);

  //formpacket(TxPacket);// Formulate Message to be send
  messageTx=formpacket(TxPacket);// Formulate Message to be send
  emberAfCorePrint("Send Tx Packet %s\n",messageTx);


  //emberAfCorePrint("Split the message from server ");
  //splitPacket(messageRX);
  emberAfCoreDebugExec(emberAfPrintIpv6Address(&server));
  emberAfCorePrintln("");

  // Convert from host-order to network-order before sending so the data can be
  // reliably reconstructed by the server.
  data = HTONL(data);
  senddata = atoi(messageTx);

  emberAfCorePrint("data from messageTX %ld => %d  , %d    %ld \n",( uint8_t *)messageTx,   strlen(messageTx),sizeof(messageTx),senddata);

  senddata = HTONL(senddata);
  emberAfCorePrint("data HTONL %ld =>",senddata);
//  status = emberCoapPost(&server,                         clientReportUri,                         //(const uint8_t *)&data,						 (const char *)messageRX,                          sizeof(messageRX),                        processServerDataAck);
  status = emberCoapPost(&server,
                            clientReportUri,
							(const uint8_t *)messageTx,
                             sizeof(messageTx),
							//(const uint8_t *)senddata,
							//sizeof(senddata),
                            processServerDataAck);

  if (status == EMBER_SUCCESS) {
    setNextState(WAIT_FOR_DATA_CONFIRMATION);
  } else {
    emberAfCorePrintln("ERR: Reporting failed: 0x%x", status);
    repeatStateWithDelay(REPORT_PERIOD_MS);
  }
}

#ifdef CORTEXM3_EFM32_MICRO
static int32_t getTemp_mC(void)
{
  return TEMPDRV_GetTemp() * 1000;
}
#else
static int32_t getTemp_mC(void)
{
  uint16_t value;
  int16_t volts;
  halStartAdcConversion(ADC_USER_APP,
                        ADC_REF_INT,
                        TEMP_SENSOR_ADC_CHANNEL,
                        ADC_CONVERSION_TIME_US_256);
  halReadAdcBlocking(ADC_USER_APP, &value);
  volts = halConvertValueToVolts(value / TEMP_SENSOR_SCALE_FACTOR);
  return (1591887L - (171 * (int32_t)volts)) / 10;
}
#endif

// report
void reportCommand(void)
{
  // If we have a server and are reporting data, we can manually send a new
  // report using a CLI command.

  if (state == REPORT_DATA_TO_SERVER) {
    reportDataToServer();
  }
}

static void detachFromServer(void)
{
  // We detach from a server in response to failed reports (or a CLI command).
  // Once we detach, we wait for a new server to advertise itself.

  assert(state == REPORT_DATA_TO_SERVER
         || state == WAIT_FOR_DATA_CONFIRMATION);

  emberAfCorePrint("Detached from server at ");
  emberAfCoreDebugExec(emberAfPrintIpv6Address(&server));
  emberAfCorePrintln("");

  setNextState(WAIT_FOR_SERVER_ADVERTISEMENT);
}

// detach
void detachCommand(void)
{
  // If we have a server and are reporting data, we can manually detach and try
  // to find a new server by using a CLI command.

  if (state == REPORT_DATA_TO_SERVER
      || state == WAIT_FOR_DATA_CONFIRMATION) {
    detachFromServer();
  }
}

static void resetNetworkState(void)
{
  emberAfCorePrintln("Resetting network state");
  emberResetNetworkState();
}

void emberResetNetworkStateReturn(EmberStatus status)
{
  // If we ever leave the network, we go right back to joining again.  This
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

  switch (state) {
  case RESUME_NETWORK:
    resumeNetwork();
    break;
  case JOIN_NETWORK:
    joinNetwork();
    break;
  case WAIT_FOR_SERVER_ADVERTISEMENT:
    waitForServerAdvertisement();
    break;
  case REPORT_DATA_TO_SERVER:
    reportDataToServer();
    break;
  case WAIT_FOR_DATA_CONFIRMATION:
    break;
  case RESET_NETWORK_STATE:
    resetNetworkState();
    break;
  default:
    assert(false);
    break;
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
