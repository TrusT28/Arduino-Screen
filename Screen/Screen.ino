
#include "funshield.h"
#ifndef slowDigitalWrite
  #define slowDigitalWrite(a,b) digitalWrite(a,b)
  #define constDigitalWrite(a,b) digitalWrite(a,b)
#endif

// ***SETUP BUTTONS*** \\

const int t_debounce=10000; //[us]
const long t_slow = 1000000;   /* this amounts to 1 sec */
const long t_fast = 300000; /* 300ms */

enum ButtonState {UP,DEBOUNCING,DOWN};

struct Button {
  long deadline;
  ButtonState state;
  unsigned char pin;
};

void button_init(Button& b, int which) {
  b.pin=which;
  b.state=UP;
  pinMode(which,INPUT);
}

long duration(long now, long then) {return ((unsigned long)now)-then;}

int get_pulse(Button& b){
  if(digitalRead(b.pin)==OFF){
    b.state = UP;
    return 0;  
  }
  long now=micros();    /* the button has been pressed, which brought us here */
  switch(b.state){
    case UP:
      b.deadline=now+t_debounce;
      b.state=DEBOUNCING; 
      return 0;
    case DEBOUNCING:
      if(duration(now,b.deadline)<0) return 0;
      b.state=DOWN;
      b.deadline+=t_slow;
      return 1;
    case DOWN:
      if(duration(now,b.deadline)<0) return 0;
      b.deadline+=t_fast;
      return 1;
  }
  return 0; //This is never reached. Just to fix warning: control reaches end of non-void function [-Wreturn-type] bug
}  

// *** SETUP SCREEN *** \\

inline void shift_out8(unsigned char data)
{
  for(signed char i=7; i>=0; i--,data>>=1){
    digitalWrite(data_pin,data&1);  /* set up the data bit */
    constDigitalWrite(clock_pin,HIGH);  /* upon clock rising edge, the 74HC595 will shift the data bit in */
    constDigitalWrite(clock_pin,LOW);
  }
}

void shift_out(unsigned int bit_pattern)
{
  shift_out8(bit_pattern);
  shift_out8(bit_pattern>>8);
  constDigitalWrite(latch_pin,HIGH);
  constDigitalWrite(latch_pin,LOW);

}

void disp_init()
{
  pinMode(latch_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  digitalWrite(clock_pin, HIGH);
  shift_out(0);
}

void disp_7seg(unsigned char column, unsigned char glowing_leds_bitmask)
{
  shift_out((1<<(8+4+column)) | (0xFF^glowing_leds_bitmask));
}


// *** SETUP HELPERS *** \\

// font is array of numbers for screen. 0th element is zero, 1st element is one and so on until nine
const unsigned char font[]={0b11111100, 0b01100000, 0b11011010, 0b11110010,0b01100110,0b10110110,0b10111110,0b11100000,0b11111110,0b11110110};

int state=0;
int switch_state=0;
bool updated = false;
int column=0;
int number=0;
int arr[4]; 

Button button_plus, button_minus, button_switch;

// *** Start Code *** \\

void increment() {
  for (int i=column; i<4; i++) {
    arr[i]++;
    if(arr[i] == 10) {
      arr[i]=0;
    }
    else break;
  }
}

void decrement() {
  for (int i=column; i<4; i++) {
    arr[i]--;
    if(arr[i]<0) {
      arr[i]=9;
    }
    else break;
  }
}

void switch_button() {
  column++;
  if(column==4){
    column=0;
  }
}

void setup()
{
  Serial.begin(9600);
  while(!Serial);
  
  disp_init();
  pinMode(button1_pin, INPUT);
  pinMode(button2_pin, INPUT);
  pinMode(button3_pin, INPUT);
  button_init(button_plus, button1_pin);
  button_init(button_minus, button2_pin);
  button_init(button_switch, button3_pin);
}

void loop() 
{
  if(updated) {
    if(state>0)
      increment();
    else if(state<0)
      decrement();
    if(switch_state>0)
      switch_button();
    state=0;
    switch_state=0;
    updated=false;
  }
  
  int newState = state;
  newState+=get_pulse(button_plus);
  newState-=get_pulse(button_minus);
  switch_state+=get_pulse(button_switch);
  if(newState!=state || switch_state>0){
    updated=true;
    state = newState;
  }

  disp_7seg(column, font[arr[column]]);
}
