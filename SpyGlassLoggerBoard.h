/*
	SpyGlass Data Logger
	Copyright (C) 2014 James Cook University

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
*/

#ifndef _DROLoggerBoard_H_
#define _DROLoggerBoard_H_



#define RBG_R 11
#define RBG_G 10
#define RBG_B 9

#define DEBUG_RX_PIN 7
#define DEBUG_TX_PIN 6

#define POWERBEE_CONTROL_PIN  5
#define POWER_CONTROL_PIN     4
#define CLOCK_INT_PIN         2
#define ONE_WIRE_BUS_ONE_PIN  3


#define HUMIDITY_PIN A0
#define AMBIENT_LIGHT_PIN_LOW A1
#define AMBIENT_LIGHT_PIN_HIGH A2
#define SOUND_SENSOR A3
#define CHARGE_STATUS A6
#define BATERY_VOLTAGE A7


#define ADC_REFERENCE_VOLTAGE 3.3d
#define AMBIENT_LIGHT_MAX_ADC 775
#define SOUND_SAMPLE_PERIOD 2000


#endif
