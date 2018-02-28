#include "sha1.h"
#include <Ethernet.h>
#include <HttpClient.h>
#include <RTClib.h>

// ===============
// Amazon AWS configuration
#define     AWS_ACCESS_KEY         "your_AWS_access_key"                               // Put your AWS access key here.
#define     AWS_SECRET_ACCESS_KEY  "your_AWS_secret_access_key"                        // Put your AWS secret access key here.
#define     AWS_REGION             "us-east-1"                                         // The region where your SNS topic lives.
                                                                                       // See the table at: http://docs.aws.amazon.com/general/latest/gr/rande.html#sns_region
#define     AWS_HOST               "sns.us-east-1.amazonaws.com"                       // The host URL for the region where your SNS topic lives.
                                                                                       // See the table at: http://docs.aws.amazon.com/general/latest/gr/rande.html#sns_region
#define     SNS_TOPIC_ARN          "url_encoded_SNS_topic_ARN"                         // Amazon resource name (ARN) for the SNS topic to receive notifications.
                                                                                       // Note: This ARN _MUST_ be URL encoded!  See http://meyerweb.com/eric/tools/dencoder/ for an example URL encoder tool.

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
	snsPublish(SNS_TOPIC_ARN, "Bowl%20is%20LOW");
	delay(5000);	
}

// Publish a message to an SNS topic.
// Note, both the topic and message strings _MUST_ be URL encoded before calling this function!
void snsPublish(const char* topic, const char* message)
{
	unsigned long currentTime = 0; // TODO:
	
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
  Sha1.print(AWS_HOST); Sha1.print(F("\n"));
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
  for(in = Sha1.resultHmac(), out=0; ; in += 3) { // octets to sextets
    b64[out++] =   in[0] >> 2;
    b64[out++] = ((in[0] & 0x03) << 4) | (in[1] >> 4);
    if(out >= 26) break;
    b64[out++] = ((in[1] & 0x0f) << 2) | (in[2] >> 6);
    b64[out++] =   in[2] & 0x3f;
  }
  b64[out] = (in[1] & 0x0f) << 2;
  // Remap sextets to base64 ASCII chars
  for(i=0; i<=out; i++) b64[i] = pgm_read_byte(&b64chars[(unsigned char)b64[i]]);
  
  // URL encode base64 signature.  Note, this is not a general URL encoding routine!
  char b64Encoded[100];
  memset(b64Encoded, 0, sizeof(b64Encoded));
  for(i=0, j=0; i<=out; i++) {
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
  
//  // Make request to SNS API.
//  uint32_t ip = 0;
//  while (ip == 0)
//  {
//    if (!cc3000.getHostByName(AWS_HOST, &ip)) {
//      Serial.println(F("Couldn't resolve!"));
//    }
//    delay(500);
//  }
//
//  
//  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
//  if (www.connected()) {    
//    www.fastrprint(F("POST /?"));
//    www.fastrprint(F("AWSAccessKeyId="));
//    www.fastrprint(AWS_ACCESS_KEY);
//    www.fastrprint(F("&Action=Publish"));
//    www.fastrprint(F("&Message="));
//    www.fastrprint(message);
//    www.fastrprint(F("&SignatureMethod=HmacSHA1"));
//    www.fastrprint(F("&SignatureVersion=2"));
//    www.fastrprint(F("&Timestamp="));
//    www.fastrprint(dateTime);
//    www.fastrprint(F("&TopicArn="));
//    www.fastrprint(topic);
//    www.fastrprint(F("&Version=2010-03-31"));
//    www.fastrprint(F("&Signature="));
//    www.fastrprint(b64Encoded);  
//    www.fastrprint(F(" HTTP/1.1\r\nHost: "));
//    www.fastrprint(AWS_HOST);
//    www.fastrprint(F("\r\nContent-Length: 0\r\n\r\n"));
//  } 
//  else {
//    Serial.println(F("Connection failed"));    
//    www.close();
//    return;
//  }
//  
//  // Read data until either the connection is closed, or the idle timeout is reached.
//  Serial.println(F("AWS response:"));
//  unsigned long lastRead = millis();
//  while (www.connected() && (millis() - lastRead < TIMEOUT_MS)) {
//    while (www.available()) {
//      char c = www.read();
//      Serial.print(c);
//      lastRead = millis();
//    }
//  }
//  www.close();
}

// Fill a 24 character buffer with the date in URL-encoded ISO8601 format, like '2013-01-01T01%3A01%3A01Z'.  
// Buffer MUST be at least 24 characters long!
void dateTime8601UrlEncoded(int year, byte month, byte day, byte hour, byte minute, byte seconds, char* buffer)
{
  ultoa(year, buffer, 10);
  buffer[4] = '-';
  btoa2Padded(month, buffer+5, 10);
  buffer[7] = '-';
  btoa2Padded(day, buffer+8, 10);
  buffer[10] = 'T';
  btoa2Padded(hour, buffer+11, 10);
  buffer[13] = '%';
  buffer[14] = '3';
  buffer[15] = 'A';
  btoa2Padded(minute, buffer+16, 10);
  buffer[18] = '%';
  buffer[19] = '3';
  buffer[20] = 'A';
  btoa2Padded(seconds, buffer+21, 10);
  buffer[23] = 'Z';
}

// Print a value from 0-99 to a 2 character 0 padded character buffer.
// Buffer MUST be at least 2 characters long!
void btoa2Padded(uint8_t value, char* buffer, int base)
{
  if (value < base) {
    *buffer = '0';
    ultoa(value, buffer+1, base);
  }
  else {
    ultoa(value, buffer, base); 
  }
}

