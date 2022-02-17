#include <SPI.h>
#include <SD.h>

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

String cidString = "";
String midString = "";
String oidString = "";
String pnmString = "";
String prvString = "";
String psnString = "";
String mdtString = "";

void setup()
{
	// Open serial communications
	Serial.begin(9600);
}

void loop(void)
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
		}
		else
		{
			Serial.println("Unknown command.");
		}
		command = "";
	}
}

void readCID()
{
	if (!card.init(SPI_HALF_SPEED, chipSelect))
	{
		Serial.println("Initialization failed. No card is inserted.");
		Serial.println();
		return;
	}

	cid_t cid;
	if (card.readCID(&cid))
	{
		cidString = getHexStringFromCID(cid);
		midString = (cid.mid > 9 ? "" : "0") + String(cid.mid, HEX);
		midString.toUpperCase();
		oidString = String(cid.oid[0]) + String(cid.oid[1]);
		pnmString = String(cid.pnm[0]) + String(cid.pnm[1]) + String(cid.pnm[2]) + String(cid.pnm[3]) + String(cid.pnm[4]);
		prvString = String(cid.prv_n) + "." + String(cid.prv_m);
		psnString = String(cid.psn, HEX);
		psnString.toUpperCase();
		mdtString = String(2000 + cid.mdt_year_low + 16 * cid.mdt_year_high) + "-" + (cid.mdt_month > 9 ? "" : "0") + String(cid.mdt_month);

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
		Serial.println("Reading CID failed.");
		Serial.println();
		return;
	}
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
