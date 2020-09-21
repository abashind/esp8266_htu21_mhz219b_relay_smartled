#include <Wire.h>
#include <Sodaq_SHT2x.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

#define CONTROL_PIN D11

#define WS2812B_PIN D8
#define NUM_PIXELS 1
Adafruit_NeoPixel pixels(NUM_PIXELS, WS2812B_PIN, NEO_GRB + NEO_KHZ800);

#define CO2_TX D12
#define CO2_RX D13
SoftwareSerial CO2_SENSOR_SERIAL(CO2_RX, CO2_TX);
byte measure_cmd[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
byte set_range_5000_cmd[9] = { 0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB };
//unsigned char response[7];
unsigned char measure_response[9];
int co2 {-1};

char auth[] = "JTX0UsGH4KiNDbThk0NfrFeTqI8fo90v";
char ssid[] = "7SkyHome";
char pass[] = "89191532537";

float hum = 0;
float temp = 0;
float dew_point = 0;
int manual_mode = 1;
int humidifier_state = 0;

BlynkTimer timer;

BLYNK_WRITE(V3)
{
	humidifier_state = param.asInt();
}

BLYNK_WRITE(V4)
{
	manual_mode = param.asInt();
}

void setup()
{
	Serial.begin(9600);
	pixels.begin();
	Blynk.begin(auth, ssid, pass);
	Wire.begin();
	
//	unsigned char set_range_5000_response[9]; 
//	CO2_SENSOR_SERIAL.write(set_range_5000_cmd, 9);
//	CO2_SENSOR_SERIAL.readBytes(set_range_5000_response, 9);
//	byte crc = 0;
//	for (int i = 1; i < 8; i++) 
//		crc += set_range_5000_response[i];
//	crc = 255 - crc + 1;
//	if (!(set_range_5000_response[0] == 0xFF && set_range_5000_response[1] == 0x99 && set_range_5000_response[8] == crc)) {
//		Serial.println("Range CRC error: " + String(crc) + " / " + String(set_range_5000_response[8]) + " (bytes 6 and 7)");
//	}
//	else {
//		Serial.println("Range was set! (bytes 6 and 7)");
//	}
	delay(1000);
	
	pinMode(CONTROL_PIN, OUTPUT);
	timer.setInterval(5000L, do_it);
}

void readCO2() {

	CO2_SENSOR_SERIAL.write(measure_cmd, 9);
	CO2_SENSOR_SERIAL.readBytes(measure_response, 9);
	byte crc = 0;
	for (int i = 1; i < 8; i++) 
		crc += measure_response[i];
	crc = 255 - crc + 1;
	if (!(measure_response[0] == 0xFF && measure_response[1] == 0x86 && measure_response[8] == crc)) {
		Serial.println("CRC error: " + String(crc) + " / " + String(measure_response[8]));
	} 
	unsigned int responseHigh = (unsigned int) measure_response[2];
	unsigned int responseLow = (unsigned int) measure_response[3];
	unsigned int ppm = (256*responseHigh) + responseLow;
	co2 = ppm; //* 0.4;
}

void do_it()
{
	hum = SHT2x.GetHumidity();
	temp = SHT2x.GetTemperature();
	dew_point = SHT2x.GetDewPoint();
	readCO2();
	
	Serial.println(" Humidity(%RH): " + String(hum));
	Serial.println(" Temperature(C): " + String(temp));
	Serial.println(" Dewpoint(C): " + String(dew_point));
	Serial.println(" Manual mode: " + String(manual_mode));
	Serial.println(" Humidifier state: " + String(humidifier_state));
	Serial.println(" CO2, ppm: " + String(co2));

	if (humidifier_state == 1)
		digitalWrite(CONTROL_PIN, HIGH);
	else
		digitalWrite(CONTROL_PIN, LOW);

	if (manual_mode != 1)
	{
		if (hum <= 40)
			humidifier_state = 1;
		else
			humidifier_state = 0;
		Blynk.virtualWrite(V3, humidifier_state);
	}
	
	pixels.clear();
	pixels.setBrightness(10);
	if (co2 == -1)
	{
		pixels.setPixelColor(0, pixels.Color(255, 0, 255));  //Blue
	}
	
	if (co2 <= 600 && co2 != -1)
	{
		pixels.setPixelColor(0, pixels.Color(0, 255, 0)); //Green
	}
	
	if (co2 > 600 && co2 <= 800) 
	{
		pixels.setPixelColor(0, pixels.Color(255, 255, 0)); //Yellow
	}	
	
	if (co2 > 800 && co2 <= 1000)
	{
		pixels.setPixelColor(0, pixels.Color(255, 140, 0));  //Dark orange
	}
	
	if(co2 > 1000)
	{
		pixels.setPixelColor(0, pixels.Color(255, 0, 0));  // Red
	}
	pixels.show();
	
	Blynk.virtualWrite(V0, hum);
	Blynk.virtualWrite(V1, temp);
	Blynk.virtualWrite(V2, dew_point);
	Blynk.virtualWrite(V6, co2);
}
 
void loop()
{
	Blynk.run();
	timer.run();
}

