#include <SPI.h>
#include <Ethernet.h>
#include <PubNub.h>
#include <stdio.h>
#include <math.h>
#include <aJSON.h>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <sstream>
#include <Time.h>


int temperaturePin = 0;
int photoPin = 1;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Memory saving tip: remove myI and dnsI from your sketch if you
// are content to rely on DHCP autoconfiguration.


char pubkey[] = "demo";
char subkey[] = "demo";
char channel[] = "demo1";


void setup()
{
  Serial.begin(9600);
  
 
  Serial.println("Serial set up");
 
  setTime(5,5,1,1,7,2013);
  
  while (!Ethernet.begin(mac)) {
 
    Serial.println("Ethernet setup error");
 
    delay(1000);
 
  }
 
  Serial.println("Ethernet set up");
 
  PubNub.begin(pubkey, subkey);
 
  Serial.println("PubNub set up");
    
}

float getVoltage(int pin){
  return (analogRead(pin) * .004882814);
}


int getPhoto(int pin) {
  return analogRead(pin);
}


float getTemp(int pin) 
{

  float temperature = getVoltage(pin);
  
  temperature = (((temperature - .5) * 100) * 1.8) + 32;
  
  return temperature;
}


char *getTime() 
{
  
  char timeString[200];
  time_t t = now();
  long myTime = (long)t;
  sprintf(timeString, "%lu", myTime);
  
  return timeString;
}


aJsonObject *createMessage()
{
	aJsonObject *msg = aJson.createObject();

	aJsonObject *sender = aJson.createObject();
	aJson.addStringToObject(sender, "name", "arduino");
	aJson.addNumberToObject(sender, "mac_last_byte", mac[5]);
	aJson.addItemToObject(msg, "sender", sender);

	int analogValues[6];
	for (int i = 0; i < 6; i++) {
		analogValues[i] = analogRead(i);
	}

	aJsonObject *analog = aJson.createIntArray(analogValues, 6);
	aJson.addItemToObject(msg, "analog", analog);
        aJson.addNumberToObject(msg, "temp", getTemp(temperaturePin));
        aJson.addNumberToObject(msg, "light", getPhoto(photoPin));
        aJson.addStringToObject(msg, "time", getTime());

	return msg;
}


/* Process message like: { "pwm": { "8": 0, "9": 128 } } */
void processPwmInfo(aJsonObject *item)
{
	aJsonObject *pwm = aJson.getObjectItem(item, "pwm");
	if (!pwm) {
		Serial.println("no pwm data");
		return;
	}

	const static int pins[] = { 8, 9 };
	const static int pins_n = 2;
	for (int i = 0; i < pins_n; i++) {
		char pinstr[3];
		snprintf(pinstr, sizeof(pinstr), "%d", pins[i]);

		aJsonObject *pwmval = aJson.getObjectItem(pwm, pinstr);
		if (!pwmval) continue; /* Value not provided, ok. */
		if (pwmval->type != aJson_Int) {
			Serial.print(" invalid data type ");
			Serial.print(pwmval->type, DEC);
			Serial.print(" for pin ");
			Serial.println(pins[i], DEC);
			continue;
		}

		Serial.print(" setting pin ");
		Serial.print(pins[i], DEC);
		Serial.print(" to value ");
		Serial.println(pwmval->valueint, DEC);
		analogWrite(pins[i], pwmval->valueint);
	}
}

void dumpMessage(Stream &s, aJsonObject *msg)
{
	int msg_count = aJson.getArraySize(msg);
	for (int i = 0; i < msg_count; i++) {
		aJsonObject *item, *sender, *analog, *value;
		s.print("Msg #");
		s.println(i, DEC);

		item = aJson.getArrayItem(msg, i);
		if (!item) { s.println("item not acquired"); delay(1000); return; }

		processPwmInfo(item);

		/* Below, we parse and dump messages from fellow Arduinos. */

		sender = aJson.getObjectItem(item, "sender");
		if (!sender) { s.println("sender not acquired"); delay(1000); return; }

		s.print(" mac_last_byte: ");
		value = aJson.getObjectItem(sender, "mac_last_byte");
		if (!value) { s.println("mac_last_byte not acquired"); delay(1000); return; }
		s.print(value->valueint, DEC);

		s.print(" A2: ");
		analog = aJson.getObjectItem(item, "analog");
		if (!analog) { s.println("analog not acquired"); delay(1000); return; }
		value = aJson.getArrayItem(analog, 2);
		if (!value) { s.println("analog[2] not acquired"); delay(1000); return; }
		s.print(value->valueint, DEC);

		s.println();
	}
}


void loop()
{
  
  Ethernet.maintain();
  EthernetClient *client;
  
  aJsonObject *msg = createMessage();
  char *msgStr = aJson.print(msg);
  aJson.deleteItem(msg);
  
  // msgStr is returned in a buffer that can be potentially
  // needlessly large; this call will "tighten" it
  
  msgStr = (char *) realloc(msgStr, strlen(msgStr) + 1);

  Serial.println(msgStr);

  client = PubNub.publish(channel, msgStr);
  
  free(msgStr);

  client->stop();
  
  delay(1000);

}
  



  
