/* 
 * File:   temperature.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 10:42 AM
 */

#ifndef TEMPERATURE_H
#define	TEMPERATURE_H

#define TEMP_NOT_INITIALIZED -99.0
#define TEMP_INVALID_TEMP -98.0
#define TEMP_INVALID_NAME -97.0

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Get the temperature of a thermistor and print out
 * the result to the rimbox. a0 through a7
 * @param args the args passed in from the command processor. 
 *             Only the first index should contain a thermistor name.
 */
void gettemp_command(char *args[]);

/**
 * Get the temperature of one of the sensors.
 * @param name the name of the thermistor. a0 through a7
 * @return temp in degrees C. ON_BOARD_TEMP_NOT_INITIALIZED or 
 *   ON_BOARD_TEMP_INVALID_TEMP
 */
float get_temp(const char *name);

#ifdef	__cplusplus
}
#endif

#endif	/* TEMPERATURE_H */

