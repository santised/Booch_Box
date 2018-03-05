#include <FastLED.h>
#include <Wire.h>
#include <SparkFunCCS811.h>
#include <SparkFunBME280.h>
#include <SFE_MicroOLED.h>
#include <OneWire.h>  

#define BME_ADDR 0x77
#define CCS811_ADDR 0x5B

#define PIN_RESET 9
#define DC_JUMPER 10
#define DATA_OUT 11
#define CLOCK_OUT 13
#define NUM_LEDS 2
#define LED_TYPE APA102
#define COLOR_ORDER BGR
#define UPDATES_PER_SECOND 50
#define BRIGHTNESS 60 
#define dsmTherm 2 
//#define heatPad 
#define capButton 8


OneWire ds(dsmTherm);
MicroOLED oled(PIN_RESET, DC_JUMPER);
CCS811 myCCS811(CCS811_ADDR);
BME280 myBME; 
CRGB leds[NUM_LEDS];
CRGBPalette16 currentPalette;
TBlendType currentBlending;

extern const TProgmemPalette16 hotCold_p PROGMEM; 

void setup() 
{  
  Serial.begin(57600); 
  delay(20);
  oled.begin();
  oled.clear(ALL);
  oled.display();
  print_OLED(0);
  FastLED.addLeds<LED_TYPE, DATA_OUT, CLOCK_OUT, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  currentPalette = hotCold_p;
  currentBlending = LINEARBLEND;
  
	CCS811Core::status returnCode = myCCS811.begin();
	printDriverError(returnCode);
	Serial.println("");

  myBME.settings.commInterface = I2C_MODE;
  myBME.settings.I2CAddress = BME_ADDR;
  myBME.settings.runMode=3; 
  myBME.settings.tStandby=0; 
  myBME.settings.filter=4; 
  myBME.settings.tempOverSample=5; 
  myBME.settings.pressOverSample=5; 
  myBME.settings.humidOverSample=5; 
	delay(30);

	Serial.print("Starting BME....0x ");
	uint8_t bmeBeginValue = myBME.begin();
	Serial.println(	bmeBeginValue, HEX );

	//pinMode(ther, OUTPUT);
	//pinMode(heatPad, OUTPUT);
	pinMode(capButton, INPUT);
  print_OLED(1);
}

void loop() 
{
	//if( get_booch_temp() < 75 ) digitalWrite(heatPad, HIGH); 
	//else if( get_booch_temp() > 85 ) digitalWrite(heatPad, LOW); 
	if( digitalRead(capButton) == HIGH )
	{
    delay(300);
    //0 = Temp
    //1 = Humidity
    print_OLED(2);
		float ambAir = get_ambient_air();
    int boochTempF = get_booch_temp();
    print_box_OLED(3, boochTempF); 
	}
 delay(350);
} 


float get_ambient_air()
{
  print_OLED(5); 
	boolean foundData = false; 
	float BMEtempC = myBME.readTempC();
  int BMEtempF = myBME.readTempF();
  print_box_OLED(0, BMEtempF); 
	int BMEhumid = myBME.readFloatHumidity();
  print_box_OLED(1, BMEhumid); 
	myCCS811.setEnvironmentalData(BMEhumid, BMEtempC);
	Serial.println("Checking CCS811 for data"); 
	for( int i=0; i < 10; i++ )
	{	
		if( myCCS811.dataAvailable() )
		{
			myCCS811.readAlgorithmResults();
      delay(100);
      int BMECO2 = myCCS811.getCO2(); 
      print_box_OLED(2, BMECO2); 
			foundData = true; 
			return true;
		}
		else if( myCCS811.checkForStatusError() )
		{
			printSensorError();
			foundData = false;
      print_OLED(4);
			return false;
		}
   delay(200);
	}
	if( foundData == false ) Serial.println(" NO AIR DATA "); 
}

void printDriverError( CCS811Core::status errorCode )
{
  switch ( errorCode )
  {
    case CCS811Core::SENSOR_SUCCESS:
      Serial.print("SUCCESS");
      break;
    case CCS811Core::SENSOR_ID_ERROR:
      Serial.print("ID_ERROR");
      break;
    case CCS811Core::SENSOR_I2C_ERROR:
      Serial.print("I2C_ERROR");
      break;
    case CCS811Core::SENSOR_INTERNAL_ERROR:
      Serial.print("INTERNAL_ERROR");
      break;
    case CCS811Core::SENSOR_GENERIC_ERROR:
      Serial.print("GENERIC_ERROR");
      break;
    default:
      Serial.print("Unspecified error.");
  }
}

void printSensorError()
{
  uint8_t error = myCCS811.getErrorRegister();

  if ( error == 0xFF ) //comm error
  {
    Serial.println("Failed to get ERROR_ID register.");
  }
  else
  {
    Serial.print("Error: ");
    if (error & 1 << 5) Serial.print("HeaterSupply");
    if (error & 1 << 4) Serial.print("HeaterFault");
    if (error & 1 << 3) Serial.print("MaxResistance");
    if (error & 1 << 2) Serial.print("MeasModeInvalid");
    if (error & 1 << 1) Serial.print("ReadRegInvalid");
    if (error & 1 << 0) Serial.print("MsgInvalid");
    Serial.println();
  }
}	

const TProgmemPalette16 hotCold_p PROGMEM=
{
   CRGB::Red,
   CRGB::Orange,
   CRGB::Blue,
   CRGB::Green
};

void print_OLED(int toDisplay)
{
 //0 = "Start up"
 //1 = "Ready"
 //2 = "Getting Box Information"
 //3 = "Turning on heating Pad"
 //4 = "Could not read data."
 //5 = Clear page
  oled.clear(PAGE);
  oled.setFontType(0);
  oled.setCursor(0, 0);
  oled.display(); 

  if( toDisplay == 0 ) 
  {
  oled.print("Start up."); 
  oled.display();
  delay(2000);
  oled.clear(PAGE);
  oled.setCursor(0, 0);
  oled.display(); 
  }
  
  if( toDisplay == 1 ) 
  {
  oled.print("Ready."); 
  oled.display();
  }
  
  if( toDisplay == 2 ) 
  {
  oled.print("Getting"); 
  oled.setCursor(0, 8);
  oled.print("Box Info");
  oled.display();
  delay(2000);
  oled.clear(PAGE);
  oled.setCursor(0, 21);
  oled.display(); 
  }
  
  if( toDisplay == 4 ) 
  {
  oled.print("Error"); 
  oled.display();
  delay(2000);
  oled.clear(PAGE);
  oled.setCursor(0, 0);
  oled.display(); 
  }
  
  if( toDisplay == 5 ) 
  {
  oled.clear(PAGE);
  oled.setFontType(0);
  oled.setCursor(0, 0);
  oled.display(); 
  }
}

void print_box_OLED(int object, int reading)
{
  if( object == 0 )
  {
    oled.print("Temp:");
    oled.print(reading);
    oled.print("F"); 
    oled.display();
  }
  if( object ==  1 )
  {
    oled.setCursor(0, 8);
    oled.print("Hum:");
    oled.print(reading); 
    oled.print("RH");
    oled.display();     
  }
  if( object == 2 )
  {
    oled.setCursor(0,16);
    oled.print("CO2:");
    oled.print(reading);    
    oled.display(); 

  }
  if( object == 3)
  {
    oled.setCursor(0, 24);
    oled.print("Booch:");
    oled.print(reading); 
    oled.display();    
    delay(7000); 
    oled.clear(PAGE); 
    oled.setCursor(0, 0);
    oled.display(); 
    print_OLED(1); 
  }
}

float get_booch_temp()
{
  //returns the temperature from one DS18S20 in DEG Celsius
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  float tempCelcius = (TemperatureSum * 18 + 5)/10 + 32;
  return (tempCelcius * 1.8 + 32); //Converting to Farenheit
}

