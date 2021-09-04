
#include <Wire.h>
#include <SPI.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <DS3231.h>
#include <Bounce2.h>

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10

MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

#define BOUNCE_PIN 3
// INSTANTIATE A Bounce OBJECT
Bounce bounce = Bounce();

DS3231 Clock;

const byte interruptPin = 2;
volatile bool update = false;

void updateISR() {
  update = true;
}

void setup(void)
{
  P.begin();
  P.setIntensity(7);
  P.displayClear();
  P.setTextAlignment(PA_CENTER);

  int battery = analogRead(0);
  char str[16];
  float volts = ((float)battery / 1024.0) * 5.0 * 2;
  dtostrf(volts, 2, 2, str);
  strcat(str, "V");
  P.print(str);
  delay(2000);

  bounce.attach( BOUNCE_PIN, INPUT_PULLUP );
  bounce.interval(5); // interval in ms

  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), updateISR, FALLING);

  Wire.begin();
  Clock.enableOscillator(true, false, 0);

  P.print("Ready");
}

void display(unsigned short h, unsigned short m, unsigned short s) {
  char str[16];
  if ((h == 0) && (m == 0)) {
    snprintf(str, 16, "%i sec", s);
  } else if (h == 0) {
    if (s == 0) {
      if (m == 1) {
        snprintf(str, 16, "%i min", m);
      } else {
        snprintf(str, 16, "%i mins", m);
      }
    } else {
      if (m < 10) {
        snprintf(str, 16, "%im %is", m, s);
      } else {
        snprintf(str, 16, "%im %i", m, s);
      }
    }
  } else {
    if ((m == 0) && (h < 100)) {
      if (h == 1) {
        snprintf(str, 16, "%i hour", h);
      } else {
        snprintf(str, 16, "%i hrs", h);
      }
    } else {
      if (h < 10) {
        snprintf(str, 16, "%ih %im", h, m);
      } else if (h < 100) {
        snprintf(str, 16, "%ih %i", h, m);
      } else {
        snprintf(str, 16, "%ih", h);
      }
    }
  }
  P.print(str);
}

typedef enum press_t {
  noPress, shortPress, longPress
} press_t;

typedef enum state_t {
  ready, running, stopped
} state_t;
state_t clockState = ready;

uint16_t l = 0;

void loop() {
  press_t p = noPress;
  bounce.update();

  // <Bounce>.changed() RETURNS true IF THE STATE CHANGED (FROM HIGH TO LOW OR LOW TO HIGH)
  if ( bounce.changed() ) {
    // THE STATE OF THE INPUT CHANGED
    // GET THE STATE
    int deboucedInput = bounce.read();
    // IF THE CHANGED VALUE IS LOW
    if ( deboucedInput == LOW ) {
      l = -1;
    } else {
      if (l) {
        p = shortPress;
        l = 0;
      }
    }
  }

  if (l) {
    l--;
    if (l == 0) {
      p = longPress;
    }
  }

  if (p == shortPress) {
    switch (clockState) {
      case ready:
        clockState = running;
        Clock.setHour(0);
        Clock.setMinute(0);
        Clock.setSecond(0);
        break;
      case running:
        clockState = stopped;
        break;
      case stopped:
        break;
    }
  }
  if (p == longPress) {
    switch (clockState) {
      case ready:
      case running:
      case stopped:
        clockState = ready;
        P.print("Ready");
        break;
    }
  }

  if ((clockState == running) && update) {
    update = false;
    DateTime now = RTClib::now();
    display(now.hour(), now.minute(), now.second());
  }
}
