// spot_welder.ino ~ Copyright 2018 Paul Beaudet ~ MIT License

#include <JS_Timer.h>        // library from - https://github.com/paulbeaudet/JS_Timer
#define LED 13               // find and replace LED with 13: AKA the LED pin
#define TRANSFORMER_RELAY 5  // pin for relay that drives transformer that powers electrodes
#define PNEUMATIC_RELAY 6    // pin for relay that drives pnematic valve that clamps electrodes together
#define FOOT_TRIGGER 7       // pin for footpedel that actuates spot weld
#define BOUNCETIME 5         // Debounce foot trigger
#define HOLDSTATE 300        // time to count foot trigger as held
#define DEFAULT_WELD_TIME 30 // milliseconds to hold weld for

JS_Timer timer = JS_Timer(); // create an instance of our timer object from timer library

void setup() { // put your setup code here, to run once:
  pinMode(TRANSFORMER_RELAY, OUTPUT);   // Initiate relay pins
  pinMode(PNEUMATIC_RELAY, OUTPUT);
  pinMode(FOOT_TRIGGER, INPUT_PULLUP);  // Initiate
}

void loop() { // put your main code here, to run repeatedly:
  static int timeToWeld = DEFAULT_WELD_TIME;

  byte footPedelStatus = checkFootPedel(); // get running state of foot trigger
  if(footPedelStatus == 1){                // press foot pedel intiate weld sequence
    triggerWeld(timeToWeld);
  } else if ( footPedelStatus == 2){       // hold foot pedel increase weld time? Probably shit idea
    timeToWeld += 10;
  }

  timer.todoChecker();                     // check whether potential in progress weld sequences are done or not
}

// ---------------------------- Program Functions ------------------------------------- //

byte checkFootPedel(){
  static unsigned long pressTime = millis();
  static boolean timingState = false;
                                          // low is a press with the pullup
  if(digitalRead(FOOT_TRIGGER) == HIGH){    // if the button has been pressed
    if(timingState) {                     // given timer has started
      if(millis() - pressTime > BOUNCETIME){ // check if bounce time has elapesed
        if(millis() - pressTime > HOLDSTATE){// case button held longer return state 2
          return 2;                       // return hold state
        }
        return 1;                         // return debounced press state
      }
      return 0;                           // still in potential "bounce" window
    }
    timingState = true; // note that the timing state is set
    pressTime = millis();    // placemark when time press event started
    return 0;           // return with the timestate placeholder set
  }                     // outside of eventcases given no reading
  timingState = false;  // in case the timing state was set, unset
  return 0;             // not pressed
}

void stopWeld(){ // cut power to welder
  digitalWrite(TRANSFORMER_RELAY, LOW);
  digitalWrite(PNEUMATIC_RELAY, LOW);
}

void triggerWeld(int weldTime){ // start weld and set a time to cut power
  digitalWrite(TRANSFORMER_RELAY, HIGH);
  digitalWrite(PNEUMATIC_RELAY, HIGH);
  timer.setTimeout(stopWeld, weldTime);
}
