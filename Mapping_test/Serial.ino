#define INPUT_SIZE 200
char serialBuf[INPUT_SIZE + 1];
int currSerialPos = 0;


void checkSerial() {
	while (Serial1.available()) {
		char inChar = (char)Serial1.read();
		if (inChar == '\n' || inChar == '\r') {
			if (!currSerialPos == 0) {
				serialBuf[currSerialPos] = 0;  // add termination character
				parseSerial();
				currSerialPos = 0;
			}
		}
		else if (currSerialPos < INPUT_SIZE) {
			serialBuf[currSerialPos] = inChar;
			currSerialPos++;
		}
	}
}


void parseSerial() {
	int messID = atoi(strtok(serialBuf, ","));

	if (messID == 1) parseMasterSwitch();
	if (messID == 2) parseEpoch();
	if (messID == 3) parseTimer();
}


void parseMasterSwitch() {
	int masterSW = atoi(strtok(0, ","));
	
	masterSwitch = masterSW;
	if (masterSwitch) turnOn();
	else turnOff();
}

void parseEpoch() {
	time_t ts = atoll(strtok(0, ","));
	setTime(ts);

	Serial.print("NEW TIME:\t");
	Serial.print(getHours(ts));
	printDigits(getMinutes(ts));
	printDigits(getSeconds(ts));
	Serial.println();

	setAlarms();
	checkOnOff();
}

void parseTimer() {
	onTime = atol(strtok(0, ","));
	offTime = atol(strtok(0, ","));

	Serial.println("NEW ALARMS: ");
	Serial.print("ON: \t");
	printTime(onTime);
	Serial.print("OFF: \t");
	printTime(offTime);
	Serial.println();

	setAlarms();
	checkOnOff();
}

void checkOnOff() {
	long currT = hour() * 60 * 60 + minute() * 60 + second();

	if (onTime < offTime) {
		if (currT > onTime && currT < offTime) turnOn();
		else turnOff();
	}
	else {
		if (currT > offTime && currT < onTime) turnOff();
		else turnOn();
	}
}

void setAlarms() {
	Alarm.free(onAlarmID);
	Alarm.free(offAlarmID);

	onAlarmID = Alarm.alarmRepeat(getHours(onTime), getMinutes(onTime), getSeconds(onTime), turnOn);
	offAlarmID = Alarm.alarmRepeat(getHours(offTime), getMinutes(offTime), getSeconds(offTime), turnOff);
}

int getHours(unsigned long val) {
	return val / 60 / 60 - (val / 60 / 60 / 24 * 24);
}
int getMinutes(unsigned long val) {
	return val / 60 - (val / 60 / 60 * 60);
}
int getSeconds(unsigned long val) {
	return val - (val / 60 * 60);
}