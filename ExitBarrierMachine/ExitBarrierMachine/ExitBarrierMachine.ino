/*RTC Header*/
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

/*USB Shield Header*/
#include <hiduniversal.h>
#include <usbhub.h>
#include <avr/pgmspace.h>
#include <Usb.h>
#include <usbhub.h>
#include <avr/pgmspace.h>
#include <hidboot.h>
#define DISPLAY_WIDTH 16

/*Ethernet Header*/
#include <SPI.h>
#include <HttpClient.h>
#include <Ethernet.h>
#include <EthernetClient.h>

/*LCD Display Header*/
#include <LiquidCrystal.h>

/*LCD Display Variable*/
LiquidCrystal lcd(22, 23, 24, 25, 26, 27);

/*Ethernet Variable*/
EthernetClient client;
HttpClient http(client);
int totalCount = 0;
String connectionStatus = "";
#define delayMillis 3000UL
unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

/*ExitBarrierMachine Variable*/
const int inputTicketpin = 2;
const int motorPin1 = 3;
const int motorPin2 = 4;
const int motorPWM = 5;
const int barrierTrigger = 6;
const int carPassByInside = 7;
const int barrierStatus = 9;

String SerialNo;
String serverRet;
String errorStatus;
int testState = 0;
int ticketState = 0;
int barcodeScaned = 0;
int ServerResult = 1;
int carPassByStatusInside = 0;

/*RTC Variable*/
int perviousState = HIGH;
int currentState = LOW;
String ErrorMsg;
String TimeDate;
const char *monthName[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
tmElements_t tm;

/*USB Variable*/
USB     Usb;
USBHub     Hub(&Usb);
HIDUniversal      Hid(&Usb);

/*USB Function*/
class KbdRptParser : public KeyboardReportParser
{
    void PrintKey(uint8_t mod, uint8_t key);
  protected:
    virtual void OnKeyDown  (uint8_t mod, uint8_t key);
    virtual void OnKeyPressed(uint8_t key);
};

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
  uint8_t c = OemToAscii(mod, key);

  if (c)
    OnKeyPressed(c);
}

/* what to do when symbol arrives */
void KbdRptParser::OnKeyPressed(uint8_t key)
{
  static uint32_t next_time = 0;
  static uint8_t current_cursor = 0;

  if ( millis() > next_time ) {
    current_cursor = 0;
    delay( 5 );
  }

  next_time = millis() + 200;  //reset watchdog

  if ( current_cursor++ == ( DISPLAY_WIDTH + 1 ))
  {
  }

  SerialNo += (char)key;
};

KbdRptParser Prs;

void setup() {
  lcd.begin(20, 4);
  lcd.setCursor(0, 0);
  lcd.print("IPS Equipment");

  byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  IPAddress ip(192, 168, 0, 177);
  IPAddress gateway(192, 168, 0, 1);
  IPAddress subnet(255, 255, 255, 0);

  pinMode(inputTicketpin, INPUT);
  // pinMode(carPassByInside, INPUT);
  //pinMode(barrierStatus, INPUT);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPWM, OUTPUT);
  pinMode(barrierTrigger, OUTPUT);
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);
  digitalWrite(carPassByInside, HIGH);
  digitalWrite(barrierStatus, HIGH);
  digitalWrite(barrierTrigger, HIGH);
  Serial.begin( 115200 );
  Serial.println("Start");

  /*USB initialize*/
  if (Usb.Init() == -1) {
    Serial.println("OSC did not start.");
  }
  delay( 200 );
  Hid.SetReportParser(0, (HIDReportParser*)&Prs);
  delay( 200 );

  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  Serial.println(F("Starting ethernet..."));
  Ethernet.begin(mac, ip, gateway, gateway, subnet);
  Serial.println(Ethernet.localIP());
  delay(500);
  Serial.println(F("Ready"));
}

void loop() {
  // put your main code here, to run repeatedly:
  dateTimeUpdate();
  lcd.setCursor(0, 2);
  lcd.print("Welcome           ");
  unsigned int TimeOut = 0;
  unsigned int currentTime = getTimeNow();
  carPassByStatusInside = digitalRead(carPassByInside);
  if (carPassByStatusInside == 0)
  {
    lcd.setCursor(0, 3);
    lcd.print("                    ");
  }
  if ((carPassByStatusInside == 1)/* && (digitalRead(barrierStatus) == 0)*/)
  {
    dateTimeUpdate();
    lcd.setCursor(0, 3);
    lcd.print("Please Insert Ticket");
    while ((digitalRead(inputTicketpin) == LOW) && (barcodeScaned == 0))
    {
      dateTimeUpdate();
      lcd.setCursor(0, 3);
      lcd.print("Please Wait         ");
      unsigned int compareTime = getTimeNow_ToCompare();
      while ((barcodeScaned == 0) && (ticketState == 0))
      {
        dateTimeUpdate();
        analogWrite(motorPWM, 255);
        digitalWrite(motorPin1, HIGH);
        digitalWrite(motorPin2, LOW);
        while ((digitalRead(inputTicketpin) == HIGH) && (barcodeScaned == 0) && (ticketState == 0)) {
          dateTimeUpdate();
          digitalWrite(motorPin1, LOW);
          digitalWrite(motorPin2, LOW);
          Usb.Task();
          if (SerialNo.length() >= 14)
          {
            Serial.println(SerialNo);
            lcd.setCursor(0, 3);
            lcd.print(SerialNo + "      ");
            barcodeScaned = 1;
            TimeOut = (((SerialNo.substring(8, 10)).toInt()) * 3600) + (((SerialNo.substring(10, 12)).toInt()) * 60);
            if (currentTime - TimeOut  < 960) {
              Serial.println("its Free");
              digitalWrite(barrierTrigger, LOW);
              ServerResult = 1;
            }
            else {
              //Server Side
              Serial.println("ask Server");
              thisMillis = millis();

              if (thisMillis - lastMillis > delayMillis)
              {
                lastMillis = thisMillis;

                //sprintf(pageAdd, "/postime/Test1.php?", totalCount);

                if (!getPage())
                {
                  connectionStatus = "F";
                  Serial.print(F("Fail "));
                }
                else
                {
                  connectionStatus = "C";
                  Serial.print(F("Pass "));
                  ServerResult = 0;
                  if (serverRet.indexOf("OK") >= 0)
                  {
                    //barrier up;
                    ServerResult = 1;
                    digitalWrite(barrierTrigger, LOW);
                  }
                  else if (serverRet.indexOf("Anti-Passback") >= 0)
                  {
                    Serial.println("Anti-Passback");
                  }
                  else if (serverRet.indexOf("Expired") >= 0)
                  {
                    Serial.println("Expired");
                  }
                  else if (serverRet.indexOf("Invalid Ticket") >= 0)
                  {
                    Serial.println("Invalid Ticket");
                  }
                }
                Serial.println(serverRet.length());
                int strlength = serverRet.length();
                int i = 0;
                for (i = 0; i < 20 - strlength; i++)
                {
                  serverRet += " ";
                }
                lcd.setCursor(0, 3);
                lcd.print(serverRet);
                Serial.println(serverRet);
                serverRet = "";
                totalCount++;
                Serial.println(totalCount, DEC);
              }
            }

            ///////TODO///////migrate with the machine/////////
            if (ServerResult) {
              analogWrite(motorPWM, 255);
              digitalWrite(motorPin1, HIGH);
              digitalWrite(motorPin2, LOW);
              delay(500);
              barcodeScaned = 0;
              testState = 1;
            }
            else
            {
              analogWrite(motorPWM, 255);
              digitalWrite(motorPin1, LOW);
              digitalWrite(motorPin2, HIGH);
              delay(3000);
              barcodeScaned = 0;
              testState = 1;
            }
            SerialNo = "";
            ticketState = 1;
            digitalWrite(barrierTrigger, HIGH);
            delay(1000);
          }
        }
      }
    }

    digitalWrite(motorPin1, LOW);
    digitalWrite(motorPin2, LOW);

    if (testState == 1) {
      testState = 0;
      ticketState = 0;
    }
    else if (digitalRead(inputTicketpin) == HIGH) {
      ticketState = 0;
    }
  }
}

/*RTC Function*/
int getTimeNow()
{
  tmElements_t tm;
  unsigned int currentTime = 0;

  if (RTC.read(tm))
  {
    //TODO (Change TO LCD Display)
    currentTime = (tm.Hour * 60 * 60) + (tm.Minute * 60) + (tm.Second);
    return currentTime;
  }
  else
  {
    if (RTC.chipPresent())
    {
      //TODO (Change TO LCD Display)
      ErrorMsg += "The DS1307 is stopped.  Please run the SetTime";
    }
    else
    {
      //TODO (Change TO LCD Display)
      ErrorMsg += "DS1307 read error!  Please check the circuitry.";
    }
  }
}

int getTimeNow_ToCompare()
{
  tmElements_t tm;
  unsigned int compareTime = 0;
  if (RTC.read(tm))
  {
    //TODO (Change TO LCD Display)
    compareTime = (tm.Hour) + (tm.Minute) + (tm.Second);
    return compareTime;
  }
  else
  {
    if (RTC.chipPresent())
    {
      //TODO (Change TO LCD Display)
      ErrorMsg += "The DS1307 is stopped.  Please run the SetTime";
    }
    else
    {
      //TODO (Change TO LCD Display)
      ErrorMsg += "DS1307 read error!  Please check the circuitry.";
    }
  }
}

/*Ethernet Function*/
byte getPage()
{
  IPAddress server(192, 168, 0, 68);
  const char serverName[] = "192.168.0.68";
  int serverPort = 90;
  int err = 0;
  String path = "/postime/Test1.php?TicketNo=" + SerialNo.substring(0, 14);
  char charArr[100];
  path.toCharArray(charArr, path.length());
  err = http.get(serverName, serverPort, charArr);
  if (err == 0)
  {
    //Serial.println("startedRequest ok");

    err = http.responseStatusCode();
    if (err >= 0)
    {
      //Serial.print("Got status code: ");
      //Serial.println(err);

      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get

      err = http.skipResponseHeaders();
      if (err >= 0)
      {
        int bodyLen = http.contentLength();

        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ( (http.connected() || http.available()) &&
                ((millis() - timeoutStart) < delayMillis) )
        {
          if (http.available())
          {
            c = http.read();
            serverRet += (char)c;
            // Print out this character

            bodyLen--;
            // We read something, reset the timeout counter
            timeoutStart = millis();
          }
          else
          {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(1000);
          }
        }
        lcd.setCursor(18, 2);
        lcd.print("OK");
        Serial.println(serverRet);
      }
      else
      {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
        lcd.setCursor(18, 2);
        lcd.print("DC");
        return 0;
      }
    }
    else
    {
      Serial.print("Getting response failed: ");
      Serial.println(err);
      lcd.setCursor(18, 2);
      lcd.print("DC");
      return 0;
    }
  }
  else
  {
    Serial.print("Connect failed: ");
    Serial.println(err);
    lcd.setCursor(18, 2);
    lcd.print("DC");
    return 0;
  }

  http.stop();
  return 1;
}

void dateTimeUpdate()
{
  tmElements_t tm;
  lcd.setCursor(0, 1);

  if (RTC.read(tm))
  {
    //TODO (Change TO LCD Display)
    TimeDate = "";
    print2digits(tm.Hour);
    TimeDate += ":";
    print2digits(tm.Minute);
    lcd.print(TimeDate);
    lcd.print(" ");
    lcd.print(tm.Day);
    lcd.print("/");
    lcd.print(tm.Month);
    lcd.print("/");
    lcd.print(tmYearToCalendar(tm.Year));
  }
  else
  {
    if (RTC.chipPresent())
    {
      //TODO (Change TO LCD Display)
      ErrorMsg += "The DS1307 is stopped.  Please run the SetTime";
      getTimeConfig();
    }
    else
    {
      //TODO (Change TO LCD Display)
      ErrorMsg += "DS1307 read error!  Please check the circuitry.";
    }
  }
}

void print2digits(int number)
{
  if (number >= 0 && number < 10)
  {
    //TODO (Change TO LCD Display)
    TimeDate += '0';
  }
  //TODO (Change TO LCD Display)
  TimeDate += String(number);
}

bool getTime(const char *str)
{
  int Hour, Min, Sec;
  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;
  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++)
  {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void getTimeConfig()
{
  bool parse = false;
  bool config = false;

  // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }
}
