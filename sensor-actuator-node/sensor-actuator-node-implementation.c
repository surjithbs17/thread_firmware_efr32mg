// Copyright 2016 Silicon Laboratories, Inc.

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK
#include EMBER_AF_API_HAL
#include EMBER_AF_API_BUTTON_PRESS
#include EMBER_AF_API_COMMAND_INTERPRETER2
#include EMBER_AF_API_NETWORK_MANAGEMENT
#ifdef EMBER_AF_API_DEBUG_PRINT
  #include EMBER_AF_API_DEBUG_PRINT
#endif
#include EMBER_AF_API_TFTP
#include EMBER_AF_API_TFTP_BOOTLOADER

#if defined(CORTEXM3_EFM32_MICRO)
#include "em_adc.h"
#include "em_cmu.h"
#if defined(EMBER_AF_PLUGIN_GLIB)
#include "spidrv.h"
#include "displaypalconfig.h"
#include "glib.h"
#include "graphics.h"
#endif //EMBER_AF_PLUGIN_GLIB
#endif //CORTEXM3_EFM32_MICRO

#define ALIAS(x) x##Alias
#include "app/thread/plugin/udp-debug/udp-debug.c"

#include <stdio.h>

// This definition is used when a Thread network should be joined out of band.
// If ENABLE_JOIN_OUT_OF_BAND is defined a precommissioned network will be used
// instead of commissioning. Use this feature with a border-router application
// that also has this defined and uses the same settings specified below.
//
// NOTE: This is currently used by default because commissioning as it is
// specified in version 1.1 of the Thread spec is not supported in the official
// Thread commissioning application yet.
#define ENABLE_JOIN_OUT_OF_BAND
#define OOB_PREFERRED_CHANNEL 19
#define OOB_FALLBACK_CHANNEL_MASK 0 // All channels
#define OOB_PAN_ID 0x1075
#define OOB_EXTENDED_PAN_ID { 0xc6, 0xef, 0xe1, 0xb4, 0x5f, 0xc7, 0x8e, 0x4f }
#define OOB_KEY_SEQUENCE 0
#define OOB_FIXED_JOIN_KEY "ABCD1234"
#define OOB_ULA_PREFIX {0xfd, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}

// WARNING: This sample application generates a human-readable (i.e., ASCII)
// join key based on a simple hash of the EUI64 of the node if
// USE_EUI64_AS_JOIN_KEY is defined.  If it is not defined it will use the
// FIXED_JOIN_KEY.  These methods are used because it provides a predictable
// yet reasonably unique key suitable for a development kit node.  Real devices
// MUST NOT use join keys that are based on the EUI64 or predefined.  Random
// join keys MUST be used for real devices, with the keys typically programmed
// during product manufacturing and printed on the devices themselves for access
// by the user.
//
#define USE_EUI64_AS_JOIN_KEY 1
#define FIXED_JOIN_KEY "ABCD1234"

// This represents how many times the device will attempt to join until it
// stops scanning and waits for another button press to trigger a join
#define SENSOR_ACTUATOR_JOIN_ATTEMPTS 2

// LCD UI macros
#define JOIN_KEY_ROW 5
#define NODE_TYPE_ROW 25
#define NETWORK_ID_ROW 45
#define PAN_ID_ROW 55
#define CHANNEL_ROW 65
#define SHORT_ID_ROW 75
#define IP_ADDRESS_INITIAL_ROW 85

#define ROW_SIZE 10
#define START_COLUMN 5
#define LCD_ASCII_SCREEN_WIDTH 20

#define PADDED_ASCII_BUFFER_SIZE 32
#define IP_ADDRESS_CHAR_TRIM 9

#define LCD_OPAQUE_VALUE 0
#define LCD_REFRESH_MS 200

// Start off with join attempts remaining to 0, to have this scan on startup
// set this to SENSOR_ACTUATOR_JOIN_ATTEMPTS
static uint8_t joinAttemptsRemaining = 0; 

// Every IDLE_PERIOD_TIMEOUT_MS the key is printed to the console
#define IDLE_PERIOD_TIMEOUT_MS (60 * MILLISECOND_TICKS_PER_SECOND)

#define NETWORK_STATUS_STRING(status) \
  (status == EMBER_NO_NETWORK ? "NO NETWORK": \
  (status == EMBER_SAVED_NETWORK ? "SAVED NETWORK" : \
  (status == EMBER_JOINING_NETWORK ? "JOINING NETWORK" : \
  (status == EMBER_JOINED_NETWORK_ATTACHING ? "JOINED ATTACHING" : \
  (status == EMBER_JOINED_NETWORK_ATTACHED ? "JOINED ATTACHED" : \
  (status == EMBER_JOINED_NETWORK_NO_PARENT ? "JOINED NO PARENT" : \
  "ERROR"))))))

static void resumeNetwork(void);
static void getJoinKey(uint8_t *joinKey, uint8_t *joinKeyLength);
#if defined(ENABLE_JOIN_OUT_OF_BAND)
#define joinNetwork joinNetworkCommissioned
static void joinNetworkCommissioned(void);
#else
static void joinNetwork(void);
#endif
static void announceToAddress(const EmberIpv6Address *newBorderRouter);
static void finishAnnounceToAddress(void);
static void waitForSubscriptions(void);
static void startReportingTemperature(const EmberIpv6Address *newSubscriber);
static void reportTemperature(void);
static void stopReportingTemperature(void);
static void resetNetworkState(void);
static void rejoinNetwork(void);

// These functions will be used when the WSTK is used as the platform to
// populate the LCD with useful information
#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
static char* nodeTypeToString(EmberNodeType nodeType);
static void lcdPrintNetworkStatus(EmberNetworkStatus status, uint8_t printRow);
static void lcdPrintJoinCode(uint8_t *joinKey, uint8_t printRow);
static void lcdPrintIpv6Address(const EmberIpv6Address *address,
                                uint8_t printRow,
                                uint8_t screenWidth);
static void lcdPrintShortId(void);
static void lcdPrintCurrentDeviceState(void);
#endif //EMBER_AF_PLUGIN_GLIB

static void reportTemperatureStatusHandler(EmberCoapStatus status,
                                           EmberCoapMessage *coap,
                                           void *appData,
                                           uint16_t appDatalength);

static void buttonPressStatusHandler(EmberCoapStatus status,
                                     EmberCoapMessage *coap,
                                     void *appData,
                                     uint16_t appDatalength);

static EmberIpv6Address globalAddress;
static uint8_t globalAddressString[EMBER_IPV6_ADDRESS_STRING_SIZE] = {0};
static EmberIpv6Address discoverRequestAddress;
static EmberIpv6Address subscriberAddress;
static uint8_t joinKey[EMBER_ENCRYPTION_KEY_SIZE + 1] = {0};
static uint8_t joinKeyLength = 0;

static const EmberIpv6Address allMeshRouters = {
  {0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,}
};

static uint8_t failedReports;
#define REPORT_FAILURE_LIMIT 3
#define WAIT_PERIOD_MS   (30 * MILLISECOND_TICKS_PER_SECOND)
#define REPORT_PERIOD_MS (10 * MILLISECOND_TICKS_PER_SECOND)

static const uint8_t deviceAnnounceUri[] = "device/announce";
static const uint8_t temperatureUri[] = "device/temperature";
static const uint8_t buttonUri[] = "device/button";

#if defined(ENABLE_JOIN_OUT_OF_BAND)
// WARNING: This compile option uses a fixed join key that is global to the
// entire Thread network.  This method is used because it provides a predictable
// key suitable for a development kit node. Real devices MUST NOT use join keys
// that are fixed. Random join keys MUST be used for real devices, with the keys
// typically programmed during product manufacturing and printed on the devices
// themselves for access by the user.

static const uint8_t networkId[EMBER_NETWORK_ID_SIZE] = "border-router";
static const uint8_t preferredChannel = OOB_PREFERRED_CHANNEL;
static const uint32_t fallbackChannelMask = OOB_FALLBACK_CHANNEL_MASK;
static const uint16_t panId = OOB_PAN_ID;
static const uint8_t ulaPrefix[] = OOB_ULA_PREFIX;
static const uint8_t extendedPanId[] = OOB_EXTENDED_PAN_ID;
static const uint32_t keySequence = OOB_KEY_SEQUENCE;

// WARNING: This compile option uses the well-know sensor/sink network key as 
// the master key.  This is done for demonstration purposes, so that packets
// decrypt automatically in Ember Desktop.  Using predefined keys keys is a 
// significant security risk.  Real devices MUST NOT used fixed keys and 
// MUST use random keys instead.  The stack automatically generates random
// keys if emberSetSecurityParameters is not called prior to forming.
//#define USE_RANDOM_MASTER_KEY
static const EmberKeyData masterKey = {
  {0x65, 0x6D, 0x62, 0x65, 0x72, 0x20, 0x45, 0x4D,
   0x32, 0x35, 0x30, 0x20, 0x63, 0x68, 0x69, 0x70,}
};
#endif 


enum {
  INITIAL                              = 0,
  RESUME_NETWORK                       = 1,
  JOIN_NETWORK                         = 2,
  WAIT_FOR_JOIN_NETWORK                = 3,
  WAIT_FOR_SUBSCRIPTIONS               = 4,
  REPORT_TEMP_TO_SUBSCRIBER            = 5,
  WAIT_FOR_DATA_CONFIRMATION           = 6,
  JOINED_TO_NETWORK                    = 7,
  RESET_NETWORK_STATE                  = 8,
};

#define STATE_STRING(state) \
  (state == INITIAL ? "INITIAL": \
  (state == RESUME_NETWORK ? "RESUME_NETWORK": \
  (state == JOIN_NETWORK ? "JOIN_NETWORK": \
  (state == WAIT_FOR_JOIN_NETWORK ? "WAIT_FOR_JOIN_NETWORK": \
  (state == WAIT_FOR_SUBSCRIPTIONS ? "WAIT_FOR_SUBSCRIPTIONS": \
  (state == REPORT_TEMP_TO_SUBSCRIBER ? "REPORT_TEMP_TO_SUBSCRIBER": \
  (state == WAIT_FOR_DATA_CONFIRMATION ? "WAIT_FOR_DATA_CONFIRMATION": \
  (state == JOINED_TO_NETWORK ? "JOINED_TO_NETWORK": \
  (state == RESET_NETWORK_STATE ? "RESET_NETWORK_STATE": \
  "ERROR")))))))))

static uint8_t state = INITIAL;
EmberEventControl stateEventControl;
EmberEventControl updateDisplayEventControl;

static void setNextStateWithDelay(uint8_t nextState, uint32_t delayMs);
#define setNextState(nextState)       setNextStateWithDelay((nextState), 0)
#define repeatState()                 setNextStateWithDelay(state, 0)
#define repeatStateWithDelay(delayMs) setNextStateWithDelay(state, (delayMs))

// Keep our EUI string for JSON payloads, take size in bytes x2 to account for
// ASCII and add 1 for a NULL terminator
static uint8_t euiString[EUI64_SIZE * 2 + 1] = {0};

// String helper function prototypes
static void formatEuiString(uint8_t* euiString, size_t strSz, const uint8_t* euiBytes);
static const uint8_t *printableNetworkId(void);

#if defined(CORTEXM3_EFM32_MICRO)
static void adcSetup(void);
static int32_t convertToCelsius(int32_t adcSample);
static uint32_t adcRead(void);

// @brief Initialize ADC for temperature sensor readings in single join
static void adcSetup(void)
{
  // Enable ADC clock 
  CMU_ClockEnable(cmuClock_ADC0, true);

  // Base the ADC configuration on the default setup. 
  ADC_Init_TypeDef       init  = ADC_INIT_DEFAULT;
  ADC_InitSingle_TypeDef sInit = ADC_INITSINGLE_DEFAULT;

  // Initialize timebases 
  init.timebase = ADC_TimebaseCalc(0);
  init.prescale = ADC_PrescaleCalc(400000, 0);
  ADC_Init(ADC0, &init);

  // Set input to temperature sensor. Reference must be 1.25V 
  sInit.reference   = adcRef1V25;
  sInit.acqTime     = adcAcqTime8; // Minimum time for temperature sensor 
  sInit.posSel      = adcPosSelTEMP;
  ADC_InitSingle(ADC0, &sInit);
}

// @brief  Do one ADC conversion
// @return ADC conversion result
static uint32_t adcRead(void)
{
  ADC_Start(ADC0, adcStartSingle);
  while ((ADC0->STATUS & ADC_STATUS_SINGLEDV) == 0) {
    // Busy wait
  }
  return ADC_DataSingleGet(ADC0);
}

// @brief Convert ADC sample values to celsius.
// @detail See section 25.3.4.1 in the reference manual for detail
//   on temperature measurement and conversion.
// @param adcSample Raw value from ADC to be converted to celsius
// @return The temperature in milli degrees celsius.
static int32_t convertToCelsius(int32_t adcSample)
{
  uint32_t calTemp0;
  uint32_t calValue0;
  int32_t readDiff;
  float temp;

  // Factory calibration temperature from device information page.
  calTemp0 = ((DEVINFO->CAL & _DEVINFO_CAL_TEMP_MASK)
              >> _DEVINFO_CAL_TEMP_SHIFT);

  calValue0 = ((DEVINFO->ADC0CAL3
                // _DEVINFO_ADC0CAL3_TEMPREAD1V25_MASK is not correct in
                //    current CMSIS. This is a 12-bit value, not 16-bit.
                & 0xFFF0)
               >> _DEVINFO_ADC0CAL3_TEMPREAD1V25_SHIFT);

  if ((calTemp0 == 0xFF) || (calValue0) == 0xFFF) {
    // The temperature sensor is not calibrated 
    return -100.0;
  }

  // Vref = 1250mV
  // TGRAD_ADCTH = 1.835 mV/degC (from datasheet)

  readDiff = calValue0 - adcSample;
  temp     = ((float)readDiff * 1250);
  temp    /= (4096 * -1.835);

  // Calculate offset from calibration temperature 
  temp     = (float)calTemp0 - temp;
  return (uint32_t)(temp * 1000);
}
#else
static void adcSetup(void)
{
  // No setup needed for non EFR devices
}
#endif //CORTEXM3_EFM32_MICRO

void emberAfInitCallback(void)
{
  // This callback is called after the stack has been initialized.  It is our
  // opportunity to do initialization, like resuming the network or joining a
  // new network.

  // Get and store EUI since we need it around for JSON strings
  formatEuiString(euiString, sizeof(euiString), emberEui64()->bytes);
  emberAfCorePrintln("Eui64: >%s", euiString);

  // joinKey does not change during operation and can be called during init
  getJoinKey(joinKey, &joinKeyLength);

  // Set up the ADC for temperature conversions
  adcSetup();

  // Set Led to on by default
  halSetLed(BOARD_ACTIVITY_LED);

#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
  // If it is present, initialize the graphics display
  GRAPHICS_Init();
  glibContext.backgroundColor = White;
  glibContext.foregroundColor = Black;
  GLIB_clear(&glibContext);
#endif //EMBER_AF_PLUGIN_GLIB
}

void emberAfNetworkStatusCallback(EmberNetworkStatus newNetworkStatus,
                                  EmberNetworkStatus oldNetworkStatus,
                                  EmberJoinFailureReason reason)
{
  // This callback is called whenever the network status changes, like when
  // we finish joining to a network or when we lose connectivity.  If we have
  // no network, we try joining to one as long as join attempts remain.  Join
  // attempts are reset whenever button 1 is pressed.  If we have a saved
  // network, we try to resume operations on that network.  When we are joined
  // and attached to the network, we wait for an advertisement and then begin 
  // reporting.

  emberEventControlSetInactive(stateEventControl);
  emberEventControlSetActive(updateDisplayEventControl);

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

    joinAttemptsRemaining = 0;

    setNextState(WAIT_FOR_SUBSCRIPTIONS);
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

  emberEventControlSetActive(updateDisplayEventControl);
}

#if defined(ENABLE_JOIN_OUT_OF_BAND)
static void joinNetworkCommissioned(void)
{
  emberAfCorePrintln("Joining network with precommisioned network configuration");
  emberCommissionNetwork(preferredChannel,
                         fallbackChannelMask,
                         networkId,
                         sizeof(networkId),
                         panId,
                         ulaPrefix,
                         extendedPanId,
                         &masterKey,
                         keySequence);
}
#else
static void joinNetwork(void)
{
  // When joining a network, we look for one specifically with our network id.
  // The commissioner must have our join key for this to succeed.

  EmberNetworkParameters parameters = {{0}};

  assert(state == JOIN_NETWORK);

  if (joinAttemptsRemaining > 0) {
     // Decrement the join counter 
     joinAttemptsRemaining--;

     emberAfCorePrintln("Joining network using connect code \"%s\", join attempts left = %d",
                        joinKey,
                        joinAttemptsRemaining);

     parameters.nodeType = EMBER_ROUTER;
     parameters.radioTxPower = 3;
     parameters.joinKeyLength = joinKeyLength;
     MEMCOPY(parameters.joinKey, joinKey, parameters.joinKeyLength);

     emberJoinNetwork(&parameters,
                      (EMBER_NODE_TYPE_OPTION
                       | EMBER_TX_POWER_OPTION
                       | EMBER_JOIN_KEY_OPTION),
                      EMBER_ALL_802_15_4_CHANNELS_MASK);
  } else {
    // No join attempts remain 
    emberAfCorePrintln("Device idle, press button 1 or `rejoin-network` command to start a join for key \"%s\"",
                       joinKey);
    repeatStateWithDelay(WAIT_PERIOD_MS);
  }
}
#endif //ENABLE_JOIN_OUT_OF_BAND

static void getJoinKey(uint8_t *joinKey, uint8_t *joinKeyLength)
{
#if USE_EUI64_AS_JOIN_KEY
  // Using a join key generated from the EUI64 of the node is a security risk,
  // but is useful for demonstration purposes. See the warning above. This
  // hash generates alphanumeric characters in the ranges 0-9 and A-U. The
  // Thread specification disallows the characters I, O, Q, and Z, for
  // readability. I, O, and Q are therefore remapped to V, W, and X. Z is not
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
#else
  // Using a static join key is a security risk, but is useful for demonstration
  // purposes.  See the warning above.
  char fixedJoinKey[EMBER_ENCRRYPTION_KEY_SIZE + 1] = FIXED_JOIN_KEY;
  *joinKeyLength = strlen(fixedJoinKey);
  MEMCOPY(joinKey, fixedJoinKey, *joinKeyLength); 
#endif
  emberEventControlSetActive(updateDisplayEventControl);
}

void emberJoinNetworkReturn(EmberStatus status)
{
  // This return indicates whether the stack is going to attempt to join.  If
  // so, the result is reported later as a network status change.  Otherwise,
  // we just try to join again as long as join attempts remain.

  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Unable to join: 0x%x", status);
    setNextState(JOIN_NETWORK);
  } else {
    setNextState(WAIT_FOR_JOIN_NETWORK);
  }
}

void emberCommissionNetworkReturn(EmberStatus status)
{
  switch (status) {
  case EMBER_SUCCESS:
    setNextState(WAIT_FOR_JOIN_NETWORK);
    break;
  case EMBER_BAD_ARGUMENT:
  case EMBER_INVALID_CALL:
    // Try to join the network again if it fails
    emberAfCorePrintln("Pre-Commission failed status %x, retrying", status);
    setNextState(JOIN_NETWORK);
    break;
  default:
    break;
  }
}

void joinNetworkCommissionedCompletion(void)
{
  emberAfCorePrintln("Completing precommisioned join");
  emberJoinCommissioned(3,            // radio tx power
                        EMBER_ROUTER, // type
                        true);        // require connectivity
}

void borderRouterDiscoveryHandler(const EmberCoapMessage *request)
{
  // Announce to the remote address trying to discover
  announceToAddress(&request->remoteAddress);
}

// announce
void announceCommand(void)
{
  // We can force an announce to all mesh routers with an announce command
  announceToAddress(&allMeshRouters);
}

static void announceToAddress(const EmberIpv6Address *address)
{
  // We announce to the requested address in response to a discovery (or a CLI
  // command).
  MEMCOPY(&discoverRequestAddress, address, sizeof(EmberIpv6Address));

  emberAfCorePrint("Announcing to address to: ");
  emberAfCoreDebugExec(emberAfPrintIpv6Address(&discoverRequestAddress));
  emberAfCorePrintln("");

  // Get the global address to return to the border router when attaching
  emberGetGlobalAddresses(NULL, 0); // all prefixes

  // The announcement will finish once the global address is returned
}

void emberGetGlobalAddressReturn(const EmberIpv6Address *address,
                                 uint32_t preferredLifetime,
                                 uint32_t validLifetime,
                                 uint8_t addressFlags)
{
  MEMCOPY(&globalAddress, address, sizeof(EmberIpv6Address));
  emberIpv6AddressToString(&globalAddress,
                           globalAddressString,
                           sizeof(globalAddressString));
  emberAfCorePrintln("Global address assigned %s", globalAddressString);
  finishAnnounceToAddress();

  emberEventControlSetInactive(updateDisplayEventControl);
}

static void finishAnnounceToAddress(void)
{
  EmberStatus status;

  // If we returned and got an IP address, return it otherwise just return
  // the EUI and device type
  // Send this CoAP response to the border router when it tries to discover:
  //     {
  //        "ip":"aaaa:0000:0000:0000:1234:1234:1234:1234",
  //        "eui64":"1234123412341234",
  //        "type":"sensor-actuator-node"
  //     }

  // NULL JSON that is a sensor-actuator-node

  char jsonString[140] =
    "{\"ip\":\"0000:0000:0000:0000:0000:0000:0000:0000\",\"eui64\":\"0000000000000000\",\"type\":\"sensor-actuator-node\"}\0";
  uint8_t strSz = strlen("{\"ip\":\"");
  char euiKey[] = "\",\"eui64\":\"";
  char jsonClose[] = "\",\"type\":\"sensor-actuator-node\"}";

  // If we got a global address put it in the JSON
  strncpy(jsonString + strSz, (char const*)globalAddressString, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)globalAddressString);

  // Put the EUI64 in the JSON
  strncpy(jsonString + strSz, (char const*)euiKey, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)euiKey);
  strncpy(jsonString + strSz, (char const*)euiString, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)euiString);

  strncpy(jsonString + strSz, (char const*)jsonClose, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)jsonClose);

  status = emberCoapPostNonconfirmable(&discoverRequestAddress,
                                       deviceAnnounceUri,
                                       (uint8_t const*)jsonString,
                                       strlen(jsonString));
  if (status != EMBER_SUCCESS) {
    emberAfCorePrintln("ERR: Discovery announcement failed: 0x%x", status);
  }
}

void subscribeHandler(const EmberCoapMessage *request)
{
  EmberCoapCode responseCode;

  if (state == WAIT_FOR_SUBSCRIPTIONS) {
    startReportingTemperature(&request->remoteAddress);
    responseCode = EMBER_COAP_CODE_204_CHANGED;
  } else {
    responseCode = EMBER_COAP_CODE_503_SERVICE_UNAVAILABLE;
  }

  emberCoapRespond(responseCode, NULL, 0); // no payload
}

static void startReportingTemperature(const EmberIpv6Address *newSubscriber)
{
  assert(state == WAIT_FOR_SUBSCRIPTIONS);

  MEMCOPY(&subscriberAddress, newSubscriber, sizeof(EmberIpv6Address));
  failedReports = 0;

  emberAfCorePrint("Starting reporting to subscriber at ");
  emberAfCoreDebugExec(emberAfPrintIpv6Address(&subscriberAddress));
  emberAfCorePrintln("");

  setNextState(REPORT_TEMP_TO_SUBSCRIBER);
}

void ledHandler(const EmberCoapMessage *request)
{
  emberAfCorePrintln("Toggle LED");
  halToggleLed(BOARD_ACTIVITY_LED);

  emberCoapRespond(EMBER_COAP_CODE_204_CHANGED, NULL, 0); // no payload
}

void buzzerHandler(const EmberCoapMessage *request)
{
#if defined(CORTEXM3_EFM32_MICRO)
  emberAfCorePrintln("WARN: Received buzzer command, no buzzer HW supported");
  emberCoapRespond(EMBER_COAP_CODE_404_NOT_FOUND, NULL, 0); // no payload
#else
  emberAfCorePrintln("Sound Buzzer");
  halStackIndicatePresence();
  emberCoapRespond(EMBER_COAP_CODE_204_CHANGED, NULL, 0); // no payload
#endif
}

void onOffOutHandler(const EmberCoapMessage *request)
{
  emberAfCorePrintln("Toggling Activity LED");

  if (request->payload == NULL) {
    emberCoapRespond(EMBER_COAP_CODE_400_BAD_REQUEST, NULL, 0); // no payload
    return;
  } else {
    emberCoapRespond(EMBER_COAP_CODE_204_CHANGED, NULL, 0); // no payload
  }

  if (request->payload[0] == '0') {
    halClearLed(BOARD_ACTIVITY_LED);
  } else {
    halSetLed(BOARD_ACTIVITY_LED);
  }
}

static void waitForSubscriptions(void)
{
  // Once attached to the border router, we wait for a subscriber to
  // request subcribe to our data. We periodically print a message while
  // waiting to prove we are alive.

  assert(state == WAIT_FOR_SUBSCRIPTIONS);

  emberAfCorePrintln("Waiting for a subscriber");

  repeatStateWithDelay(WAIT_PERIOD_MS);
}

static void reportTemperature(void)
{
  // We periodically send data to the subscriber. The data is the temperature,
  // measured in 10^-3 degrees Celsius.  The success or failure of the reports
  // is tracked so we can determine if the server has disappeared and we should
  // find a new one.

  EmberStatus status;
  int32_t temperatureMC;

#if defined(CORTEXM3_EFM32_MICRO)
  uint32_t tempRead;
#else
  uint16_t value;
  int16_t volts;
#endif

  assert(state == REPORT_TEMP_TO_SUBSCRIBER);

#if defined(CORTEXM3_EFM32_MICRO)
  tempRead = adcRead();
  temperatureMC = convertToCelsius(tempRead);
#else
  halStartAdcConversion(ADC_USER_APP,
                        ADC_REF_INT,
                        TEMP_SENSOR_ADC_CHANNEL,
                        ADC_CONVERSION_TIME_US_256);
  halReadAdcBlocking(ADC_USER_APP, &value);
  volts = halConvertValueToVolts(value / TEMP_SENSOR_SCALE_FACTOR);
  volts = halConvertValueToVolts(value);
  temperatureMC = (1591887L - (171 * (int32_t)volts)) / 10;
#endif
  emberAfCorePrint("Reporting %ld to subscriber at ", temperatureMC);
  emberAfCoreDebugExec(emberAfPrintIpv6Address(&subscriberAddress));
  emberAfCorePrintln("");

  // Send this CoAP report to the border router when it tries to subscribes:
  // {
  //   "eui64":"1234123412341234",
  //   "temp":"27567"
  // }

  char tempString[25] = {0};

  // NULL JSON that returns the temp and ID of the device
  char jsonString[60] =
    "{\"eui64\":\"0000000000000000\",\"temp\":\"0\"}";

  char tempKey[] = "\",\"temp\":\"";
  char jsonClose[] = "\"}";

  // Put the temp in JSON, for the end of the string
  sprintf(tempString, "%d", temperatureMC);

  uint8_t strSz = strlen("{\"eui64\":\"");
  strncpy(jsonString + strSz, (char const*)euiString, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)euiString);

  // Put the temp in the JSON
  strncpy(jsonString + strSz, (char const*)tempKey, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)tempKey);
  strncpy(jsonString + strSz, (char const*)tempString, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)tempString);

  strncpy(jsonString + strSz, (char const*)jsonClose, sizeof(jsonString) - strSz);
  strSz += strlen((char const*)jsonClose);

  status = emberCoapPost(&subscriberAddress,
                         temperatureUri,
                         (uint8_t const*)jsonString,
                         strlen(jsonString),
                         &reportTemperatureStatusHandler);

  if (status == EMBER_SUCCESS) {
    setNextState(WAIT_FOR_DATA_CONFIRMATION);
  } else {
    emberAfCorePrintln("ERR: Reporting failed: 0x%x", status);
    repeatStateWithDelay(REPORT_PERIOD_MS);
  }
}

static void reportTemperatureStatusHandler(EmberCoapStatus status,
                                           EmberCoapMessage *coap,
                                           void *appData,
                                           uint16_t appDatalength)
{
  // We track the success or failure of reports so that we can determine when
  // we have lost the subscriber.  A series of consecutive failures is the
  // trigger to detach from the current subscriber and find a new one.  Any
  // successfully-transmitted report clears past failures.

  if (state == WAIT_FOR_DATA_CONFIRMATION) {
    if (status == EMBER_COAP_MESSAGE_ACKED) {
      failedReports = 0;
    } else {
      failedReports++;
      emberAfCorePrintln("ERR: Report timed out - failure %u of %u",
                         failedReports,
                         REPORT_FAILURE_LIMIT);
    }
    if (failedReports < REPORT_FAILURE_LIMIT) {
      setNextStateWithDelay(REPORT_TEMP_TO_SUBSCRIBER, REPORT_PERIOD_MS);
    } else {
      stopReportingTemperature();
    }
  }
}

void netResetHandler(const EmberCoapMessage *request)
{
  emberAfCorePrintln("Performing net reset from CoAP");
  // No CoAP response required, these will be non-confirmable requests
  resetNetworkState();
  rejoinNetwork();
}

void emberButtonPressIsr(uint8_t button, EmberButtonPress press)
{
  // Button 1 - triggers a net reset, then tries to join a network
  // Button 0 - if connected to a subsriber button 0 will send a CoAP request
  // to the subscriber's address

  switch (button) {
  case BUTTON0:
    switch (press) {
    case EMBER_SINGLE_PRESS: {
      emberAfCorePrintln("Button 0 Pressed");
      // Only report a button press if we are reporting to a subscriber
      if (state == REPORT_TEMP_TO_SUBSCRIBER ||
          state == WAIT_FOR_DATA_CONFIRMATION) {
        EmberStatus status;

        // Send the attach CoAP response to the border router:
        // {
        //   "eui64":"1234123412341234",
        // }

        // NULL JSON that contains the ID
        char jsonClose[3] = "\"}";
        char jsonString[32] = "{\"eui64\":\"0000000000000000\"}";
        uint8_t strSz = strlen("{\"eui64\":\"");

        strncpy(jsonString + strSz, (char const*)euiString, sizeof(jsonString) - strSz);
        strSz += strlen((char const*)euiString);
        strncpy(jsonString + strSz, (char const*)jsonClose, sizeof(jsonString) - strSz);
        strSz += strlen((char const*)jsonClose);

        status = emberCoapPost(&subscriberAddress,
                               buttonUri,
                               (uint8_t const*)jsonString,
                               strlen(jsonString),
                               &buttonPressStatusHandler);

        if (status != EMBER_SUCCESS) {
          emberAfCorePrintln("ERR: button report failed: 0x%x", status);
        }
      }
    } break;

    case EMBER_DOUBLE_PRESS:
      break;

    default:
      assert(false);
      break;
    }
    break;

  case BUTTON1:
    switch (press) {
    case EMBER_SINGLE_PRESS:
      emberAfCorePrintln("Button 1 Pressed");
      rejoinNetwork();
      break;

    case EMBER_DOUBLE_PRESS:
      break;

    default:
      assert(false);
      break;
    }
    break;

  default:
    assert(false);
    break;
  };
}

// rejoin-network
void rejoinNetworkCommand(void)
{
  rejoinNetwork();
}

static void rejoinNetwork(void)
{
  // This function will attempt to rejoin a network in all cases, if it is
  // already attached to a network it will perform a reset which automatically
  // tries to rejoin
  EmberNetworkStatus networkStatus = emberNetworkStatus();

  emberAfCorePrintln("Rejoin network issued. State: %s NetworkStatus: %s",
                     STATE_STRING(state),
                     NETWORK_STATUS_STRING(networkStatus));

  switch (networkStatus) {
  case EMBER_SAVED_NETWORK:
    joinAttemptsRemaining = SENSOR_ACTUATOR_JOIN_ATTEMPTS;
    setNextState(RESUME_NETWORK);
    break;
  case EMBER_NO_NETWORK:
  case EMBER_JOINED_NETWORK_ATTACHED:
  default:
    // If there is no saved network, then just set the join attempts to the
    // maximum, then reset the network state to try and trigger a join/rejoin
    joinAttemptsRemaining = SENSOR_ACTUATOR_JOIN_ATTEMPTS;
    setNextState(RESET_NETWORK_STATE);
    break;
  }
}
static void buttonPressStatusHandler(EmberCoapStatus status,
                                     EmberCoapMessage *coap,
                                     void *appData,
                                     uint16_t appDatalength)
{
  if (status != EMBER_COAP_MESSAGE_ACKED) {
    emberAfCorePrintln("ERR: Button press reporting failed");
  }
}

// stop-report
void stopReportCommand(void)
{
  // If we have a server and are reporting data, we can manually detach and try
  // to find a new server by using a CLI command.

  if (state == REPORT_TEMP_TO_SUBSCRIBER
      || state == WAIT_FOR_DATA_CONFIRMATION) {
    stopReportingTemperature();
  }
}

static void stopReportingTemperature(void)
{
  // We stop reporting temperature in response to failed reports (or a CLI
  // command). Once we detach, we wait for a new subscriber.

  assert(state == REPORT_TEMP_TO_SUBSCRIBER
         || state == WAIT_FOR_DATA_CONFIRMATION);

  emberAfCorePrintln("Stopped reporting to subscriber");

  setNextState(WAIT_FOR_SUBSCRIPTIONS);
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
    emberAfCorePrintln("Reset network state acknowledged");
    setNextState(JOIN_NETWORK); 
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
  case WAIT_FOR_JOIN_NETWORK:
    // Wait for joinNetwork to complete
#if defined(ENABLE_JOIN_OUT_OF_BAND)
    joinNetworkCommissionedCompletion();
#endif
    break;
  case WAIT_FOR_SUBSCRIPTIONS:
    waitForSubscriptions();
    break;
  case REPORT_TEMP_TO_SUBSCRIBER:
    reportTemperature();
    break;
  case WAIT_FOR_DATA_CONFIRMATION:
    break;
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

static void formatEuiString(uint8_t* euiString, size_t strSz, const uint8_t* euiBytes)
{
  uint8_t i;
  uint8_t strOffset = 0;
  uint8_t *euiStringEnd = euiString + strSz;

  assert (euiString != NULL);
  assert (strSz > 0);

  // Prints two nybles in hex format into euiString per byte in euiBytes
  for (i = 0; i < EUI64_SIZE && (euiString + strOffset < euiStringEnd); i++) {
    snprintf((char*)(euiString + strOffset),
              strSz,
              "%X%X",
              (euiBytes[7-i]>>4 & 0x0F),
              (euiBytes[7-i] & 0x0F));
    strOffset += 2;
  }
}

static const uint8_t *printableNetworkId(void)
{
  EmberNetworkParameters parameters = {{0}};
  static uint8_t networkId[EMBER_NETWORK_ID_SIZE + 1] = {0};
  emberGetNetworkParameters(&parameters);
  MEMCOPY(networkId, parameters.networkId, EMBER_NETWORK_ID_SIZE);
  return networkId;
}

#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
static char* nodeTypeToString(EmberNodeType nodeType)
{
  static char* returnString[] = { "UNKNOWN_DEVICE ",   // 0
                                  "ERROR          ",   // 1
                                  "ROUTER         ",   // 2
                                  "END_DEVICE     ",   // 3
                                  "SLEEPY_END_DEV ",   // 4
                                  "ERROR          ",   // 5
                                  "ERROR          ",   // 6
                                  "COMMISSIONER   "};  // 7
  switch (nodeType) {
  case EMBER_UNKNOWN_DEVICE:
    return returnString[EMBER_UNKNOWN_DEVICE];
    break;
  case EMBER_ROUTER:
  return returnString[EMBER_ROUTER];
    break;
  case EMBER_END_DEVICE:
    return returnString[EMBER_END_DEVICE];
    break;
  case EMBER_SLEEPY_END_DEVICE:
    return returnString[EMBER_SLEEPY_END_DEVICE];
    break;
  case EMBER_COMMISSIONER:
  return returnString[EMBER_COMMISSIONER];
    break;
  default:
    return returnString[1];
  break;
  }
}
#endif

void updateDisplayEventHandler(void)
{
  emberEventControlSetInactive(updateDisplayEventControl);
#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
  EmberNetworkStatus networkStatus;
  SPIDRV_Init_t initData = SPIDRV_MASTER_USART1;
  SPIDRV_HandleData_t handleData;
  SPIDRV_Handle_t handle = &handleData;

  initData.portLocationTx = PAL_SPI_USART_LOCATION_TX;
  initData.portLocationRx = 0;
  initData.portLocationClk = PAL_SPI_USART_LOCATION_SCLK;
  initData.portLocationCs = 0x00;
  initData.bitRate = PAL_SPI_BAUDRATE;
  initData.bitOrder = spidrvBitOrderLsbFirst;
  initData.csControl = spidrvCsControlApplication;

  SPIDRV_Init(handle, &initData);
  lcdPrintCurrentDeviceState();
  networkStatus = emberNetworkStatus();
  lcdPrintShortId();
  lcdPrintNetworkStatus(networkStatus, 15);
  SPIDRV_DeInit(handle);
  emberEventControlSetDelayMS(updateDisplayEventControl, LCD_REFRESH_MS);
#endif
}

// Utility functions provided to make writing to the LCD on the WSTK easier

#if defined(EMBER_AF_PLUGIN_GLIB) && defined(CORTEXM3_EFM32_MICRO)
void lcdPrintCurrentDeviceState(void)
{
  EmberNetworkStatus networkStatus;
  EmberNetworkParameters parameters = {{0}};
  uint8_t extendedPanId[EXTENDED_PAN_ID_SIZE + 1] = {0};
  char tempString[LCD_ASCII_SCREEN_WIDTH + 1] = {0};

  // Clear the screen
  GLIB_clear(&glibContext);

  // Print the join key
  lcdPrintJoinCode(joinKey, JOIN_KEY_ROW);

  // Print network status
  networkStatus = emberNetworkStatus();
  emberEventControlSetActive(updateDisplayEventControl);

  // If we are on a network, we can print other network parameters
  if (networkStatus != EMBER_NO_NETWORK) {
    uint8_t i;

    // Get network parameters
    emberGetNetworkParameters(&parameters);

    // Print Node Type
    sprintf(tempString, "Type: %s", nodeTypeToString(parameters.nodeType));
    GLIB_drawString(&glibContext,
                    tempString,
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    NODE_TYPE_ROW,
                    LCD_OPAQUE_VALUE);

    // Print network ID
    MEMCOPY(tempString, parameters.networkId, EMBER_NETWORK_ID_SIZE);
    GLIB_drawString(&glibContext,
                    tempString,
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    NETWORK_ID_ROW,
                    LCD_OPAQUE_VALUE);

    // Print the PAN ID
    MEMCOPY(extendedPanId, parameters.extendedPanId, EXTENDED_PAN_ID_SIZE);
    sprintf(tempString, "Pan ID: 0x%x", extendedPanId);
    GLIB_drawString(&glibContext,
                    tempString,
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    PAN_ID_ROW,
                    LCD_OPAQUE_VALUE);

    // Print channel
    sprintf(tempString, "Channel: %d", parameters.channel);
    GLIB_drawString(&glibContext,
                    tempString,
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    CHANNEL_ROW,
                    LCD_OPAQUE_VALUE);

    // Print IP addresses
    for (i = 0; i < EMBER_MAX_IPV6_ADDRESS_COUNT; i++) {
      EmberIpv6Address address;
      if (emberGetLocalIpAddress(i, &address)) {
        lcdPrintIpv6Address(&address,
                            IP_ADDRESS_INITIAL_ROW + ROW_SIZE * i,
                            LCD_ASCII_SCREEN_WIDTH);
      }
    }
  }
  DMD_updateDisplay();
}

void lcdPrintJoinCode(uint8_t *joinKey, uint8_t printRow)
{
// Don't show the join code if we are using out-of-band joining
#if !defined(ENABLE_JOIN_OUT_OF_BAND)
  char strJoinKey[LCD_ASCII_SCREEN_WIDTH + 1] = {0};
  // Note that the display is labeled "Conn Code" to match the commissioning
  // app's wording of Connect Code
  sprintf(strJoinKey, "Conn Code: %s", joinKey);
  GLIB_drawString(&glibContext,
                  strJoinKey,
                  PADDED_ASCII_BUFFER_SIZE,
                  START_COLUMN,
                  printRow,
                  LCD_OPAQUE_VALUE);
#endif
}

void lcdPrintNetworkStatus(EmberNetworkStatus status, uint8_t printRow)
{
  switch (status) {
  case EMBER_NO_NETWORK:
    GLIB_drawString(&glibContext,
                    "NS: NO_NETWORK",
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    printRow,
                    LCD_OPAQUE_VALUE);
    break;
  case EMBER_SAVED_NETWORK:
    GLIB_drawString(&glibContext,
                    "NS: SAVED_NETWORK",
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    printRow,
                    LCD_OPAQUE_VALUE);
    break;
  case EMBER_JOINING_NETWORK:
    GLIB_drawString(&glibContext, 
                    "NS: JOINING_NETWORK",
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    printRow,
                    LCD_OPAQUE_VALUE);
    break;
  case EMBER_JOINED_NETWORK_ATTACHING:
    GLIB_drawString(&glibContext,
                    "NS: JOINED_ATTACHING",
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    printRow,
                    LCD_OPAQUE_VALUE);
    break;
  case EMBER_JOINED_NETWORK_ATTACHED:
    GLIB_drawString(&glibContext,
                    "NS: JOINED_ATTACHED",
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    printRow,
                    LCD_OPAQUE_VALUE);
    break;
  case EMBER_JOINED_NETWORK_NO_PARENT:
    GLIB_drawString(&glibContext,
                    "NS: JOINED_NO_PARENT",
                    PADDED_ASCII_BUFFER_SIZE,
                    START_COLUMN,
                    printRow,
                    LCD_OPAQUE_VALUE);
    break;
  default:
    //PRINT ERROR
    break;
  }
}

static void lcdPrintShortId(void)
{
  char nodeIdString[LCD_ASCII_SCREEN_WIDTH + 1] = {0};
  EmberNodeId shortId = emberGetNodeId();

  sprintf(nodeIdString, "ShortID: %04x", shortId);
  GLIB_drawString(&glibContext,
                  nodeIdString,
                  PADDED_ASCII_BUFFER_SIZE,
                  START_COLUMN,
                  SHORT_ID_ROW,
                  LCD_OPAQUE_VALUE);
  DMD_updateDisplay();
}

void lcdPrintIpv6Address(const EmberIpv6Address *address,
                         uint8_t printRow,
                         uint8_t screenWidth)
{
  uint8_t addrString[EMBER_IPV6_ADDRESS_STRING_SIZE] = {0};
  char shortAddrString[LCD_ASCII_SCREEN_WIDTH + 1] = "....................";
  uint8_t i;
  uint8_t addrLen;

  emberIpv6AddressToString(address, addrString, sizeof(addrString));
  addrLen = strlen((char const*)addrString);

  // The LCD is too short to print the entire address so print the first set of
  // characters
  for (i = 0; i < IP_ADDRESS_CHAR_TRIM; i++) {
    shortAddrString[i] = addrString[i];
  }
  // Then print the last set of characters
  for (i = screenWidth - IP_ADDRESS_CHAR_TRIM; i < screenWidth; i++) {
    shortAddrString[i] = addrString[addrLen - screenWidth + i];
  }
  GLIB_drawString(&glibContext,
                  shortAddrString,
                  PADDED_ASCII_BUFFER_SIZE,
                  START_COLUMN,
                  printRow,
                  LCD_OPAQUE_VALUE);
}
#endif //EMBER_AF_PLUGIN_GLIB
