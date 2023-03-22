/*
 Operant Task Master Script
 By: Brielle Ferguson
 Task will test animals ability to remember cue location and respond 
 either in the appropriate port to receive a subsequent reward.
Can be customized for training, and testing of different types based on the 
variables defined below.
*/

//set up servo motor
#include <Servo.h>
int servoOpen = 0;
int servoClosed = 150;
Servo myservo;  // create servo object to control the servo motor which will give access to the doors 

//define pins that will control various task hardware

//visual cue lights positioned around the center initiation port
#define centerLWhiteLED 13
#define centerRWhiteLED 5

//visual cue lights positioned in the reward ports for MATCH-TO-SAMPLE training or testing
#define leftRewardWhiteLED 11
#define rightRewardWhiteLED 7

//sound cues 
#define upsweepTTL 34  
#define downsweepTTL 32 

// visual cue lights defined for the NON-MATCH TO SAMPLE in the reward ports 
//#define leftRewardWhiteLED 24
//#define rightRewardWhiteLED 26

//LEDs that can be used to either signal modality in the crossmodal task or be used as a house light indicating trial availability
#define GreenLED 42
#define BlueLED 40

//output to syringe pumps
#define leftRewardPort 44 
#define rightRewardPort 46 

//IR Photodiode Receivers
#define IR_Photodioderight 20
#define IR_PhotodiodeCenter 18
#define IR_Photodiodeleft 3

//IR LED Pins
#define IR_LEDright 4 //right
#define IR_LEDcenter 8 //Center
#define IR_LEDleft 12 //left

//will send a TTL pulse to NIDA to sync camera data with trial initiation or directly to LED driver for optostim 
#define trialInitiationTTL 30 

//variables that will be updated throughout the task
//ints to monitor the beam break status in each of the ports
int sensorStateleft = 0, sensorStateCenter = 0, sensorStateright = 0; 
int lastStateleft = 0, lastStateCenter = 0, lastStateright = 0;

//will keep track of the choices, trials, accuracy
int leftChoices = 0; 
int rightChoices = 0; 
int correctTrials = 0; 
int incorrectTrials = 0; 
int trialsCompleted = 0; 
String command; 
int side; 
int correctSide;
int trialModality;

// User can set up various session parameters. 

/*set times for various session parameters for training or testing*/ 
int cleanPortsDelay = 10000; //time (ms) ports are open to collect milk prior to the trial sequence beginning
int required = 200; //time (ms) mouse must hold its nose in the center poke to initiate a trial
int modalityCueLength = 1000; //length of modality cue, green or blue LED 
int cueLength = 5000; //length (ms) of visual and auditory stim, can either set here (variableCue = false), or not define and use the sequence below (variableCue = true)
int delayToCue = 0; //delay time (ms) between sample (visual or auditory stim) and reward port availability (variableDelay = false to use delay declared here, variableDelay = true to use sequence)
int timeout = 15000; //timeout (ms) where a trial isn't available after animal makes an error
int rewardAvailable = 10000; //time (ms) animal has to consume reward until the reward ports close 
int numTrials = 50; //doesn't really matter because the loop will just repeat   
unsigned long trialMinutes = 30; //(in minutes) after this amount of time is reached the serial monitor will display summary stats about the animal's performance. 
unsigned long totalSessionTime =  trialMinutes * 60000; //converts number of minutes into milliseconds
unsigned long startSession = millis(); //will record the current time, and compare to this after each trial to determine how much time has elapsed and whether to report summary stats
unsigned long endSession;  

/* change probability that the trial sequence will repeat */
int repeatProbability = 100;//probability trial will repeat following an error, make 100 for training and 0 for testing 
long repeatTrial; //random number generated that will determine whether an incorrect choice results in a repeat of the previous trial. 

/*define task mode for various test types */
bool variableCue = false; //when true the cue length varies pseudorandomly from the cue length sequence, make true for 5CL or 3CL testing, false will use cueLength above          <--------------------------------------------------------------
bool variableDelay = false;// true will use delayLength sequence, false will use delayLength above 
bool visualTrial = true; //LEDs serve as cues for the correct reward port
bool auditoryTrial = false; //sounds (upsweep and downsweep) servr as cues for the correct port
bool crossmodalTrial = false; //true for crossmodal training and testing
bool checkConflict = false; //true for crossmodal testing
bool modalityVariable = false;
bool conflict = false; //if you're using crossmodal task, will either read this or use the conflict sequence
bool accuracy; // will update on each trial during training or testing

/* optogenetic stimulation*/
bool optoStim = false; //if randomOptoStim and blockOptoStim are false, will use this value to always or never use opto stim
int pulselength = 5; //in ms
int optoFreq = 40; //in Hz 
int optoPulseDelay = 1000/optoFreq; //determine pulses per second based on optoFreq
bool FiP = true; //true for sending ttl pulse to NIDA at trial initiation 
bool optoStimPulse = false; //will send pulses at pulseLength and optoFreq 
bool optoStimContinuous = true; //will use continuous opto stim
bool cueOptoStim = true; //opto during cue
bool responseOptoStim = false; //opto during response
bool delayOptoStim = false; //opto durung delay between trial initiation and the cue 
bool blockOptoStim = false; //if true will use sequence of blocks of 5 off 5 on opto stim trials
bool randomOptoStim = false; // if true will use the optoProbability defined below 
int optoProbability = 0; //if randomOptoStim = true, this represents the chance that the arduino will trigger opto stim on a given trial
long optoOn; //variable to save random number generated to determine optoStim if randomOptoStim = true


//all trial, cue length, delay, modality, and opto stim sequences for various session types. Uncomment desired sequence below
/*Declare variables for all the different trial types, see trialtype function below for more details on what each means*/ 
enum{training = 1, leftCue = 2, rightCue = 3, leftDMTP = 4,  rightDMTP = 5, leftDNMTP = 6, rightDNMTP = 7, cleanPorts = 8, habituation1 = 9, habituation2 = 10}; 
enum{visual = 11, auditory = 12};

/*Each sequence begins with a cleanPorts trial, as sending the sketch to the board, 
results in errant ttl pulses that trigger reward delivery. Experimenter can clean out the reward ports during cleanPortsDelay (declared above before placing animal in the chamber. */

//int trialSequence[] = {habituation1};

/*will open the doors and flash the light at the specified cueLength (use 5s)*/
//int trialSequence[] = {habituation2};

/*teaching them to initiate trials*/ 
//int trialSequence[] = {training};
 
/*pseudorandom seqeunce of  50correct sides, designed ot be counterbalanced equally for both variable cue lengths and delay lengths, always use for training or 5 cue length testing  <---------------------------------------------*/
int trialSequence[]  ={rightCue, leftCue, leftCue, rightCue, rightCue, leftCue, leftCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, leftCue,
leftCue, rightCue, rightCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, leftCue, leftCue, rightCue, rightCue, rightCue, leftCue, rightCue, 
rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, leftCue, leftCue, rightCue, rightCue, leftCue, rightCue, 
rightCue, leftCue, leftCue, rightCue};

/*pseudorandom seqeunce of 60 correct sides, designed ot be counterbalanced equally for variableCue testing with 3 cue lengths*/
//int trialSequence[] = {2, 3, 3, 2, 2, 2, 3, 2, 2, 3, 3, 3, 2, 3, 2, 2, 3, 3, 2, 3, 3, 2, 2, 2, 3, 2, 3, 3, 2, 3, 2, 3, 2, 3, 3, 3, 2, 2, 2, 3, 2, 2, 3, 3, 2, 3, 2, 2, 3, 2, 2, 3, 3, 2, 3, 3, 2, 2, 3, 3};   // for 3 CL testing <---------------------------------


/*single-side cued training with more right choices*/
//int trialSequence[]  = {cleanPorts, rightCue, rightCue,  leftCue, rightCue, rightCue, leftCue, rightCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, rightCue, rightCue, leftCue, rightCue, leftCue, 
//leftCue, rightCue, rightCue, leftCue, rightCue, leftCue, rightCue, rightCue,  leftCue, rightCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue}; 

/*more left*/
//int trialSequence[]  = {cleanPorts, leftCue, leftCue, leftCue, rightCue, rightCue, leftCue, leftCue, rightCue, leftCue, leftCue, leftCue,  rightCue, leftCue, leftCue, rightCue, leftCue, 
//leftCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue, leftCue, leftCue, rightCue, rightCue, leftCue, rightCue, leftCue, rightCue, leftCue}; 

/*Pseudorandom sequence of cue lengths to be used for the variableCue version of the task with 5 cue lengths */  
//  int cueLengthSequence[] = {100, 500, 1000, 5000, 2000, 2000, 100, 1000, 5000, 500, 500, 1000, 100, 1000, 2000, 5000, 500, 100, 5000, 2000, 2000, 5000,
//  500, 1000, 5000, 500, 2000, 100, 100, 1000, 500, 1000, 1000, 100, 2000, 2000, 100, 5000, 5000, 500, 2000, 5000, 1000, 100, 1000, 500, 5000, 100, 500, 2000};

/*Pseudorandom sequence of cue lengths to be used for the variableCue version of the task with three cue lengths 5000, 2000, 500 */ 
int cueLengthSequence[] = {5000, 500, 2000,  500, 500, 5000,  2000,  2000,  500, 5000,  500, 5000,  2000,  500, 2000,  2000,  5000,  2000,  2000,  500, 2000,  500, 5000,  500, 2000,  2000,  5000,  2000,
5000,  5000,  500, 5000,  2000,  500, 500, 500, 5000,  500, 5000,  500, 2000,  5000,  5000,  2000,  500, 2000,  500, 5000,  5000,  2000,  5000,  500, 2000,  2000,  5000,  2000, 500, 5000,  500, 5000};

/*Blocks of indivdual cue lengths from 5CL task, can be used as a control to ensure group differences are due to attention and not perception/vision*/
//int cueLengthSequence[] = {5000 , 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 
//2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 1000, 1000 , 1000 , 1000 , 1000, 
//1000 , 1000 , 1000 , 1000 , 1000, 500, 500, 500, 500, 500, 500, 500, 500, 500, 500, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};  


/*Pseudorandom sequence of delay lengths to be used for the variableDelay version of the task with three cue lenghts - 3, 4, and 5s */
int variableDelayLengthSequence[] = {4000, 3000, 5000, 3000, 3000, 4000, 5000, 5000, 3000, 4000, 3000, 4000, 5000, 3000, 5000, 5000, 4000, 5000, 5000, 3000, 5000, 3000, 4000, 3000, 5000, 5000, 4000, 
5000, 4000, 4000, 3000, 4000, 5000, 3000, 3000, 3000, 4000, 3000, 4000, 3000, 5000, 4000, 4000, 5000, 3000, 5000, 3000, 4000, 4000, 5000, 4000, 3000, 5000, 5000, 4000, 5000, 3000, 4000, 3000, 4000};

/*Pseudorandom sequence of delay lengths to be used for the variableDelay version of the task with 2, 4, 8, 6, and 16s */
//unsigned long variableDelayLengthSequence[] = {2000, 16000, 8000, 4000, 1000, 1000, 2000, 8000, 4000, 16000, 16000, 8000, 2000, 8000, 1000, 4000, 16000, 2000, 4000, 
//1000, 1000, 4000, 16000, 8000, 4000, 16000, 1000, 2000, 2000, 8000, 16000, 8000, 8000, 2000, 1000, 1000, 2000, 4000, 4000, 16000, 1000, 4000, 8000,
//2000, 8000, 16000, 4000, 2000, 16000, 1000}; 

/*Pseudorandom sequence of delay lengths to be used for the variableDelay version of the task 100ms, 500 ms, 1s, 2s, 5s */
//unsigned long variableDelayLengthSequence[] = {100, 500, 1000, 5000, 2000, 2000, 100, 1000, 5000, 500, 500, 1000, 100, 1000, 2000, 5000, 500, 100, 5000, 2000, 2000, 5000,
//500, 1000, 5000, 500, 2000, 100, 100, 1000, 500, 1000, 1000, 100, 2000, 2000, 100, 5000, 5000, 500, 2000, 5000, 1000, 100, 1000, 500, 5000, 100, 500, 2000};

/*Pseudorandom sequence of delay lengths to be used for the variableDelay version of the task 1,2,5,8,and 10s */
//unsigned long variableDelayLengthSequence[] = {10000, 8000, 1000, 5000, 2000, 2000, 10000, 1000, 5000, 8000, 8000, 1000, 10000, 1000, 2000, 5000, 8000, 10000, 5000, 2000, 2000, 5000,
//8000, 1000, 5000, 8000, 2000, 10000, 10000, 1000, 8000, 1000, 1000, 10000, 2000, 2000, 10000, 5000, 5000, 8000, 2000, 5000, 1000, 10000, 1000, 8000, 5000, 10000, 8000, 2000};

/*Pseudorandom sequence of delay lengths to be used for the variableDelay version of the task 1,2,4,8,16s */
//unsigned long variableDelayLengthSequence[] = {2000, 16000, 8000, 4000, 1000, 1000, 2000, 8000, 4000, 16000, 16000, 8000, 2000, 8000, 1000, 4000, 16000, 2000, 4000, 
//1000, 1000, 4000, 16000, 8000, 4000, 16000, 1000, 2000, 2000, 8000, 16000, 8000, 8000, 2000, 1000, 1000, 2000, 4000, 4000, 16000, 1000, 4000, 8000,
//2000, 8000, 16000, 4000, 2000, 16000, 1000}; 

/*pseudorandom visual and auditory trials for crossmodal task*/ 
//int trialModalitySequence[]  = {visual, auditory, auditory, visual, visual, auditory, auditory, visual, auditory, visual, auditory, visual,visual, auditory, auditory, visual, visual, auditory, auditory, visual,
//visual, auditory, auditory, visual, visual, auditory, auditory, visual, auditory, visual,auditory, visual,visual, auditory, auditory, visual, visual, auditory, auditory, visual,
//visual, auditory, auditory, visual, visual, auditory, auditory, visual, auditory, visual, auditory, visual,visual, auditory, auditory, visual, visual, auditory, auditory, visual,
//visual, auditory, auditory, visual, visual, auditory, auditory, visual, auditory, visual,auditory, visual,visual, auditory, auditory, visual, visual, auditory, auditory, visual}; 

/*5A5V blocks for training during for the crossmodal task*/
int trialModalitySequence[]  = {auditory, auditory, auditory, auditory, auditory, visual, visual, visual, visual, visual,auditory, auditory, auditory, auditory, auditory, visual, visual, visual, visual, visual, 
auditory, auditory, auditory, auditory, auditory, visual, visual, visual, visual, visual, auditory, auditory, auditory, auditory, auditory, visual, visual, visual, visual, visual, 
auditory, auditory, auditory, auditory, auditory, visual, visual, visual, visual, visual}; 

/*7A3V blocks for training during for the crossmodal task*/
//int trialModalitySequence[]  = {auditory, auditory, auditory, auditory, auditory, auditory, auditory, visual, visual, visual,auditory, auditory, auditory, auditory, auditory, visual, visual, visual, auditory, auditory, 
//auditory, auditory, auditory, auditory, auditory, auditory, auditory, visual, visual, visual, auditory, auditory, auditory, auditory, auditory, auditory, auditory, visual, visual, visual, 
//auditory, auditory, auditory, auditory, auditory, auditory, auditory, visual, visual, visual}; 

/*10A3V blocks for training during for the crossmodal task*/
//int trialModalitySequence[]  = {auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, auditory, 
//auditory, auditory, auditory, auditory, auditory, auditory, auditory, visual, visual, visual, auditory, auditory, auditory, auditory, auditory, auditory, auditory, visual, visual, visual, 
//auditory, auditory, auditory, auditory, auditory, auditory, auditory, visual, visual, visual}; 

/*1A1V blocks for training during for the crossmodal task*/
//int trialModalitySequence[]  = {auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, 
//auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual, 
//auditory, visual, auditory, visual, auditory, visual, auditory, visual, auditory, visual}; 

/*Blocks of 5 off 5 on trials for optogenetic stim, will be used if blockOptoStim = true*/
bool optoStimSequence[] = {false, false, false, false, false, true, true, true, true, true, false, false, false, false, false, true, true, true, true, true, false, false, false, false, false, 
true, true, true, true, true, false, false, false, false, false, true, true, true, true, true, false, false, false, false, false, true, true, true, true, true};

/*Blocks for training in crossmodal task if conflict is set to true*/
bool conflictSequence[] = {true, false, true, true, true, false, false, false, false, false,true, true, true, true, true, false, false, false, false, false, true, true, true, true, true, false, false, false, false, false,
true, true, true, true, true, false, false, false, false, false, true, true, true, true, true, false, false, false, false, false}; 


/*initialize all pins defined above as inputs, outputs, and turn them on if on throughout the task, will initialize the serial monitor*/
void setup() {
  myservo.attach(48);  // attaches the servo on pin 49 to the servo object
  closePokes();
  pinMode(leftRewardPort, OUTPUT); 
  pinMode(rightRewardPort, OUTPUT); 
  pinMode(GreenLED, OUTPUT); 
  pinMode(BlueLED, OUTPUT); 
  pinMode(centerLWhiteLED, OUTPUT);
  pinMode(centerRWhiteLED, OUTPUT);
  pinMode(leftRewardWhiteLED, OUTPUT);
  pinMode(rightRewardWhiteLED, OUTPUT);
  pinMode(upsweepTTL, OUTPUT);
  pinMode(downsweepTTL, OUTPUT); 
  digitalWrite(upsweepTTL, HIGH); 
  digitalWrite(downsweepTTL, HIGH); 
  pinMode(IR_Photodioderight, INPUT);  
  pinMode(IR_PhotodiodeCenter, INPUT);  
  pinMode(IR_Photodiodeleft, INPUT); 
  digitalWrite(IR_Photodioderight, HIGH);   
  digitalWrite(IR_PhotodiodeCenter, HIGH);
  digitalWrite(IR_Photodiodeleft, HIGH); 
  pinMode(IR_LEDright, OUTPUT);  
  pinMode(IR_LEDcenter, OUTPUT); 
  pinMode(IR_LEDleft, OUTPUT); 
  digitalWrite(IR_LEDright, HIGH);   
  digitalWrite(IR_LEDcenter, HIGH);
  digitalWrite(IR_LEDleft, HIGH);
  digitalWrite(trialInitiationTTL,OUTPUT);
  digitalWrite(trialInitiationTTL,LOW);
  randomSeed(analogRead(0));
  Serial.begin(115200); //turns on the serial monitor 
  Serial.println("Begin Session");
  Serial.println("Clean Ports"); 
  myservo.write(servoOpen); 
  intertrialInterval(cleanPortsDelay); //gives user time to clean out ports
  myservo.write(servoClosed);
}

//below all functions are defined that will be used throughout the task to be called during the main function

/*delay between trials*/
void intratrialInterval (int delayLength) {
  delay(delayLength);
}

/*delay during trials*/
void intertrialInterval (int delayLength) {
  delay(delayLength);
}

/*array to be used in switch case statements for everything where function passes in a side or both sides*/
enum{left = 1, right = 2, both = 3}; 

/*will dispense a reward to int side (left,right, or both) by sending a 1s ttl pulse to a syringe pump*/
void dispenseReward (int side) {
  switch (side) {
    case left: 
      Serial.println("Dispensing reward left");
      digitalWrite(leftRewardPort, HIGH); //left reward delivery by activating left syringe pump 
      delay(1000);  
      digitalWrite(leftRewardPort, LOW);
    break; 
    case right: 
      Serial.println("Dispensing reward right");
      digitalWrite(rightRewardPort, HIGH); //right reward delivery by activating left syringe pump 
      delay(1000);  
      digitalWrite(rightRewardPort, LOW);
    break; 
    case both: 
      Serial.println("Dispensing reward to both ports");
      digitalWrite(leftRewardPort, HIGH); //left reward delivery by activating left syringe pump 
      digitalWrite(rightRewardPort, HIGH); //right reward delivery by activating left syringe pump
      delay(1000);   
      digitalWrite(leftRewardPort, LOW);
      digitalWrite(rightRewardPort, LOW);
    break;
    
  }
}

/*useful for sending to commands to change operant task during a session, can only be used if viewing session through arduinos serial monitor, not in MATLAB*/
void executeUserCommand(String command) {
        if(command.equals("dispense reward left")){
           Serial.print("Executing User Command - "); 
           dispenseReward(left);  
        }
        else if (command.equals("dispense reward right")){
          Serial.print("Executing User Command - "); 
          dispenseReward(right); 
        } 
        else if (command.equals("deactivate right reward port")){
           Serial.println("Executing User Command - Deactivating Right Reward Port"); 
           digitalWrite(IR_LEDright, LOW);   
        }
        else if (command.equals("deactivate left reward port")){
           Serial.println("Executing User Command - Deactivating Left Reward Port"); 
           digitalWrite(IR_LEDleft, LOW);    
        }
        else if (command.equals("activate right reward port")){
           Serial.println("Executing User Command - Activating Right Reward Port"); 
           digitalWrite(IR_LEDright, HIGH);  
        }
        else if (command.equals("activate left reward port")){
           Serial.println("Executing User Command - Activating Left Reward Port"); 
           digitalWrite(IR_LEDleft, HIGH);   
        }
        else if (command.equals("open reward ports")){
           Serial.println("Executing User Command - Opening Reward Ports"); 
           myservo.write(servoOpen);  
        }
        else if (command.equals("close reward ports")){
           Serial.println("Executing User Command - Closing Reward Ports"); 
           closePokes();    
        }
        else if (command.equals("repeat if incorrect")){
           Serial.println("Executing User Command - Repeat Probability = 100%"); 
           repeatProbability = 100;    
        }
        else if (command.equals("do not repeat if incorrect")){
           Serial.println("Executing User Command - Repeat Probability = 0"); 
           repeatProbability = 0;    
        }
        else if (command.equals("no opto stim")){
           Serial.println("Optogenetic Stim turned off"); 
           randomOptoStim = false; 
           blockOptoStim = false;   
        }
        else{
            Serial.println("Invalid command");
        }  
}

/*will monitor the state of all ports and continously report those states, until the delay
"int d" has passed. */
void checkBeamBreakForDelay(unsigned long d) {
  unsigned long startTime = millis(); 
  unsigned long endTime = startTime; 
  while ((endTime - startTime) <= d) 
    { 
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.println("Left Port Entry");
        leftChoices = leftChoices + 1; 
      }
      lastStateleft = sensorStateleft;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Port Entry");
      }
      lastStateCenter = sensorStateCenter;

      if (sensorStateright && !lastStateright) {
        Serial.println("Right Port Entry");
        rightChoices = rightChoices + 1; 
      }
      lastStateright = sensorStateright;
      endTime = millis(); 
   }
}


/* Continuously monitors the state of each port for entry, if entry is either left or right, 
will report entry, and then report the status of all ports for 20 seconds while animal can freely
collect rewards. If entry is center, will report and keep looping until a left or right choice is made.*/ 
void checkBeamBreakForChoice() { 
  bool beamBreakStatus = false; 
  while (beamBreakStatus == false) 
    {
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.println("Left Port Entry First");
        checkBeamBreakForDelay(10000); 
        beamBreakStatus = true; 
        leftChoices = leftChoices + 1; 
      }
      lastStateleft = sensorStateleft;

     
      if (sensorStateright && !lastStateright) {
        Serial.println("Right Port Entry First");
        checkBeamBreakForDelay(10000); 
        beamBreakStatus = true; 
        rightChoices = rightChoices + 1; 
      }
      lastStateright = sensorStateright;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Port Entry");
      }
      lastStateCenter = sensorStateCenter;
    }
}

/*continuously montors the state of all ports until the a choice is made,
then will exit the loop. Once choice is made, will dispense reward into the port that was entered. */
void waitAnyChoice() {
  bool choiceMade = false; 
  while (choiceMade == false) 
    {
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.print("Left Port Entry - ");
        choiceMade = true;
        leftChoices = leftChoices + 1; 
        dispenseReward(left); 
        intratrialInterval(15000);
      }
      lastStateleft = sensorStateleft;
     
      if (sensorStateright && !lastStateright) {
        Serial.print("Right Port Entry - ");
        choiceMade = true; 
        rightChoices = rightChoices + 1; 
        dispenseReward(right);
        intertrialInterval(15000);
      }
      lastStateright = sensorStateright;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Port Entry");
      }
      lastStateCenter = sensorStateCenter;
    }
} 

/*continuously monitors the state of all ports until the correct choice is made, will dispense reward and
then will exit the loop. If incorrect choice is made will report the choice and that
it was incorrect, and keep waiting for the correct choice. If the correct choice was made, 
will report the choice, and that it was correct, dispense reward and then exit the loop.*/
void waitCorrectChoice(int correctChoice) {
  bool correctChoiceMade = false; 
  bool beamBreakleft = false;
  bool beamBreakright = false;
  while (correctChoiceMade == false) 
    {
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.print("Left Port Entry");
        beamBreakleft = true;
        leftChoices = leftChoices + 1; 
        if(beamBreakleft == true && correctChoice == left){
          Serial.println(" - Correct");
          dispenseReward(left); 
          correctChoiceMade = true; 
          correctTrials = correctTrials + 1; 
          intertrialInterval(rewardAvailable);
          closePokes(); 
        }
        else {
          Serial.println(" - Incorrect"); 
         if (visualTrial) {
         flashLED(correctChoice); 
        }
         if (auditoryTrial) {
         playSound(correctChoice); 
        } 
          incorrectTrials = incorrectTrials + 1; 
        }
      }
      lastStateleft = sensorStateleft;
     
      if (sensorStateright && !lastStateright) {
        Serial.print("Right Port Entry");
        beamBreakright = true; 
        rightChoices = rightChoices + 1; 
        if(beamBreakright == true && correctChoice == right) {        
          Serial.println(" - Correct");
          dispenseReward(right); 
          correctChoiceMade = true; 
          correctTrials = correctTrials + 1; 
          intertrialInterval(rewardAvailable);
          closePokes(); 
        }
        else {
          Serial.println(" - Incorrect");
        if (visualTrial) {
         flashLED(correctChoice); 
        }
         if (auditoryTrial) {
         playSound(correctChoice); 
        } 
          incorrectTrials = incorrectTrials + 1; 
        }
      }
      lastStateright = sensorStateright;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Port Entry");
      }
      lastStateCenter = sensorStateCenter;
    }
} 

/*continuously monitors the state of all ports until the a choice is made, then will exit
the loop. If correct, wil dispense the reward, and exit the loop. If the incorrect choice 
is made, will report the incorrect choice, and then exit the loop.*/
 
void checkCorrectChoice(int correctChoice) {
  bool choiceMade = false; 
  bool beamBreakleft = false;
  bool beamBreakright = false;
  while (choiceMade == false) 
    {
      sensorStateleft = digitalRead(IR_Photodiodeleft);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
      sensorStateright = digitalRead(IR_Photodioderight);

      if (sensorStateleft && !lastStateleft) {
        Serial.print("Left Reward Port Entry");
        beamBreakleft = true;
        leftChoices = leftChoices + 1;
        if(beamBreakleft == true && correctChoice == left){
          Serial.println(" - Correct  ");
          accuracy = true;  
          choiceMade = true; 
          correctTrials = correctTrials + 1; 
          dispenseReward(left);
          if (optoStim) {
            if (responseOptoStim) {
              sendOptoStimForDuration(rewardAvailable);
            }
            if (!responseOptoStim) {
              intertrialInterval(rewardAvailable);
            }
          }
          if (!optoStim) {
          intertrialInterval(rewardAvailable);
          }
          closePokes();                                                                                                       
        }
        else {
          Serial.println(" - Incorrect  "); 
          accuracy = false; 
          choiceMade = true;
          incorrectTrials = incorrectTrials + 1; 
          closePokes(); 
          digitalWrite(GreenLED, LOW); 
          digitalWrite(BlueLED, LOW); 
          intertrialInterval(timeout);
        }
      }
      lastStateleft = sensorStateleft;
     
      if (sensorStateright && !lastStateright) {
        Serial.print("Right Reward Port Entry");
        beamBreakright = true; 
        rightChoices = rightChoices + 1; 
        if(beamBreakright == true && correctChoice == right) {        
          Serial.println(" - Correct  ");
          accuracy = true; 
          choiceMade = true;
          correctTrials = correctTrials + 1; 
          dispenseReward(right); 
          if (optoStim) {
            if (responseOptoStim) {
              sendOptoStimForDuration(rewardAvailable);
            }
            if (!responseOptoStim) {
              intertrialInterval(rewardAvailable);
            }
          }
          if (!optoStim) {
          intertrialInterval(rewardAvailable);
          }          

          closePokes(); 
        }
        else {
          Serial.println(" - Incorrect  ");
          accuracy = false; 
          choiceMade = true;
          incorrectTrials = incorrectTrials + 1; 
          closePokes();
          digitalWrite(GreenLED, LOW);
          digitalWrite(BlueLED,LOW);  
          intertrialInterval(timeout);
        }
      }
      lastStateright = sensorStateright;

      if (sensorStateCenter && !lastStateCenter) {
        Serial.println("Center Initiation Port Entry");
      }
      lastStateCenter = sensorStateCenter;
    }
} 

/*will send a stim at optoFreq (defined above) for a specified delay, "d", at the optoStimFrequency and pulselength set above.*/ 
void sendOptoStimForDuration(unsigned long d) {
  unsigned long startTime = millis(); 
  unsigned long endTime = startTime; 
  if (optoStim){
  if (optoStimPulse) {
  while ((endTime - startTime) <= d) 
    { 
      digitalWrite(trialInitiationTTL, HIGH);
      delay(pulselength);//length of the pulse is 5ms
      digitalWrite(trialInitiationTTL, LOW);
      delay(optoPulseDelay - pulselength); //length of the delay till the next pulse is 25 ms - the pulse length to get a 40 Hz frequency. 100 to get a 10 Hz frequency
      endTime = millis(); 
   }
   Serial.println("Optogenetic Stim provided");
  }
    if (optoStimContinuous) {
     
      digitalWrite(trialInitiationTTL, HIGH);
      delay(d);
      digitalWrite(trialInitiationTTL, LOW);
   
   Serial.println("Optogenetic Stim provided");
  }
  
}
  if (!optoStim) {
    delay(d);
  }
}

/*Will wait for a poke in the center nose port, and then will send a trigger TTL to mark trialInitiation (FiP = true),
or send trigger for optoStim (optoStim = true). Will also call the visual cue, auditory cue, or crossmodal cue */
void waitTrialInitiation (int side) { 
  bool centerBeamBreakStatus = false; 
  while (centerBeamBreakStatus == false) 
  {
    sensorStateCenter = digitalRead(IR_PhotodiodeCenter);
    if (sensorStateCenter && !lastStateCenter) {       
      delay(required);
      sensorStateCenter = digitalRead(IR_PhotodiodeCenter); 
      if (sensorStateCenter) {
      centerBeamBreakStatus = true; 
      Serial.println("Center Port Entry - Trial Initiated");

      if (FiP) {
      //send TTL pulse to fiber photometry data acquisition system  
      digitalWrite(trialInitiationTTL, HIGH);
      delay(500);  
      digitalWrite(trialInitiationTTL, LOW);
      }

      if (optoStim) {
        if (delayOptoStim) {
          sendOptoStimForDuration(variableDelay);
        }
        else {
          delay(variableDelay);
        }
      }
      else {
        delay(variableDelay);
      }
  
      if (!conflict) {
        if (visualTrial) {
           flashLED(side); 
        }
        if (auditoryTrial) {
           playSound(side); 
        }
      }
      if (conflict) {
          playCrossmodalCue(trialModality,side); 
      }
              
      }      
    }
    lastStateCenter = sensorStateCenter;

    //check for user input
    if(Serial.available()){
         command = Serial.readStringUntil('\n');
         executeUserCommand(command); 
    }
  }
} 


/*send TTL pulse to arduino to flash LED for cueLength (defined above), can also trigger opto stim (if optoStim = true) 
during the cue (cueOptoStim = true), delay (delayOptoStim = true), or response (responseOptoStim = true) if designated above */
void flashLED (int visualCue) {
  switch (visualCue)  {
    case left: //left port reward cue 
      analogWrite(leftRewardWhiteLED, 50);
      if (optoStim) {
        if (cueOptoStim) {
        sendOptoStimForDuration(cueLength);
        }
        if (responseOptoStim) {
          delay(cueLength); 
        }
        if (delayOptoStim) {
          delay(cueLength);
        }
      }
      if (!optoStim) {
      delay(cueLength); 
      } 
      analogWrite(leftRewardWhiteLED, 0);
    break;
    case right: //right port reward cue 
      analogWrite(rightRewardWhiteLED, 50);

      if (optoStim) {
        if (cueOptoStim) {
        sendOptoStimForDuration(cueLength);
        }
        if (responseOptoStim) {
          delay(cueLength); 
        }
         if (delayOptoStim) {
          delay(cueLength);
        }
      }
      if (!optoStim) {
      delay(cueLength); 
      } 
  
      analogWrite(rightRewardWhiteLED, 0);
    break;  
    case both: //simultaneous flash of both LEDs
      analogWrite(rightRewardWhiteLED, 50);
      analogWrite(leftRewardWhiteLED, 50);
      delay(cueLength);  
      analogWrite(rightRewardWhiteLED, 0);
      analogWrite(leftRewardWhiteLED, 0);
  }  
}

/*if running a crossmodal session (crossmodal = true), will flash Green LED to indicate the visual cue, 
and the Blue LED to indicate the auditory cue */
void playModalityCue (int modality) {
  switch (modality)  {
    case visual: //left port reward cue 
//    Serial.println("Flashing Green LED"); 
    digitalWrite(GreenLED, HIGH);
    digitalWrite(BlueLED, LOW); 
    break;
    case auditory: //right port reward cue 
//    Serial.println("Flashing Blue LED"); 
      digitalWrite(BlueLED, HIGH);
      digitalWrite(GreenLED, LOW); 
    break;  
  }  
}
/*variables for the upsweep and downswep*/
enum{downsweep = 1, upsweep = 2};

/*Will send a pulse to an adafruit soundcard to play an upsweep or downsweep, currently set up to play the sound cue twice  */
void playSound (int soundCue) {
  switch (soundCue)  {
    case upsweep: //right port reward cue 
      digitalWrite(upsweepTTL, LOW);
      delay(300);  
      digitalWrite(upsweepTTL, HIGH);
      delay(300);  
      digitalWrite(upsweepTTL, LOW);
      delay(300);
      digitalWrite(upsweepTTL, HIGH);
//      delay(300);  
//      digitalWrite(upsweepTTL, LOW);
//      delay(300);
//      digitalWrite(upsweepTTL, HIGH);
    break;
    case downsweep: //left port reward cue 
      digitalWrite(downsweepTTL, LOW);
      delay(300);  
      digitalWrite(downsweepTTL, HIGH);
      delay(300);  
      digitalWrite(downsweepTTL, LOW);
      delay(300);  
      digitalWrite(downsweepTTL, HIGH);
//      delay(300);  
//      digitalWrite(downsweepTTL, LOW);
//      delay(300);  
//      digitalWrite(downsweepTTL, HIGH);      
    break;  
    case both:          
      digitalWrite(downsweepTTL, LOW);
      delay(300);  
      digitalWrite(downsweepTTL, HIGH);
      delay(300);  
      digitalWrite(upsweepTTL, LOW);
      delay(300);
      digitalWrite(upsweepTTL, HIGH);
    break; 
  }  
}

/*Will play the correct cue, based on the trialModality and correctSide, if visual and left, will flash left on the right
and play the sound cue for the right and vice versa */
void playCrossmodalCue (int trialModality, int correctSide ) {
   switch (trialModality)  {
    case visual: //will play the coordinating visual cue and conflicting auditory cue 
      if (correctSide == left) {
      digitalWrite(leftRewardWhiteLED, HIGH);
      playSound(right); 
      delay(cueLength - 300);  //to account for the length of the playSound() function 
      digitalWrite(leftRewardWhiteLED, LOW);       
      Serial.println("Flashing LED and playing sound"); 
      }
      if (correctSide == right) {
      digitalWrite(rightRewardWhiteLED, HIGH);
      playSound(left); 
      delay(cueLength - 300);  //to account for the length of the playSound() function 
      digitalWrite(rightRewardWhiteLED, LOW);
      Serial.println("Flashing LED and playing sound");           
      }
    break;
    case auditory: //will play the coordinating auditory cue and conflicting visual cue 
      if (correctSide == left) {
      digitalWrite(rightRewardWhiteLED, HIGH);
      playSound(left); 
      delay(cueLength - 300);  //to account for the length of the playSound() function 
      digitalWrite(rightRewardWhiteLED, LOW); 
      Serial.println("Flashing LED and playing sound");        
      }
      if (correctSide == right) {
      digitalWrite(leftRewardWhiteLED, HIGH);
      playSound(right); 
      delay(cueLength - 300);  //to account for the length of the playSound() function 
      digitalWrite(leftRewardWhiteLED, LOW);     
      Serial.println("Flashing LED and playing sound");      
      }
    break;  
  } 
}

/*close nose pokes by movng down ten units on the servo until the doors reach the closed position*/
void closePokes() {
  for (int i = 0; i <= servoClosed; i += 10) {
    myservo.write(servoOpen + i);
    delay(100);
  }
  myservo.write(servoClosed);
  delay(15);
}

/*conv3rt the trial types to strings that can be displayed in the serial monitor*/
String eventToString (int e){
  switch (e) {
    case training: return "training";
    case leftCue: return "Left Cue";
    case rightCue: return "Right Cue";
    case leftDMTP: return "Left DMTP";
    case rightDMTP: return "Right DMTP";
    case leftDNMTP: return "Left DNMTP"; 
    case rightDNMTP: return "Right DNMTP";   
    case cleanPorts: return "Clean Ports"; 
    case habituation1: return "Habituation - Phase 1";
    case habituation2: return "Habituation - Phase 2"; 
    case auditory: return "Auditory"; 
    case visual: return "Visual"; 
  }
}

/*chain of events for each trial type*/
void executeTrial(int trialType) {
  switch (trialType) {
    case cleanPorts:
    //ports open for 10 seconds so experimenter can clean ports       
      myservo.write(servoOpen); 
       intertrialInterval(20000); 
      closePokes();
    break; 
    case habituation1:/*paint the reward ports with milk and let them consume milk from all three for 10 minutes. Doors will open and close 
       and the reward will be dispensed every 2 minutes */ 
      myservo.write(servoOpen); 
      checkBeamBreakForDelay(120000); 
      closePokes();
      intratrialInterval(5000); 
      myservo.write(servoOpen); 
      dispenseReward(both); 
      checkBeamBreakForDelay(120000); 
      closePokes();
      intratrialInterval(5000); 
      myservo.write(servoOpen); 
      dispenseReward(both);   
      checkBeamBreakForDelay(120000); 
      closePokes();
      intratrialInterval(5000); 
      myservo.write(servoOpen); 
      dispenseReward(both);   
      checkBeamBreakForDelay(120000);
      closePokes();
    
    break; 
    case habituation2: //will play cues for both sides, wait for animal to make a choice, dispense reward in the choice port, and start a new trial after 5 s
      myservo.write(servoOpen); 
      if (visualTrial) {
           flashLED(both); 
        }
        if (auditoryTrial) {
           playSound(both); 
        }
      waitAnyChoice();//will dispense reward after choice
      closePokes();
      intertrialInterval(5000);
    break; 
    case training:
    //animal pokes nose in center port to initiate trial, both lights flash, reward is available in both sides after a nose poke       
      waitTrialInitiation(both); 
      myservo.write(servoOpen); 
      waitAnyChoice();
      closePokes();
      intertrialInterval(5000);
    break;       
    case leftCue: 
      correctSide = left;
      waitTrialInitiation(left); 
      myservo.write(servoOpen); 
      checkCorrectChoice(left);
    break; 
    case rightCue: 
      correctSide = right;
      waitTrialInitiation(right); 
      myservo.write(servoOpen); 
      checkCorrectChoice(right); 
     break; 
     case leftDMTP: //for DMTP and DNMTP, do we want for the mouse to have to place its nose back in the initiation port to get the go cue to make the next choice? 
      waitTrialInitiation(left);
      flashLED(left); 
      intertrialInterval(500); 
      myservo.write(servoOpen); 
      waitCorrectChoice(left);
      dispenseReward(left); 
      intertrialInterval(10000);
      flashLED(both); 
      checkCorrectChoice(left);
      closePokes();
      intratrialInterval(5000);
     break; 
     case rightDMTP:
      waitTrialInitiation(right);
      flashLED(right); 
      intertrialInterval(500); 
      myservo.write(servoOpen); 
      waitCorrectChoice(right);
      dispenseReward(right); 
      intertrialInterval(10000);
      flashLED(both); 
      checkCorrectChoice(left);
      closePokes();   
      intratrialInterval(5000); 
     break; 
     case leftDNMTP:
      waitTrialInitiation(left);
      flashLED(left); 
      intertrialInterval(500); 
      myservo.write(servoOpen); 
      waitCorrectChoice(left);
      dispenseReward(left); 
      intertrialInterval(10000);
      flashLED(both); 
      checkCorrectChoice(right);
      closePokes(); 
      intratrialInterval(5000);   
     break; 
     case rightDNMTP: 
      waitTrialInitiation(right);
      flashLED(right); 
      intertrialInterval(500); 
      myservo.write(servoOpen); 
      waitCorrectChoice(right);
      dispenseReward(right); 
      intertrialInterval(10000);
      flashLED(both); 
      checkCorrectChoice(left);
      closePokes(); 
      intratrialInterval(5000);     
     break;         
  }
}

/*after sessionTime (defined above) will display summary stats through the serial monitor*/
void displaySummaryStats () {
    Serial.println("Session ended"); 
    Serial.println("Performance Summary:"); 
    Serial.print("Number of Trials: ");
    Serial.println(trialsCompleted);
    Serial.print("Left Choices: ");
    Serial.println(leftChoices);
    Serial.print("Right Choices: ");
    Serial.println(rightChoices); 
    Serial.print("Correct Choices: ");
    Serial.println(correctTrials);
    Serial.print("Incorrect Choices: ");
    Serial.println(incorrectTrials); 
}


/*main loop that will execute the desired trial sequence using the parameters defined above*/
void loop() { 

   for (int i = 0; i < numTrials; i++){

                if(Serial.available()){
                    command = Serial.readStringUntil('\n');
                    executeUserCommand(command); 
                }
                
                Serial.print("Trial Available: "); 
                String trialString = eventToString(trialSequence[i]); 

                if (modalityVariable){
                trialModality = trialModalitySequence[i];
                if (trialModality == visual) {
                  visualTrial = true; 
                  auditoryTrial = false; }
                if (trialModality == auditory){
                  auditoryTrial = true; 
                  visualTrial = false;                
                }
                playModalityCue(trialModality);
                }

//              Crossmodal version of the task                
                if (checkConflict) {
                conflict = conflictSequence[i];
                if (conflict) {
                Serial.print("Conflict "); 
                }
                if (!conflict) {
                  Serial.print("No conflict ");
                }
                }


                //if not doing the crossmodal and want to keep the modality light and use as house light to indicate trial availabity and to turn 
                //off during a timeout 

                  if (visualTrial) {
                    digitalWrite(GreenLED, HIGH);
                    digitalWrite(BlueLED, LOW);
                  }
                  if (auditoryTrial) {
                    digitalWrite(BlueLED, HIGH); 
                    digitalWrite(GreenLED, LOW);
                  }
                  
                if (modalityVariable) {
                //print out whether trial is visual or auditory
                String trialModalityString = eventToString(trialModality);  
                Serial.print(trialString);
                Serial.print(" - ");              
                Serial.println(trialModalityString); 
                }

                //will print out trial string for all training sessions except the working memory and sustained attention task 
                if (!variableCue && !variableDelay && !crossmodalTrial && !modalityVariable) {
                 Serial.println(trialString);  
                }

//              variableCue version of the task 
                if (variableCue) {
                Serial.print(trialString);
                Serial.print(" Length - "); 
                cueLength = cueLengthSequence[i];
                Serial.println(cueLength); 
                }

//              variableDelay version of the task 
                if (variableDelay) {
                Serial.print(trialString);
                Serial.print(" Delay - "); 
                variableDelay = variableDelayLengthSequence[i];
                Serial.println(variableDelay);
                }
                if (blockOptoStim) {
                 optoStim = optoStimSequence[i];
  
                }

                if (randomOptoStim) {
                  optoOn = random(100);
                  //Serial.println(optoOn);
                  if (optoOn < optoProbability) {
                    optoStim = true;
                  }
                  else {
                    optoStim = false; 
                  }
                }
                
                executeTrial(trialSequence[i]);     
                   
                
                if (!accuracy) {
                  repeatTrial =  random(100); 
                  if (repeatTrial < repeatProbability) {
                    i = i - 1; 
                    Serial.println("Will repeat previous trial");
                  }               
                }

                endSession = millis(); 
                trialsCompleted = trialsCompleted + 1; 

                 if (trialsCompleted > 100) {
                          Serial.println("100 Trials Completed"); 
                 } 
                
                 if ((endSession - startSession) > totalSessionTime) {
                          displaySummaryStats(); 
                          startSession = millis();
                 }              
          }
}
