
#include "funshield.h"
#ifndef slowDigitalWrite
  #define slowDigitalWrite(a,b) digitalWrite(a,b)
  #define constDigitalWrite(a,b) digitalWrite(a,b)
#endif

// ***SETUP BUTTONS*** \\

const int t_debounce=10000; //[us]
const long period = 1000000;   /* this amounts to 1 sec */
const long duty = 300000; /* 300ms */

enum ButtonState {UP,DEBOUNCING,DOWN};
enum ButtonRole {PLUS,MINUS,SWITCH};

struct Button {
  int hold; // 0 - not pressed, 1 - holding button, 2 - incrementing_pause, 3 - incrementing
  long deadline;
  long anchor;
  ButtonState state;
  ButtonRole role;
  unsigned char pin;
};


void button_init(Button& b, int which, ButtonRole role) {
  b.hold=0;
  b.pin=which;
  b.state=UP;
  b.role=role;
  pinMode(which,INPUT);
}

int get_pulse(Button& b){
  if(digitalRead(b.pin)==OFF){
    b.state = UP;
    b.hold = 0;
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
      b.hold=1;
      b.anchor = micros();
      return 1;
    case DOWN:
      if(b.hold>0 && duration(now,b.anchor)>=duty) {
        if(b.hold == 2) {
           b.hold = 3; // Set increment's pause
           b.anchor = micros();
        }
        else if((duration(now,b.anchor)>=period && b.hold==1) || b.hold == 3) {
          b.hold = 2; //Button was held for 1s or Set increment's action 
        } 
      }
      return 0;
  }
 // return 0; //This is never reached. Just to fix warning: control reaches end of non-void function [-Wreturn-type] bug
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
  constDigitalWrite(latch_pin,HIGH); /* upon latch_pin rising edge, both 74HC595s will instantly change its outputs to */
  constDigitalWrite(latch_pin,LOW);  /* its internal value that we've just shifted in */
}

void disp_init()
{
  pinMode(latch_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);
  digitalWrite(clock_pin, LOW);
  shift_out(0);
}

void disp_7seg(unsigned char column, unsigned char glowing_leds_bitmask)
{
  shift_out((1<<(8+4+column)) | (0xFF^glowing_leds_bitmask));
}


// *** SETUP HELPERS *** \\

// button helpers

Button button_plus, button_minus, button_switch;

//  general

inline long duration(long now, long then) {return ((unsigned long)now)-then;}

int column=0;
int number=0;

// font is array of numbers for screen. 0th element is zero, 1st element is one and so on until nine
const unsigned char font[]={0b11111100, 0b01100000, 0b11011010, 0b11110010,0b01100110,0b10110110,0b10111110,0b11100000,0b11111110,0b11110110};

// *** Start Code *** \\
int number_arr[4] = {0,0,0,0};

void increment() {
  number++;
  if(number==10) {
    number=0;
  }
}
void decrement() {
  number--;
  if(number==-1) {
    number = 9;
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
  button_init(button_plus, button1_pin, PLUS);
  button_init(button_minus, button2_pin, MINUS);
  button_init(button_switch, button3_pin, SWITCH);
}

void loop() 
{
  if(get_pulse(button_plus)){
    increment();
  }
  if(get_pulse(button_minus)){
    decrement();
  }
  if(get_pulse(button_switch)){
    switch_button();
  }

  if(button_plus.hold==2)
    increment();
  if(button_minus.hold==2)
     decrement();
  if(button_switch.hold==2)
     switch_button();

   
  disp_7seg(column, font[number]);
}
