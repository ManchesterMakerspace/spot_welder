// spot_welder.ino ~ Copyright 2018 Paul Beaudet ~ MIT License

#include <JS_Timer.h>          // library from - https://github.com/paulbeaudet/JS_Timer
#define LED 13                 // find and replace LED with 13: AKA the LED pin
#define TRANSFORMER_RELAY 10   // pin for relay that drives transformer that powers electrodes
#define PNEUMATIC_RELAY 13     // pin for relay that drives pnematic valve that clamps electrodes together
#define FOOT_TRIGGER 4         // pin for footpedel that actuates spot weld
#define BOUNCETIME 5           // Debounce foot trigger
#define DEFAULT_WELD_TIME 1000 // milliseconds to hold weld for
#define HOLD_INCREMENTOR 25    // resolution of timing adjustments
#define TRASFORMER_WARM_UP 500 // milliseconds to electrify coil

JS_Timer timer = JS_Timer(); // create an instance of our timer object from timer library

void setup() { // put your setup code here, to run once:
  pinMode(TRANSFORMER_RELAY, OUTPUT);   // Initiate relay pins
  pinMode(PNEUMATIC_RELAY, OUTPUT);
  pinMode(FOOT_TRIGGER, INPUT_PULLUP);  // Initiate reading signals from foot trigger
  Serial.begin(9600);
}

void loop() { // put your main code here, to run repeatedly:
  static int timeToWeld = DEFAULT_WELD_TIME;
  static byte lastFootState = 0;                   // for figuring state

  byte triggerStatus = checkFootPedel();         // get running state of foot trigger
  if(lastFootState == 0 && triggerStatus == 1){  // press foot pedel intiate weld sequence
    Serial.println("starting weld");
    triggerWeld(timeToWeld);
  }
  if(lastFootState == 1 && triggerStatus == 0){
    sequenceState(0, 0); // terminate sequence
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

int sequenceState(byte currentStateTimer, int weldTime){ // Tracks timer ids and requested weld time within an async sequence
  static byte timerId = 0;                               // Id for clearing a set timeout event
  static int holdWeldTime = DEFAULT_WELD_TIME;           // Need to hold this data while, timer is running
  static boolean inSequence = false;                     // is sequence finished or not, for interuptions

  if(currentStateTimer == 0xff){ // timer stop condition, this is also the condition for running out of timers, but that shouldn't be problematic
    stopWeldCmds();              // Stop intiated by timer running out
    inSequence = false;
    timerId = 0;                 // timeout is already cleared in this condition
  } else if(currentStateTimer){  // given the next timer in sequence needs to be stored for potential interuption
    if(weldTime){holdWeldTime = weldTime;} // also store weld time because I'm not creating global variables
    inSequence = true;
    timerId = currentStateTimer;
  } else if(inSequence){          // given no timer was passed and in sequence, this signifies sequence termination event
    stopWeldCmds();               // premature manual stop by taking foot off of the trigger
    inSequence = false;
    Serial.println("clearing timeout");
    timer.clearTimeout(timerId);  // need to interupt timer, what was id of the timer? Problem this function basically solves
    timerId = 0;                  // make sure timer is cleared
  }
  return holdWeldTime;
}

void stopWeldCmds(){
  Serial.println("stopping weld");
  digitalWrite(TRANSFORMER_RELAY, LOW);
  digitalWrite(PNEUMATIC_RELAY, LOW);
}

void stopWeldPointer(){   // pointer for function that cut power to welder by timer
  sequenceState(0xff, 0); // easier to use this function call stop event and clear timerid within sequenceState
}

byte transformerWarmedUp(){   // now intiate pneumatic clamp
  Serial.println("Transformer warmed up");
  digitalWrite(PNEUMATIC_RELAY, HIGH);
  int weldTime = sequenceState(1, 0); // dummy signal to get weld time
  sequenceState(timer.setTimeout(stopWeldPointer, weldTime), 0);
}

byte triggerWeld(int weldTime){ // start by warming up transformer
  digitalWrite(TRANSFORMER_RELAY, HIGH);
  sequenceState(timer.setTimeout(transformerWarmedUp, TRASFORMER_WARM_UP), weldTime);
}
