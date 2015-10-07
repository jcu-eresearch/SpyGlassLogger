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

#include "SpyGlassLogger.h"


OneWire* busses[ONEWIRE_BUS_COUNT];
DallasTemperature* temps[ONEWIRE_BUS_COUNT];

Stream* debug;
Stream* data;
SoftwareSerial *uart;

upload_t upload;
record_t start_delim;
record_t end_delim;

uint8_t retry_count = 0;
boolean sleep = false;
boolean did_backup_sleep = false;


void setup()
{
	uint8_t *buf = (uint8_t *)&start_delim;
	uint8_t *buf2 = (uint8_t *)&end_delim;
	for(uint16_t i = 0; i < sizeof(record_t); i++)
	{
		buf[i] = i;
		buf2[sizeof(record_t) - i - 1] = i;
	}
	Serial.begin(9600);
	data = &Serial;

	uart = new SoftwareSerial(DEBUG_RX_PIN, DEBUG_TX_PIN);
	uart->begin(9600);

	debug = uart;
	debug->println("Starting...");


	busses[0] = new OneWire(ONE_WIRE_BUS_ONE_PIN);
	temps[0] = new DallasTemperature(busses[0]);
	debug->println(freeRam());
	debug->print("header_t size: ");
	debug->println(sizeof(header_t));
	debug->print("record_t size: ");
	debug->println(sizeof(record_t));
	debug->print("Logger function count: ");
	debug->println(sizeof(loggers)/2);
	data->setTimeout(ACK_TIMEOUT);


	PORTD |= 0x04;
    DDRD &=~ 0x04;
	RTC.set33kHzOutput(false);
	RTC.clearAlarmFlag(3);
	RTC.setSQIMode(sqiModeAlarm1);
}

void loop()
{
	debug->print("COLLISION_AVOID_INTERVAL: ");
	debug->println(COLLISION_AVOID_INTERVAL);
	power_up_radio();
	power_up_devices();
	clear_input();

	if(!repeat(&do_logging, 5, 100))
	{
		debug->println("Failed to send data.");
	}

	power_down_devices();
	power_down_radio();

	debug->println("Sleeping.");
	backup_sleep();
}

void INT0_ISR()
{

}

void backup_sleep()
{

	tmElements_t alarm;
	wake_up_at(RTC.get(), alarm);
	debug->print("Current Time: ");
	displayDate(RTC.get(), debug);
	debug->println();
	debug->print("Wake At: ");
	displayDate(alarm, debug);
	debug->println();
	RTC.clearAlarmFlag(3);
	RTC.writeAlarm(1, alarmModeDateMatch, alarm);
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	attachInterrupt(0, INT0_ISR, FALLING);

    cli();
    sleep_enable();      // Set sleep enable bit
    sleep_bod_disable(); // Disable brown out detection during sleep. Saves more power
    sei();

    //delay(30); //This delay is required to allow print to complete

    //Shut down all peripherals like ADC before sleep. Refer Atmega328 manual
    power_all_disable(); //This shuts down ADC, TWI, SPI, Timers and USART
    sleep_cpu();         // Sleep the CPU as per the mode set earlier(power down)
    sleep_disable();     // Wakes up sleep and clears enable bit. Before this ISR would have executed
    power_all_enable();  //This shuts enables ADC, TWI, SPI, Timers and USART
    delay(10); //This delay is required to allow CPU to stabilize

	did_backup_sleep = true;
	RTC.clearAlarmFlag(3);
	debug->print("Woke up at: ");
	displayDate(RTC.get(), debug);
	debug->println();
}

time_t wake_up_at(time_t current_time, tmElements_t &alarm)
{

	time_t waking_at = current_time - (current_time % LOG_INTERVAL) + LOG_INTERVAL + COLLISION_AVOID_INTERVAL;
	if((waking_at - current_time) < LOG_INTERVAL_THRESHOLD)
	{
		waking_at += LOG_INTERVAL;
	}
	breakTime(waking_at, alarm);
	return waking_at;
}

void power_up_devices()
{
	debug->println("Powering up devices.");
	pinMode(POWER_CONTROL_PIN, OUTPUT);
	digitalWrite(POWER_CONTROL_PIN, HIGH);
	delay(1000);
}

void power_down_devices()
{
	debug->println("Powering down devices.");
	digitalWrite(POWER_CONTROL_PIN, LOW);
}

void power_up_radio()
{
	pinMode(POWERBEE_CONTROL_PIN, OUTPUT);
	digitalWrite(POWERBEE_CONTROL_PIN, HIGH);
	delay(500);
}

void power_down_radio()
{
	digitalWrite(POWERBEE_CONTROL_PIN, LOW);
}

void clear_input()
{
	while(data->available())
	{
		data->read();
	}
}

bool do_logging(int repeat_count)
{
	data->write((uint8_t*)&start_delim, sizeof(record_t));
	header_t header = { DATA, RTC.get(), repeat_count};
	data->write((uint8_t*)&header, sizeof(header_t));
	upload.humidity_count = 0;
	upload.temperature_count = 0;
	for(int i = 0; i < ONEWIRE_BUS_COUNT; i++)
	{
		temps[i]->begin();
		temps[i]->requestTemperatures();
	}
	for(int i = 0; i < ONEWIRE_BUS_COUNT; i++)
	{
		debug->print("Logging Bus: ");
		debug->println(i);
		log_bus(i);
	}
	for(size_t i = 0; i < (sizeof(loggers)/2); i++)
	{
		loggers[i]();
	}

	debug->print("Temperature Count: ");
	debug->println(upload.temperature_count);
//	debug->print("Humidity Count: ");
//	debug->println(upload.humidity_count);
	debug->println();

	data->write((uint8_t*)&end_delim, sizeof(record_t));
	debug->println(freeRam());

	if(find_ack())
	{
		debug->println("ACK received.");
		return true;
	}
	debug->println("ACK not received.");
	return false;
}


void log_bus(uint8_t bus)
{
	busses[bus]->reset_search();
	uint8_t address[8];

	while(busses[bus]->search(address))
	{
		if (OneWire::crc8(address, 7) == address[7]) {

			switch(address[0])
			{
				case 0x28:
				case 0x10:
					log_temperature(bus, address);
					break;
				case 0x30:
					//log_humidity(bus, address);
					break;
				default:
					break;
			}
		}else
		{
			debug->println("Error");
		}
	}
}

void log_temperature(uint8_t bus, uint8_t *address)
{

	record_t record;
	record.record_type = RECORD_ONE_WIRE;
	record.value = temps[bus]->getTempC(address);
	memcpy(&record.address, address, 8);
	if(temps[bus]->getResolution(address) != 12)
	{
		temps[bus]->setResolution(address, 12);
	}

	data->write((uint8_t*)&record, sizeof(record_t));

	debug->print(' ');
	log_address(debug, address);
	debug->print(' ');
	debug->println(record.value);

	upload.temperature_count++;
}

void log_humidity()
{
	debug->println("Logging Humidity");
	record_t humidity;
	memset(humidity.address, 0, 8);
	humidity.record_type = RECORD_ADC;
	humidity.address[0] = HUMIDITY_PIN;

	pinMode(HUMIDITY_PIN, INPUT);
	humidity.value = analogRead(HUMIDITY_PIN);
	data->write((uint8_t*)&humidity, sizeof(record_t));

}

void log_ambient_light()
{
	debug->println("Logging Ambient Light");
	record_t ambient_light;
	memset(ambient_light.address, 0, 8);
	ambient_light.record_type = RECORD_ADC;
	ambient_light.address[0] = AMBIENT_LIGHT_PIN_LOW;

	pinMode(AMBIENT_LIGHT_PIN_LOW, INPUT);
	ambient_light.value = analogRead(AMBIENT_LIGHT_PIN_LOW);
	data->write((uint8_t*)&ambient_light, sizeof(record_t));
}

void log_sound()
{
	debug->println("Logging Sound");
	record_t sound;
	memset(sound.address, 0, 8);
	sound.record_type = RECORD_ADC;
	sound.address[0] = SOUND_SENSOR;

	pinMode(SOUND_SENSOR, INPUT);
	sound.value = analogRead(SOUND_SENSOR);
	data->write((uint8_t*)&sound, sizeof(record_t));
}

void log_battery_voltage()
{
	debug->println("Logging Battery Voltage");
	pinMode(BATERY_VOLTAGE, INPUT);

	record_t battery;
	memset(battery.address, 0, 8);
	battery.record_type = RECORD_ADC;
	battery.address[0] = BATERY_VOLTAGE;
	battery.value = analogRead(BATERY_VOLTAGE) * (1.1 / 1024)* (10+2)/2;
	data->write((uint8_t*)&battery, sizeof(record_t));
}

void log_charge_status()
{
	debug->println("Logging Charge Status");
	pinMode(CHARGE_STATUS, INPUT);

	record_t charge;
	memset(charge.address, 0, 8);
	charge.record_type = RECORD_ADC;
	charge.address[0] = CHARGE_STATUS;
	uint16_t val = analogRead(CHARGE_STATUS);

	if(val>900)
	  {
		charge.value = 0;//sleeping
	  }
	  else if(val>550)
	  {
		  charge.value = 1;//charging
	  }
	  else if(val>350)
	  {
		  charge.value = 2;//done
	  }
	  else
	  {
		  charge.value = 3;//error
	  }

	data->write((uint8_t*)&charge, sizeof(record_t));
}

void log_address(Stream* stream, uint8_t *address)
{
	for(int i = 0; i < 8; i++)
	{
		if(address[i] < 0x10)
		{
			stream->print(0, HEX);
		}
		stream->print(address[i], HEX);
	}
}

bool repeat(bool (*func)(int repeat_count), uint32_t count, uint32_t delayms)
{
	for(uint32_t i = 0; i < count; i++)
	{
		if(func(i))
		{
			return true;
		}
		delay(delayms);
	}
	return false;
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void displayDate(time_t time, Stream* displayOn)
{
	tmElements_t tm;
	breakTime(time, tm);
	displayDate(tm, displayOn);
}

void displayDate(tmElements_t &time, Stream* displayOn)
{
    char slash = '/';
    char colan = ':';
	displayOn->print(tmYearToCalendar(time.Year), DEC);
    displayOn->print(slash);
    LEADING_ZERO(displayOn, time.Month);
    displayOn->print(time.Month, DEC);
    displayOn->print(slash);
    LEADING_ZERO(displayOn, time.Day);
    displayOn->print(time.Day, DEC);
    displayOn->print(slash);
    LEADING_ZERO(displayOn, time.Hour);
    displayOn->print(time.Hour, DEC);
    displayOn->print(':');
    LEADING_ZERO(displayOn, time.Minute);
    displayOn->print(time.Minute, DEC);
    displayOn->print(colan);
    LEADING_ZERO(displayOn, time.Second);
    displayOn->print(time.Second, DEC);
}


bool find_ack()
{
	CircularBuffer ack_buf(sizeof(ack_t), debug);
	unsigned long start = millis();
	ack_t ack;


	while(((millis() - start) < ACK_TIMEOUT))
	{
		int val = data->read();
		if(val >= 0)
		{
			ack_buf.insert(val);
			if(ack_buf.endsWith(ACK, strlen(ACK)))
			{

				uint8_t *buf = (uint8_t*)&ack;
				for(size_t i = 0; i < sizeof(ack_t); i++)
				{
					buf[i] = ack_buf[i];
				}

				if(ack.magic_number == 0xBEEF)
				{
					handle_ack(&ack);
				}

				return true;
			}
		}
	}

	return false;
}

void handle_ack(ack_t* response)
{
	switch(response->command)
	{
	case 0xC10C:
		RTC.set((time_t)response->value);
		break;
	default:
		debug->println("Unknown ACK type");
	}
}
