// This file is generated by Simplicity Studio.  Please do not edit manually.
//
//

#include PLATFORM_HEADER
#include CONFIGURATION_HEADER
#include EMBER_AF_API_STACK

bool emberAfOkToSleep(uint32_t durationMs);

void emberAfPluginIdleSleepWakeUp(uint32_t durationMs);

bool emberAfOkToIdle(uint32_t durationMs);

void emberAfPluginIdleSleepActive(uint32_t durationMs);

void emAfMain(MAIN_FUNCTION_PARAMETERS);

void emAfInit(void);

void emAfTick(void);

void emAfMarkApplicationBuffers(void);

void emberNetworkStatusHandler(EmberNetworkStatus newNetworkStatus, EmberNetworkStatus oldNetworkStatus, EmberJoinFailureReason reason);
