/*
 * actuator_control.h
 *
 *  Created on: Nov 30, 2016
 *      Author: chsh4239
 */

#ifndef ACTUATOR_CONTROL_ACTUATOR_CONTROL_H_
#define ACTUATOR_CONTROL_ACTUATOR_CONTROL_H_

#include <em_gpio.h>
#include <em_cmu.h>



#define RELAY_PORT gpioPortD
#define RELAY_PIN0 10
#define RELAY_PIN1 11

typedef enum relay_pins{
	relaypin0 = RELAY_PIN0,
	relaypin1 = RELAY_PIN1
}relaypins_typdef;

//function declarations

void setRelayPin(relaypins_typdef relaypin);
void clearRelayPin(relaypins_typdef relaypin);
void InitRelayPins(void);
void actuatorOn();
void actuatorOff();


#endif /* ACTUATOR_CONTROL_ACTUATOR_CONTROL_H_ */
