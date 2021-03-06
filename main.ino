/** Fikses:
 * Fikse print i monitor til riktig vedr. godkjent/ikke godkjent (funker bare first try)
 */

#include <SPI.h>
#include <MFRC522.h>

const int SSPin = 10; // "serial input" for "SPI" (serial peripheral interface) 
const int RSTPin = 9; // reset pin
const int ledGreen = 5;
const int ledRed = 6;
int alarmTriggered = 0;
long previousMillis = 0;
long interval = 1000;
const int piezo = 8;
int ledStateG = LOW;
int ledStateR = LOW;
int teller = 0;
bool gyldig = false;
String tagUID;
String gyldigUID = "AE 78 89 DF";
String aktivertTagUID = "";
MFRC522 rfID(SSPin, RSTPin);

// Sound sensor setup
const int sampleWindow = 50; // Sample window in mS (50 mS = 20Hz)
unsigned int sample;
int tresholdBreaches = 0;
unsigned long lastMillis;
unsigned long breachTime;

// CONFIG ALARM SENSITIVTY
int tresholdVolume = 1.8; // terskel for lydnivaa (0 - 2.55) 
unsigned long maxBreaches = 30; // hvor mange ganger lyden maa overskride
unsigned long breachTimespan = 10000; // over hvor lang tid

void setup() {
  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(piezo, OUTPUT);
  Serial.begin(9600);
  digitalWrite(ledGreen, ledStateG);
  digitalWrite(ledRed, ledStateR);
  SPI.begin();                        // initierer dataoverføring
  rfID.PCD_Init();                    // initierer RFID-instansen
}

void loop() {
  
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis > interval && (teller % 2) == 0) {
    previousMillis = currentMillis; 
    if (ledStateG == LOW)
      ledStateG = HIGH;
    else
      ledStateG = LOW;
 
    digitalWrite(ledGreen, ledStateG);  
  }
  else if (alarmTriggered == 1) {
    digitalWrite(ledGreen, LOW);
    if(currentMillis - previousMillis > interval) {
      previousMillis = currentMillis; 
      if (ledStateR == LOW) {
        ledStateR = HIGH;
        tone(piezo, 1000, 1000);
      }
        
      else
        ledStateR = LOW;
 
    digitalWrite(ledRed, ledStateR);
    }
  }
  monitorDecibel();
  ledStateG = digitalRead(ledGreen);
  ledStateR = digitalRead(ledRed);
  
  if (!rfID.PICC_IsNewCardPresent()) {  // Venter på at nytt kort skannes, disse
    return;                             // Disse if-testene kan ikke puttes utenfor loop
  }
  if (!rfID.PICC_ReadCardSerial()) {    // Hent PICC-id
    return;
  }
  
  hentUID();
  teller += 1;
  aktiver();
  delay(1000);

}

void aktiver_lyd(){                     // Lyd som spilles når systemet slås på
  tone(piezo, (160/5)*(160/5));
  delay(150);
  noTone(piezo);

  tone(piezo, (165/5)*(165/5));
  delay(150);
  noTone(piezo);

  tone(piezo, (175/5)*(175/5));
  delay(200);
  noTone(piezo);
  noTone(piezo);
  tresholdBreaches = 0;

}

void deaktiver_lyd(){                    // Lyd som spilles når systemet slås av
    for (int i = 200; i > 20; i--){
      tone(piezo, (i/5)*(i/5));
      delay(2);
    }
    noTone(piezo);
    tresholdBreaches = 0;
    Serial.println("Resetting breach counter...");
}


void aktiver(){                          // Aktiverer lys utfra av / paa - teller
    unsigned long blinkCounter = millis();
          digitalWrite(ledRed, LOW);

    if ((teller % 2) == 1) {
      alarmTriggered = 0;
      digitalWrite(ledGreen, HIGH);
      Serial.println("Monitoring sound...");
      lastMillis = millis();
      aktiver_lyd();
      //tone(piezo, 500);
      delay(500);
    }
    else if ((teller % 2) == 0) {
      alarmTriggered = 0;
      Serial.println("Standby...");
      deaktiver_lyd();
      
    }
}
  
 void hentUID() {                        // Henter ID til tag, konv. til string, print til monitor 
  Serial.println("Avlest UID: ");
  for (byte i = 0; i < rfID.uid.size; i++) {
    Serial.print(rfID.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(rfID.uid.uidByte[i], HEX);
    tagUID.concat(String(rfID.uid.uidByte[i] < 0x10 ? " 0" : " "));
    tagUID.concat(String(rfID.uid.uidByte[i], HEX));
  }
  
  Serial.println();
  tagUID.toUpperCase();

  if (tagUID.substring(1) == gyldigUID) {
    gyldig = true;
    Serial.println("Tag godkjent");
  } 
  else if (tagUID.substring(1) != gyldigUID) {
    gyldig = false;
    Serial.println("Tag ikke godkjent");
  }
}

void monitorDecibel() {
   unsigned long startMillis= millis();  // Start of sample window
   unsigned int peakToPeak = 0;   // peak-to-peak level

   unsigned int signalMax = 0;
   unsigned int signalMin = 1024;


   // collect data for 50 mS
   while (millis() - startMillis < sampleWindow)
   {
      sample = analogRead(0);
      if (sample < 1024)  // toss out spurious readings
      {
         if (sample > signalMax)
         {
            signalMax = sample;  // save just the max levels
         }
         else if (sample < signalMin)
         {
            signalMin = sample;  // save just the min levels
         }
      }
   }
   peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
   double volts = (peakToPeak * 5.0) / 1024;  // convert to volts
   if(volts > tresholdVolume) {
    tresholdBreaches++;
    Serial.print("dB treshold breached! Number of breaches: ");
    Serial.println(tresholdBreaches);
   }
    if(tresholdBreaches >= maxBreaches) { // trigges for mange ganger
      breachTime = millis()-lastMillis;
      Serial.print("Logged 30 breaches in ");
      Serial.print(breachTime/1000);

     if(breachTime < breachTimespan && (teller % 2) == 1) { // hvis den trigges 30 ganger under 10 sekunder
      alarm();
     }
     else
      lastMillis = millis();
      tresholdBreaches = 0;
      Serial.println("s. Resetting breach counter...");
   }
}

void alarm() {
  Serial.println("s. SWARMING DETECTED");
  alarmTriggered = 1;
  lastMillis = millis();
}
