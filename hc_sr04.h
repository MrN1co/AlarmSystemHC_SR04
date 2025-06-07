#ifndef HC_SR04_H
#define HC_SR04_H


#include "MKL05Z4.h"
#define TRIG_PIN 5 // Pin do sygnału TRIG
#define ECHO_PIN 6 // Pin do sygnału ECHO

#define TRIG_MASK (1 << TRIG_PIN) // Maska dla TRIG
#define ECHO_MASK (1 << ECHO_PIN) // Maska dla ECHO


/**
 * @brief Initializes the HC-SR04 ultrasonic distance sensor.
 *
 * @return None.
 */
void HC_SR04_Init(void);

/**
 * @brief Starts the HC-SR04 ultrasonic distance measurement.
 *
 * @return None.
 */
void HC_SR04_Start(void);

/**
 * @brief Gets the distance measured by the HC-SR04 sensor.
 *
 * @return Distance in centimeters.
 */
uint32_t HC_SR04_GetDistance(void);

#endif