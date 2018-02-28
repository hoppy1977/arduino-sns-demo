#include "sha1.h"
#include <Ethernet.h>
#include <HttpClient.h>
#include <RTClib.h>

// Dependencies:
// - RTClib Library
// https://github.com/adafruit/RTClib


// ===============
// Amazon AWS configuration
#define     AWS_ACCESS_KEY         "TODO:"											// Put your AWS access key here.
#define     AWS_SECRET_ACCESS_KEY  "TODO:"						// Put your AWS secret access key here.
#define     AWS_REGION             "ap-southeast-2"													// The region where your SNS topic lives.
																									// See the table at: http://docs.aws.amazon.com/general/latest/gr/rande.html#sns_region
#define     AWS_HOST               "sns.ap-southeast-2.amazonaws.com"								// The host URL for the region where your SNS topic lives.
																									// See the table at: http://docs.aws.amazon.com/general/latest/gr/rande.html#sns_region
#define     SNS_TOPIC_ARN          "arn%3Aaws%3Asns%3Aap-southeast-2%3A244360212732%3AArduionDemo"	// Amazon resource name (ARN) for the SNS topic to receive notifications.
																									// Note: This ARN _MUST_ be URL encoded!  See http://meyerweb.com/eric/tools/dencoder/ for an example URL encoder tool.
// ===============
// SHA1 hash details
#define     SHA1_HASH_LENGTH       20

// ===============
// MAC address for your Ethernet shield
byte mac[] = { 0x96, 0x97, 0xFC, 0x1D, 0xF4, 0xA2 }; // Randomly selected

EthernetClient client;

void setup()
{
	// Set up the serial port connection.
	Serial.begin(9600);
	Serial.println("Initializing...");

	// Setting up ethernet connection
	while (Ethernet.begin(mac) != 1)
	{
		Serial.println("Error getting IP address via DHCP, trying again...");
		delay(5000);
	}

	Serial.print("IP address: ");
	for (byte thisByte = 0; thisByte < 4; thisByte++)
	{
		// print the value of each byte of the IP address:
		Serial.print(Ethernet.localIP()[thisByte], DEC);
		Serial.print(".");
	}
	Serial.println();

	Serial.println("Ready...");
}

void loop()
{
	snsPublish(SNS_TOPIC_ARN, "It%20is%20working%21");
	delay(30000);
}

// Publish a message to an SNS topic.
// Note, both the topic and message strings _MUST_ be URL encoded before calling this function!
void snsPublish(const char* topic, const char* message)
{
	unsigned long currentTime = 1519824283; // TODO:

  // Set dateTime to the URL encoded ISO8601 format string.
	DateTime dt(currentTime);
	char dateTime[25];
	memset(dateTime, 0, 25);
	dateTime8601UrlEncoded(dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second(), dateTime);

	// Generate the signature for the request.
	// For details on the AWS signature process, see: 
	//   http://docs.aws.amazon.com/general/latest/gr/signature-version-2.html
	Sha1.initHmac((uint8_t*)AWS_SECRET_ACCESS_KEY, strlen(AWS_SECRET_ACCESS_KEY));
	Sha1.print(F("POST\n"));
	Sha1.print(AWS_HOST);
	Sha1.print(F("\n"));
	Sha1.print(F("/\n"));
	Sha1.print(F("AWSAccessKeyId="));
	Sha1.print(AWS_ACCESS_KEY);
	Sha1.print(F("&Action=Publish"));
	Sha1.print(F("&Message="));
	Sha1.print(message);
	Sha1.print(F("&SignatureMethod=HmacSHA1"));
	Sha1.print(F("&SignatureVersion=2"));
	Sha1.print(F("&Timestamp="));
	Sha1.print(dateTime);
	Sha1.print(F("&TopicArn="));
	Sha1.print(topic);
	Sha1.print(F("&Version=2010-03-31"));

	// Convert signature to base64
	// Adapted from Adafruit code for SendTweet example.
	uint8_t *in, out, i, j;
	char b64[27];
	memset(b64, 0, sizeof(b64));
	static const char PROGMEM b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	for (in = Sha1.resultHmac(), out = 0; ; in += 3) { // octets to sextets
		b64[out++] = in[0] >> 2;
		b64[out++] = ((in[0] & 0x03) << 4) | (in[1] >> 4);
		if (out >= 26) break;
		b64[out++] = ((in[1] & 0x0f) << 2) | (in[2] >> 6);
		b64[out++] = in[2] & 0x3f;
	}
	b64[out] = (in[1] & 0x0f) << 2;
	// Remap sextets to base64 ASCII chars
	for (i = 0; i <= out; i++) b64[i] = pgm_read_byte(&b64chars[(unsigned char)b64[i]]);

	// URL encode base64 signature.  Note, this is not a general URL encoding routine!
	char b64Encoded[100];
	memset(b64Encoded, 0, sizeof(b64Encoded));
	for (i = 0, j = 0; i <= out; i++) {
		uint8_t ch = b64[i];
		if (ch == '+') {
			b64Encoded[j++] = '%';
			b64Encoded[j++] = '2';
			b64Encoded[j++] = 'B';
		}
		else if (ch == '/') {
			b64Encoded[j++] = '%';
			b64Encoded[j++] = '2';
			b64Encoded[j++] = 'F';
		}
		else {
			b64Encoded[j++] = ch;
		}
	}
	b64Encoded[j++] = '%';
	b64Encoded[j++] = '3';
	b64Encoded[j++] = 'D';

	// Make request to SNS API
	int connectionResult = client.connect(AWS_HOST, 80);
	if (connectionResult == 1)
	{
		Serial.println("Connected to AWS...");

		String text;
		text += F("POST /?");
		text += F("AWSAccessKeyId=");
		text += AWS_ACCESS_KEY;
		text += F("&Action=Publish");
		text += F("&Message=");
		text += message;
		text += F("&SignatureMethod=HmacSHA1");
		text += F("&SignatureVersion=2");
		text += F("&Timestamp=");
		text += dateTime;
		text += F("&TopicArn=");
		text += topic;
		text += F("&Version=2010-03-31");
		text += F("&Signature=");
		text += b64Encoded;
		text += F(" HTTP/1.1\r\nHost: ");
		text += AWS_HOST;
		text += F("\r\nConnection: close\r\n");
		text += F("\r\nContent-Length: 0\r\n\r\n");

		Serial.print(text);
		client.print(text);

		// Print response to Serial Monitor
		Serial.println();
		Serial.println("Response:");
		while (client.connected())
		{
			while (client.available())
			{
				char c = client.read();
				Serial.print(c);
			}
		}

		// Close the connection after the server closes its end
		client.stop();
	}
	else
	{
		Serial.println("Error: Failed to connect to AWS! (" + String(connectionResult) + ")");
	}

	Serial.println("Finished publishing message");

	Serial.println();
	Serial.println();
}

// Fill a 24 character buffer with the date in URL-encoded ISO8601 format, like '2013-01-01T01%3A01%3A01Z'.  
// Buffer MUST be at least 24 characters long!
void dateTime8601UrlEncoded(int year, byte month, byte day, byte hour, byte minute, byte seconds, char* buffer)
{
	ultoa(year, buffer, 10);
	buffer[4] = '-';
	btoa2Padded(month, buffer + 5, 10);
	buffer[7] = '-';
	btoa2Padded(day, buffer + 8, 10);
	buffer[10] = 'T';
	btoa2Padded(hour, buffer + 11, 10);
	buffer[13] = '%';
	buffer[14] = '3';
	buffer[15] = 'A';
	btoa2Padded(minute, buffer + 16, 10);
	buffer[18] = '%';
	buffer[19] = '3';
	buffer[20] = 'A';
	btoa2Padded(seconds, buffer + 21, 10);
	buffer[23] = 'Z';
}

// Print a value from 0-99 to a 2 character 0 padded character buffer.
// Buffer MUST be at least 2 characters long!
void btoa2Padded(uint8_t value, char* buffer, int base)
{
	if (value < base) {
		*buffer = '0';
		ultoa(value, buffer + 1, base);
	}
	else {
		ultoa(value, buffer, base);
	}
}

