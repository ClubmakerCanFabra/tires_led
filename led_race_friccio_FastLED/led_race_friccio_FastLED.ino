/*
 * ____                     _      ______ _____    _____
  / __ \                   | |    |  ____|  __ \  |  __ \               
 | |  | |_ __   ___ _ __   | |    | |__  | |  | | | |__) |__ _  ___ ___ 
 | |  | | '_ \ / _ \ '_ \  | |    |  __| | |  | | |  _  // _` |/ __/ _ \
 | |__| | |_) |  __/ | | | | |____| |____| |__| | | | \ \ (_| | (_|  __/
  \____/| .__/ \___|_| |_| |______|______|_____/  |_|  \_\__,_|\___\___|
        | |                                                             
        |_|          
 Open LED Race
 An minimalist cars race for LED strip

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.


 by gbarbarov@singulardevices.com  for Arduino day Seville 2019
 https://www.hackster.io/gbarbarov/open-led-race-a0331a
 https://twitter.com/openledrace


 https://gitlab.com/open-led-race
 https://openledrace.net/open-software/
*/
// Led_Race_Fricció updated, simplified & commented by Lluís Plana

#include <FastLED.h>
#define MAXLEDS   90   // MAX LEDs actives on strip

#define PIN_LED    2   // R 500 ohms to data pin, CAP 1000 uF to VCC_5v/GND, Power Supply 5V 2A
#define PIN_P1     8   // switch player 1 to PIN and GND
#define PIN_P2     9   // switch player 2 to PIN and GND
#define PIN_AUDIO  3   // through Capacitor (1 to 10uf) to speaker 8 ohms

// definició de la rampa
#define INI_RAMP   50
#define MED_RAMP   60
#define END_RAMP   70
#define HIGH_RAMP  6

CRGB leds[MAXLEDS];
byte gravity_map[MAXLEDS];

float ACEL=0.1;   // acceleration value (0.1 for single strip density & 0.2 for double)
float kf=0.015;   // friction constant
float kg=0.003;   // gravity constant

#define COLOR1   CRGB(0,255,0)
#define COLOR2   CRGB(0,0,255)

byte flag_sw1=0;
byte flag_sw2=0;

float speed1=0;
float speed2=0;
float dist1=0;
float dist2=0;
byte loop1=0;
byte loop2=0;

byte loop_max=5;   // total race laps
byte leader=0;
byte draworder=0;
#define TDELAY 5

int TBEEP=0;
int FBEEP=0;

void set_ramp(byte H, byte a, byte b, byte c) {
   for(int i=0; i<(b-a); i++) { gravity_map[a+i]=127-i*((float)H/(b-a)); }
   gravity_map[b]=127;
   for(int i=0; i<(c-b); i++) { gravity_map[b+i+1]=127+H-i*((float)H/(c-b)); }
}

void set_loop(byte H, byte a, byte b, byte c) {
   for(int i=0; i<(b-a); i++) { gravity_map[a+i]=127-i*((float)H/(b-a)); }
   gravity_map[b]=255;
   for(int i=0; i<(c-b); i++) { gravity_map[b+i+1]=127+H-i*((float)H/(c-b)); }
}

void setup() {
   FastLED.addLeds<NEOPIXEL, PIN_LED> (leds, MAXLEDS);
   FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);
   FastLED.setBrightness(50);
   FastLED.clear(true);
   pinMode(PIN_P1, INPUT_PULLUP);
   pinMode(PIN_P2, INPUT_PULLUP);

   // defineix la cinètica a cada posició (127 pla, <127 pujada, >127 baixada)
   for(int i=0; i<MAXLEDS; i++) { gravity_map[i]=127; }
   set_ramp(HIGH_RAMP, INI_RAMP, MED_RAMP, END_RAMP);

   start_race();
}

void start_race() {
   // mostra semàfor i fa sonar l'inici de cursa
   FastLED.clear(true);
   delay(2000);
   leds[9]=CRGB(255,0,0);
   leds[8]=CRGB(255,0,0);
   FastLED.show();
   tone(PIN_AUDIO,294);  delay(2000);  noTone(PIN_AUDIO);   // Preparats: Re 4a Octava
   leds[9]=CRGB(0,0,0);
   leds[8]=CRGB(0,0,0);
   leds[7]=CRGB(255,255,0);
   leds[6]=CRGB(255,255,0);
   FastLED.show();
   tone(PIN_AUDIO,587);  delay(2000);  noTone(PIN_AUDIO);   // Llestos: Re 5a Octava
   leds[7]=CRGB(0,0,0);
   leds[6]=CRGB(0,0,0);
   leds[5]=CRGB(0,255,0);
   leds[4]=CRGB(0,255,0);
   FastLED.show();
   tone(PIN_AUDIO,1174);  delay(2000);  noTone(PIN_AUDIO);   // Ja!!!: Re 6a Octava
   
   speed1=0;  speed2=0;
   dist1=0;  dist2=0;
   loop1=0;  loop2=0;
}

void winner_fx(byte w) {
   // https://gist.github.com/gskielian/6135641#file-mario-ino-L20
   int win_intro[] = {659, 659, 0, 659, 0, 523, 659, 0, 784, 0, 0, 0, 392, 0, 0, 0};
   int win_music[] = {523, 0, 0, 392, 0, 0, 330, 0, 0, 440, 0, 494, 466, 440, 0,
                      392, 659, 784, 880, 0, 698, 784, 0, 659, 0, 523, 587, 494, 0, 0};

   FastLED.clear();
   if (w==1) for(int i=0; i<MAXLEDS/4; i++) { leds[i]=COLOR1; }
   if (w==2) for(int i=0; i<MAXLEDS/4; i++) { leds[i]=COLOR2; }
   FastLED.show();
   int msize = sizeof(win_intro) / sizeof(int);
   for(int note=0; note<msize; note++) {
      tone(PIN_AUDIO, win_intro[note], 150);
      delay(180);
   }
   msize = sizeof(win_music) / sizeof(int);
   while(true) {
      for(int note=0; note<msize; note++) {
         tone(PIN_AUDIO, win_music[note], 150);
         delay(180);
      }
   }
}

void draw_car1(void) {
   int index;
   for(int i=0; i<=loop1; i++) {
      index=((word)dist1 % MAXLEDS)+i;
      if (index>=MAXLEDS) index-=MAXLEDS;
      leds[index]=COLOR1;
   }
}

void draw_car2(void) {
   int index;
   for(int i=0; i<=loop2; i++) {
      index=((word)dist2 % MAXLEDS)+i;
      if (index>=MAXLEDS) index-=MAXLEDS;
      leds[index]=COLOR2;
   }
}

void loop() {
   // esborra la tira led
   FastLED.clear();

   // mostra la rampa
   int value;
   for(int i=0; i<(MED_RAMP-INI_RAMP); i++) { value=(i+1)*255/(MED_RAMP-INI_RAMP);  leds[INI_RAMP+i]=CRGB(value,0,value); }
   for(int i=0; i<(END_RAMP-MED_RAMP); i++) { value=(i+1)*255/(END_RAMP-MED_RAMP);  leds[END_RAMP-i]=CRGB(value,0,value); }

   // si Player1 ha premut incrementa la velocitat
   if ((flag_sw1==1) && (digitalRead(PIN_P1)==0)) { flag_sw1=0;  speed1+=ACEL; }
   if ((flag_sw1==0) && (digitalRead(PIN_P1)==1)) flag_sw1=1;

   // aplica la constant de la gravetat per frenar/accelerar a la pujada/baixada de la rampa
   if ((gravity_map[(word)dist1 % MAXLEDS])<127) speed1-=kg*(127-(gravity_map[(word)dist1 % MAXLEDS]));
   if ((gravity_map[(word)dist1 % MAXLEDS])>127) speed1+=kg*((gravity_map[(word)dist1 % MAXLEDS])-127);

   // aplica la constant de la fricció per frenar-lo
   speed1-=speed1*kf;

   // idem per Player2
   if ((flag_sw2==1) && (digitalRead(PIN_P2)==0)) { flag_sw2=0;  speed2+=ACEL; }
   if ((flag_sw2==0) && (digitalRead(PIN_P2)==1)) flag_sw2=1;

   if ((gravity_map[(word)dist2 % MAXLEDS])<127) speed2-=kg*(127-(gravity_map[(word)dist2 % MAXLEDS]));
   if ((gravity_map[(word)dist2 % MAXLEDS])>127) speed2+=kg*((gravity_map[(word)dist2 % MAXLEDS])-127);

   speed2-=speed2*kf;

   // actualitza la distancia recorreguda
   dist1+=speed1;
   dist2+=speed2;

   // comprova si hi ha canvi de líder i prepara per emetre un so addicional
   if (dist1>dist2) {
      if (leader==2) { FBEEP=440;  TBEEP=10; }
      leader=1;
   }
   if (dist2>dist1) {
      if (leader==1) { FBEEP=440*2;  TBEEP=10; }
      leader=2;
   }

   // al passar per l'inici incrementa el comptador de voltes i també emet so
   if (dist1>MAXLEDS*loop1) { loop1++;  FBEEP=440;  TBEEP=10; }
   if (dist2>MAXLEDS*loop2) { loop2++;  FBEEP=440*2;  TBEEP=10; }

   // si Player1 o Player2 han fet les voltes ja tenim guanyador
   if (loop1>loop_max) winner_fx(1);
   if (loop2>loop_max) winner_fx(2);

   // alterna l'ordre de visualització dels cotxes (per si comparteixen caselles) i els pinta
   if ((millis() & 512)==(512*draworder)) {
      if (draworder==0) draworder=1;
      else draworder=0;
   }
   if (abs(round(speed1*100))>abs(round(speed2*100))) draworder=1;
   if (abs(round(speed2*100))>abs(round(speed1*100))) draworder=0;

   if ( draworder==0 ) {
      draw_car1();
      draw_car2();
   }
   else {
      draw_car2();
      draw_car1();
   }
   FastLED.show();

   // so del motor dels cotxes en funció de la velocitat i els sons addicionals
   tone(PIN_AUDIO,FBEEP+int(speed1*440*2)+int(speed2*440*3));
   if (TBEEP>0) TBEEP--;
   else FBEEP=0;

   delay(TDELAY);
}
