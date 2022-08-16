// this software is (C) 2016-2022 by folkert@vanheusden.com
// AGPL v3.0

#include "kiss.h"

#include <RadioHead.h>
#include <RH_RF95.h> // LoRa device

// as debugging via serial is not possible (it is used for the kiss
// protocol), I use a couple of LEDs
#define pinLedError 3
#define pinLedRecv  4
#define pinLedSend  5
#define pinLedHB    6

#define pinReset    7

// this is an example implementation using a "RadioHead"-driver for
// RF95 radio (a LoRa device).
// RadioHead    : http://www.airspayce.com/mikem/arduino/RadioHead/
// GitHub mirror: https://github.com/mcauser/RadioHead
static RH_RF95 rf95;

// the 'kiss'-class requires a couple of callback functions. these
// functions do the low-level work so that the kiss-class is generic

// a call-back function which you can adjust into something that
// peeks in the radio-buffer if anything is waiting
uint16_t peekRadio() {
	return rf95.available();
}

void heartbeat() {
	const unsigned long int now = millis();
	static unsigned long int pHB = 0;

	if (now - pHB >= 500) {
		static bool state = true;

		if (pinLedHB != 0xff)
			digitalWrite(pinLedHB, state ? HIGH : LOW);

		state = !state;
		pHB = now;
	}
}

// if there's data in your radio, then this callback should retrieve it
void getRadio(uint8_t *const whereTo, uint16_t *const n) {
	heartbeat();
	uint8_t dummy = *n;
	rf95.recv(whereTo, &dummy);
	*n = dummy;
	heartbeat();
}

void putRadio(const uint8_t *const what, const uint16_t size) {
	heartbeat();
	rf95.send(what, size);
	heartbeat();
	rf95.waitPacketSent();
	heartbeat();
}

// some arduino-platforms (teensy, mega) have multiple serial ports
// there you need to replace Serial by e.g. Serial2
uint16_t peekSerial() {
	return Serial.available();
}

void getSerial(uint8_t *const whereTo, const uint16_t n) {
	for(uint16_t i=0; i<n; i++) {
		while(!Serial.available()) {
			heartbeat();
		}

		whereTo[i] = Serial.read();
	}
}

void putSerial(const uint8_t *const what, const uint16_t size) {
	Serial.write(what, size);
}

bool initRadio() {
	if (rf95.init()) {
		delay(100);

		if (pinLedRecv != 0xff)
			digitalWrite(pinLedRecv, LOW);
		if (pinLedSend != 0xff)
			digitalWrite(pinLedSend, LOW);
		if (pinLedError != 0xff)
			digitalWrite(pinLedError, LOW);
		if (pinLedHB != 0xff)
			digitalWrite(pinLedHB, LOW);

		rf95.setFrequency(869.525);	  // you may want to adjust this

	        rf95.setSpreadingFactor(12);      // spread factor 12
		rf95.setSignalBandwidth(125000);  // Bw = 125 kHz
		rf95.setCodingRate4(5);           // Cr = 4/5
		rf95.setPayloadCRC(true);         // CRC on
		rf95.setTxPower(13, false);
		rf95.spiWrite(RH_RF95_REG_39_SYNC_WORD, 0x34);

		return true;
	}

	return false;
}

bool resetRadio() {
	if (pinReset != 0xff) {
		digitalWrite(pinReset, LOW);
		delay(1); // at least 100us, this is 1000us
		digitalWrite(pinReset, HIGH);
		delay(5 + 1); // 5ms is required
	}

	return initRadio();
}

// LoRa device can have a packetsize of 254 bytes
kiss k(254, peekRadio, getRadio, putRadio, peekSerial, getSerial, putSerial, resetRadio, pinLedRecv, pinLedSend, pinLedError);

void setup() {
	// the arduino talks with 9600bps to the linux system
	Serial.begin(9600);

	if (pinLedRecv != 0xff) {
		pinMode(pinLedRecv, OUTPUT);
		digitalWrite(pinLedRecv, HIGH);
	}

	if (pinLedSend != 0xff) {
		pinMode(pinLedSend, OUTPUT);
		digitalWrite(pinLedSend, HIGH);
	}

	if (pinLedError != 0xff) {
		pinMode(pinLedError, OUTPUT);
		digitalWrite(pinLedError, HIGH);
	}

	if (pinLedHB != 0xff) {
		pinMode(pinLedHB, OUTPUT);
		digitalWrite(pinLedHB, HIGH);
	}

	if (pinReset != 0xff) {
		pinMode(pinReset, OUTPUT);
		digitalWrite(pinReset, HIGH);
	}

	k.begin();

	if (!resetRadio())
		k.debug("Radio init failed");

	k.debug("Go!");
}

void loop() {
	k.loop();

	heartbeat();
}
