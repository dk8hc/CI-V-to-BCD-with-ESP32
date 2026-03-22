#include <HardwareSerial.h>

HardwareSerial civ(1);

#define RXD2 16
#define TXD2 17

#define PTT_PIN 4   // TX Detect Eingang

// BCD Pins
#define B0 14
#define B1 27
#define B2 26
#define B3 25

uint8_t currentBand = 0;
uint8_t pendingBand = 0;

void setBCD(uint8_t value)
{
  digitalWrite(B0, value & 0x01);
  digitalWrite(B1, (value >> 1) & 0x01);
  digitalWrite(B2, (value >> 2) & 0x01);
  digitalWrite(B3, (value >> 3) & 0x01);
}

uint8_t bandFromFreq(uint32_t f)
{
  if(f >= 1800000 && f < 2000000) return 1;      // 160m
  if(f >= 3500000 && f < 4000000) return 2;      // 80m
  if(f >= 7000000 && f < 7300000) return 3;      // 40m
  if(f >= 10100000 && f < 10150000) return 4;    // 30m
  if(f >= 14000000 && f < 14350000) return 5;    // 20m
  if(f >= 18068000 && f < 18168000) return 6;    // 17m
  if(f >= 21000000 && f < 21450000) return 7;    // 15m
  if(f >= 24890000 && f < 24990000) return 8;    // 12m
  if(f >= 28000000 && f < 29700000) return 9;    // 10m
  if(f >= 50000000 && f < 55000000) return 10;   // 6m
  if(f >= 144000000 && f < 148000000) return 11; // 2m
  if(f >= 430000000 && f < 440000000) return 12; // 70cm
  if(f >= 5000000 && f < 6000000) return 15;     // 6m
  if(f >= 70000000 && f < 72000000) return 15;   // 4m

  return 0;
}

void setup()
{
  pinMode(B0, OUTPUT);
  pinMode(B1, OUTPUT);
  pinMode(B2, OUTPUT);
  pinMode(B3, OUTPUT);

  pinMode(PTT_PIN, INPUT_PULLUP);

  setBCD(0);

  civ.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

void loop()
{
  static uint8_t buf[32];
  static uint8_t idx = 0;

  // -------- CI-V Empfang --------
  while(civ.available())
  {
    buf[idx++] = civ.read();

    if(idx >= sizeof(buf)) idx = 0;

    // Telegramm Ende erkannt
    if(buf[idx-1] == 0xFD)
    {
      if(idx >= 11)
      {
        uint8_t cmd = buf[4];

        // Nur Frequenzdaten auswerten
        if(cmd == 0x00)
        {
          uint32_t freq = 0;
          uint32_t mult = 1;

          for(int i=0;i<5;i++)
          {
            uint8_t b = buf[5+i];

            freq += (b & 0x0F) * mult;
            mult *= 10;

            freq += (b >> 4) * mult;
            mult *= 10;
          }

          pendingBand = bandFromFreq(freq);

          // Debug
          Serial.print("Freq: ");
          Serial.print(freq);
          Serial.print(" Hz  Band: ");
          Serial.println(pendingBand);
        }
      }

      idx = 0;
    }
  }

  // ---- Bandumschaltung nur im RX ----
  bool txActive = (digitalRead(PTT_PIN) == LOW);

  if(!txActive && pendingBand != currentBand)
  {
    delay(50);              // Relais Beruhigungszeit
    setBCD(pendingBand);
    currentBand = pendingBand;
  }
}