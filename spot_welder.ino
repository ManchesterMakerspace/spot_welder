// spot_welder.ino ~ Copyright 2018 Paul Beaudet ~ MIT License

#include <JS_Timer.h>          // library from - https://github.com/paulbeaudet/JS_Timer
#define LED 13                 // find and replace LED with 13: AKA the LED pin
#define TRANSFORMER_RELAY 13   // pin for relay that drives transformer that powers electrodes
#define PNEUMATIC_RELAY 13     // pin for relay that drives pnematic valve that clamps electrodes together
#define FOOT_TRIGGER 4         // pin for footpedel that actuates spot weld
#define BOUNCETIME 5           // Debounce foot trigger
#define HOLDSTATE 300          // time to count foot trigger as held
#define DEFAULT_WELD_TIME 1000 // milliseconds to hold weld for
#define HOLD_INCREMENTOR 25    // resolution of timing adjustments

JS_Timer timer = JS_Timer(); // create an instance of our timer object from timer library

void setup() { // put your setup code here, to run once:
  pinMode(TRANSFORMER_RELAY, OUTPUT);   // Initiate relay pins
  pinMode(PNEUMATIC_RELAY, OUTPUT);
  pinMode(FOOT_TRIGGER, INPUT_PULLUP);  // Initiate reading signals from foot trigger
}

void loop() { // put your main code here, to run repeatedly:
  static int timeToWeld = DEFAULT_WELD_TIME;
  static byte lastFootState = 0;                   // for figuring state transitions
  static byte timerId = 0;

  byte triggerStatus = checkFootPedel();         // get running state of foot trigger
  if(lastFootState == 0 && triggerStatus == 1){  // press foot pedel intiate weld sequence
    timerId = triggerWeld(timeToWeld);
  }
  if(lastFootState == 1 && triggerStatus == 0 && timerId){
    stopWeld();                                  // if user lets go of trigger early it stops
    timer.clearTimeout(timerId);
    timerId = 0;
  }
  lastFootState = triggerStatus;           // make sure state is only triggered by intial press, instead of on each state read
  timer.todoChecker();                     // check whether potential in progress weld sequences are done or not
}

// ---------------------------- Program Functions ------------------------------------- //

byte checkFootPedel(){
  static unsigned long pressTime = millis();
  static boolean timingState = false;
                                              // low is a press with the pullup
  if(digitalRead(FOOT_TRIGGER) == LOW){       // if the button has been pressed
    if(timingState) {                         // given timer has started
      if(millis() - pressTime > BOUNCETIME){  // check if bounce time has elapesed
        return 1;                             // return debounced press state
      }
      return 0;                               // still in potential "bounce" window, short recording state again
    }
    timingState = true;    // note that state is in process of being monitored
    pressTime = millis();  // placemark when time press event started
    return 0;              // return with the timestate placeholder set
  }                        // outside of eventcases given no reading
  timingState = false;     // in case the timing state was set, unset
  return 0;                // not pressed
}

void stopWeld(){ // cut power to welder
  // digitalWrite(TRANSFORMER_RELAY, LOW); // proposition always keep Transformer running to avoid startup times
                                           // should test if this maters, probably safer to turn off
                                           // Having a warm up sequence would be pretty easy
  digitalWrite(PNEUMATIC_RELAY, LOW);
}

byte triggerWeld(int weldTime){ // start weld and set a time to cut power
  // digitalWrite(TRANSFORMER_RELAY, HIGH);
  digitalWrite(PNEUMATIC_RELAY, HIGH);
  return timer.setTimeout(stopWeld, weldTime);
}
