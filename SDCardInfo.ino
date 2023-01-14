#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>
#include "LCD_Keypad_Reader.h"

#define SUPPORTS_HID true

#if SUPPORTS_HID
#include <Keyboard.h>

#define KEYBOARD_KEY_NEXT_CELL KEY_TAB
#define KEYBOARD_KEY_COUNT_NEXT_CELL 1
#define KEYBOARD_KEY_NEXT_ROW KEY_RETURN
#define KEYBOARD_KEY_COUNT_NEXT_ROW 2
// At the last row in Numbers, 2 return keystrokes are required to create a new row.
// Otherwise, use 1 return keystroke.
#endif

#define ONBOARD_LED 13

#define NO_KEY 0
#define UP_KEY 3
#define DOWN_KEY 4
#define LEFT_KEY 2
#define RIGHT_KEY 5
#define SELECT_KEY 1

#define MENU_SIZE 7
#define MENU_CID_INDEX 0
#define MENU_MID_INDEX 1
#define MENU_OID_INDEX 2
#define MENU_PNM_INDEX 3
#define MENU_PRV_INDEX 4
#define MENU_PSN_INDEX 5
#define MENU_MDT_INDEX 6

int currentPage = MENU_CID_INDEX;

LCD_Keypad_Reader keypad;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int currentKey = 0; // The current pressed key
int lastKey = -1;	// The last pressed key

unsigned long lastKeyCheckTime = 0;
unsigned long lastKeyPressTime = 0;

// Set up variables using the SD utility library functions
Sd2Card card;
SdVolume volume;
SdFile root;

// Change this to match your SD shield or module
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
// MKRZero SD: SDCARD_SS_PIN
const int chipSelect = 4;

String command = "";

bool isReading = false;
bool hasSDCard = false;
bool hasCID = false;

String cidString = "";
String midString = "";
String oidString = "";
String pnmString = "";
String prvString = "";
String psnString = "";
String mdtString = "";

void setup()
{
	// Turn off the on-board LED
	pinMode(ONBOARD_LED, OUTPUT);
	digitalWrite(ONBOARD_LED, LOW);

	// Open serial communications
	Serial.begin(9600);

	lcd.begin(16, 2);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("  SD Card Info  ");
	delay(2000);

	readCID();
	updateMenu();

#if SUPPORTS_HID
	Keyboard.begin();
#endif
}

void loop()
{
	while (Serial.available() > 0)
	{
		command += char(Serial.read());
		delay(2);
	}
	if (command.length() > 0)
	{
		if (command.equals("c"))
		{
			readCID();
			updateMenu();
		}
		else
		{
			Serial.println("Unknown command.");
		}
		command = "";
	}

	if (millis() > lastKeyCheckTime + keySampleRate)
	{
		lastKeyCheckTime = millis();
		currentKey = keypad.getKey();

		if (currentKey != lastKey)
		{
			processKey();
			lastKey = currentKey;
		}
	}
}

void processKey()
{
	if (currentKey == SELECT_KEY)
	{
		readCID();
		updateMenu();
	}
	else if (hasSDCard && hasCID)
	{
		switch (currentKey)
		{
		case UP_KEY:
			currentPage = (currentPage == 0) ? (MENU_SIZE - 1) : (currentPage - 1);
			updateMenu();
			break;
		case DOWN_KEY:
			currentPage = (currentPage == MENU_SIZE - 1) ? 0 : (currentPage + 1);
			updateMenu();
			break;
#if SUPPORTS_HID
		case RIGHT_KEY:
			sendKeystrokes();
			break;
#endif
		default:
			break;
		}
	}
}

void readCID()
{
	digitalWrite(ONBOARD_LED, HIGH);

	if (!card.init(SPI_HALF_SPEED, chipSelect))
	{
		hasSDCard = false;
		Serial.println("Initialization failed. No card is inserted.");
		Serial.println();
		return;
	}

	hasSDCard = true;

	cid_t cid;
	if (card.readCID(&cid))
	{
		hasCID = true;

		cidString = getHexStringFromCID(cid);
		midString = getStringWithLeadingZeros(String(cid.mid, HEX), 2);
		midString.toUpperCase();
		oidString = String(cid.oid[0]) + String(cid.oid[1]);
		pnmString = String(cid.pnm[0]) + String(cid.pnm[1]) + String(cid.pnm[2]) + String(cid.pnm[3]) + String(cid.pnm[4]);
		prvString = String(cid.prv_n) + "." + String(cid.prv_m);
		psnString = getStringWithLeadingZeros(String(cid.psn, HEX), 8);
		psnString.toUpperCase();
		mdtString = String(2000 + cid.mdt_year_low + 16 * cid.mdt_year_high) + "-" + getStringWithLeadingZeros(String(cid.mdt_month), 2);

		Serial.println();
		Serial.println(" CID                   | " + cidString);
		Serial.println("-----------------------+----------------------------------");
		Serial.println(" Manufacturer ID       | " + midString);
		Serial.println("-----------------------+----------------------------------");
		Serial.println(" OEM/Application ID    | " + oidString);
		Serial.println("-----------------------+----------------------------------");
		Serial.println(" Product Name          | " + pnmString);
		Serial.println("-----------------------+----------------------------------");
		Serial.println(" Product Revision      | " + prvString);
		Serial.println("-----------------------+----------------------------------");
		Serial.println(" Product Serial Number | " + psnString);
		Serial.println("-----------------------+----------------------------------");
		Serial.println(" Manufacturing Date    | " + mdtString);
		Serial.println();
	}
	else
	{
		hasCID = false;

		Serial.println("Reading CID failed.");
		Serial.println();
		return;
	}

	digitalWrite(ONBOARD_LED, LOW);
}

String getStringWithLeadingZeros(String input, int minimumLength)
{
	String result = input;
	for (int i = result.length(); i < minimumLength; i++)
	{
		result = "0" + result;
	}
	return result;
}

// References
// https://www.cameramemoryspeed.com/sd-memory-card-faq/reading-sd-card-cid-serial-psn-internal-numbers/
String getHexStringFromCID(cid_t cid)
{
	// unsigned long truncates the highest 16 bits for unknown reasons
	unsigned int bits127To112 = (cid.mid << (120 - 112)) | (cid.oid[0] << (112 - 112));
	unsigned int bits111To096 = (cid.oid[1] << (104 - 96)) | (cid.pnm[0] << (96 - 96));
	unsigned int bits095To080 = (cid.pnm[1] << (88 - 80)) | (cid.pnm[2] << (80 - 80));
	unsigned int bits079To064 = (cid.pnm[3] << (72 - 64)) | (cid.pnm[4] << (64 - 64));
	unsigned int bits063To048 = (cid.prv_n << (60 - 48)) | (cid.prv_m << (56 - 48)) | (cid.psn >> (48 - 24));
	unsigned int bits047To032 = (cid.psn >> (32 - 24));
	unsigned int bits031To016 = (cid.psn << (24 - 16)) | (cid.reserved << (20 - 16)) | (cid.mdt_year_high << (16 - 16));
	unsigned int bits015To000 = (cid.mdt_year_low << (12 - 0)) | (cid.mdt_month << (8 - 0)) | (cid.crc << (1 - 0)) | (cid.always1 << (0 - 0));

	String result = "";
	result += get16BitHexString(bits127To112);
	result += get16BitHexString(bits111To096);
	result += get16BitHexString(bits095To080);
	result += get16BitHexString(bits079To064);
	result += get16BitHexString(bits063To048);
	result += get16BitHexString(bits047To032);
	result += get16BitHexString(bits031To016);
	result += get16BitHexString(bits015To000);
	return result;
}

String get16BitHexString(unsigned int bits)
{
	String result = "";
	int totalBits = 16;
	int hexBits = 4;
	for (int i = 0; i < totalBits / hexBits; i++)
	{
		String hexDigitString = String((bits >> (totalBits - hexBits * (i + 1))) & 0xf, HEX);
		hexDigitString.toUpperCase();
		result += hexDigitString;
	}
	return result;
}

void updateMenu()
{
	lcd.clear();
	if (hasSDCard)
	{
		if (hasCID)
		{
			switch (currentPage)
			{
			case MENU_CID_INDEX:
				lcd.setCursor(0, 0);
				lcd.print(cidString.substring(0, 16));
				lcd.setCursor(0, 1);
				lcd.print(cidString.substring(16, 32));
				break;
			case MENU_MID_INDEX:
				lcd.setCursor(0, 0);
				lcd.print("Manuf. ID");
				lcd.setCursor(0, 1);
				lcd.print(midString);
				break;
			case MENU_OID_INDEX:
				lcd.setCursor(0, 0);
				lcd.print("OEM ID");
				lcd.setCursor(0, 1);
				lcd.print(oidString);
				break;
			case MENU_PNM_INDEX:
				lcd.setCursor(0, 0);
				lcd.print("Product Name");
				lcd.setCursor(0, 1);
				lcd.print(pnmString);
				break;
			case MENU_PRV_INDEX:
				lcd.setCursor(0, 0);
				lcd.print("Product Rev.");
				lcd.setCursor(0, 1);
				lcd.print(prvString);
				break;
			case MENU_PSN_INDEX:
				lcd.setCursor(0, 0);
				lcd.print("Product S/N");
				lcd.setCursor(0, 1);
				lcd.print(psnString);
				break;
			case MENU_MDT_INDEX:
				lcd.setCursor(0, 0);
				lcd.print("Manuf. Date");
				lcd.setCursor(0, 1);
				lcd.print(mdtString);
				break;
			default:
				break;
			}
		}
		else
		{
			lcd.setCursor(0, 0);
			lcd.print("   Failed to    ");
			lcd.setCursor(0, 1);
			lcd.print("    Read CID    ");
		}
	}
	else
	{
		lcd.setCursor(0, 0);
		lcd.print("   No SD Card   ");
	}
}

#if SUPPORTS_HID
void sendKeystrokes()
{
	Keyboard.print(cidString);
	goToNextCell();
	Keyboard.print(midString);
	goToNextCell();
	Keyboard.print(oidString);
	goToNextCell();
	Keyboard.print(pnmString);
	goToNextCell();
	Keyboard.print(prvString);
	goToNextCell();
	Keyboard.print(psnString);
	goToNextCell();
	Keyboard.print(mdtString);
	goToNextRow();
}

void goToNextCell()
{
	for (int i = 0; i < KEYBOARD_KEY_COUNT_NEXT_CELL; i++)
	{
		Keyboard.write(KEYBOARD_KEY_NEXT_CELL);
	}
}

void goToNextRow()
{
	for (int i = 0; i < KEYBOARD_KEY_COUNT_NEXT_ROW; i++)
	{
		Keyboard.write(KEYBOARD_KEY_NEXT_ROW);
	}
}
#endif
