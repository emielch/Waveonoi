
elapsedMillis sinceAlarmCheck = 0;

void alarmLoop() {
  if (sinceAlarmCheck < 1000) return;
  sinceAlarmCheck = 0;


  
  int weekday = getWeekDay(now());
  if (weekday == 6 || weekday == 0){  // if it's saturday or sunday
    turnOff();
    return;
  }

  unsigned long currT = now() % (60 * 60 * 24);
  if (onTime < offTime) {
    if (currT > onTime && currT < offTime) turnOn();
    else turnOff();
  }
  else {
    if (currT > offTime && currT < onTime) turnOff();
    else turnOn();
  }

}

void printCurrTime() {
  Serial.print("Curr: \t");
  printTime(now());
  Serial.print("Weekday: \t");
  Serial.println(getWeekDay(now()));
  Serial.print("ON: \t");
  printTime(onTime);
  Serial.print("OFF: \t");
  printTime(offTime);
  Serial.println();
}

unsigned long now() {
  return Teensy3Clock.get();
}
void setTime(unsigned long t) {
  Teensy3Clock.set(t);
}



void printTime(long t) {
  Serial.print(getHours(t));
  printDigits(getMinutes(t));
  printDigits(getSeconds(t));
  Serial.println();
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

int getWeekDay(unsigned long val) {  // 0 is sunday, 1 is monday, etc...
  return (val / 60 / 60 / 24 + 4) % 7;
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
