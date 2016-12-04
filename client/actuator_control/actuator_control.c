/*
 * actuator.c
 *
 *  Created on: Nov 30, 2016
 *      Author: chsh4239
 */
#include <actuator_control/actuator_control.h>

void setRelayPin(relaypins_typdef relaypin)
{
    GPIO_PinOutSet(RELAY_PORT,relaypin);
}


void clearRelayPin(relaypins_typdef relaypin)
 {
   GPIO_PinOutClear(RELAY_PORT,relaypin);
 }

//Initialize the pins
 void InitRelayPins(void)
{
    CMU_ClockEnable(cmuClock_HFPER, true);
    CMU_ClockEnable(cmuClock_GPIO, true);
    GPIO_PinModeSet(RELAY_PORT, RELAY_PIN0, gpioModePushPull, 0);
    GPIO_PinModeSet(RELAY_PORT, RELAY_PIN1, gpioModePushPull, 0);
}

// ON the relay
 void actuatorOn(){
	 setRelayPin(RELAY_PIN1);// set the pin
	 clearRelayPin(RELAY_PIN0);//clear
	 halCommonDelayMicroseconds(100);//delay of 100 ms
 }


 // Off the relay
 void actuatorOff(){
	 clearRelayPin(RELAY_PIN0);
	 clearRelayPin(RELAY_PIN1);
 }
