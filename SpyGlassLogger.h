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

#ifndef _DROLogger_H_
#define _DROLogger_H_

#include "Arduino.h"
#include "OneWire.h"
#include "Time.h"
#include "DS3232RTC.h"
#include "DallasTemperature.h"
#include "SoftwareSerial.h"

#include "CircularBuffer.h"

#include "SpyGlassLoggerBoard.h"

#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/power.h>


#define ONEWIRE_BUS_COUNT 1
#define MAX_RETRIES 5
#define ACK "OK"
#define LOG_INTERVAL 15
#define LOG_INTERVAL_THRESHOLD 1
#define NODE_ID 2
#define COLLISION_AVOID_INTERVAL (NODE_ID * 3)
#define ACK_TIMEOUT 5000



#define LEADING_ZERO(STREAM, value) if(value < 10){STREAM->print(0);}

#ifndef sleep_bod_disable()
#define sleep_bod_disable() \
{ \
  uint8_t tempreg; \
  __asm__ __volatile__("in %[tempreg], %[mcucr]" "\n\t" \
                       "ori %[tempreg], %[bods_bodse]" "\n\t" \
                       "out %[mcucr], %[tempreg]" "\n\t" \
                       "andi %[tempreg], %[not_bodse]" "\n\t" \
                       "out %[mcucr], %[tempreg]" \
                       : [tempreg] "=&d" (tempreg) \
                       : [mcucr] "I" _SFR_IO_ADDR(MCUCR), \
                         [bods_bodse] "i" (_BV(BODS) | _BV(BODSE)), \
                         [not_bodse] "i" (~_BV(BODSE))); \
}
#endif



void backup_sleep();
void power_up_devices();
void power_down_devices();
void power_down_radio();
void power_up_radio();
void INT0_ISR();
time_t wake_up_at(time_t current_time, tmElements_t &alarm);

void clear_input();
int freeRam();
void displayDate(time_t time, Stream* displayOn);
void displayDate(tmElements_t &time, Stream* displayOn);
bool find_ack();

bool do_logging(int repeat_count);
void log_bus(uint8_t bus);
void log_temperature(uint8_t bus, uint8_t *address);
void log_address(Stream* stream, uint8_t *address);
void log_humidity();
void log_ambient_light();
void log_sound();
void log_battery_voltage();
void log_charge_status();
bool repeat(bool (*func)(int repeat_count), uint32_t count, uint32_t delayms);

void (*loggers[])(void) = {
		log_humidity,
		log_ambient_light,
		log_sound,
		log_battery_voltage,
		log_charge_status
};


enum message_t
{
	DATA = 1,
	ERROR
};

struct header_t
{
	message_t type;
	time_t ts;
	uint32_t code;
};

struct record_t
{
	uint8_t record_type;
	uint8_t address[8];
	double value;
};

enum record_types_e
{
	RECORD_INVALID = 0,
	RECORD_ONE_WIRE = 1,
	RECORD_ADC = 2,
};

struct ack_t
{
	uint32_t magic_number;
	uint32_t command;
	uint32_t value;
	char ack[sizeof(ACK) -1];
};

struct upload_t
{
	uint16_t temperature_count;
	uint16_t humidity_count;
};


void handle_ack(ack_t* response);

#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
}
#endif

#endif
