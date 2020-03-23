/**
 * @brief: Radio-Controlled Sumo robot test code.
 * @author: Lucas Cortés Gutiérrez.
 * @date: 2019/11/10
 */

#include <EnableInterrupt.h>
#include <Sabertooth.h>

#define NONE 0
#define ENABLE 0        // Change this to 1 when testing for real.
#define DEBUG !ENABLE   // When enabled, this will print via serial prompt all the measurements and messages written into this file, as well as the "Tactic's" files.

#define DRIFT 50        // Stick drift dead-zone.
#define THRESHOLD 50    // Switch threshold.
#define TURN_TIME 300   // Time (in mili-seconds) needed for a 180 degree turn.
#define DIVIDER 1       // POWER_MAX divider.
#define POWER_MAX 255   // Maximum power supplied through the motor driver.
double POWER = 0;

/**
 * @brief : Reflex sensors pinout definition
 */

#define PIN_SENSOR_LEFT 7
#define PIN_SENSOR_F_LEFT 8
#define PIN_SENSOR_FRONT 9
#define PIN_SENSOR_F_RIGHT 10
#define PIN_SENSOR_RIGHT 11

#define TEST_COUNT_MAX 500
#define MISS_COUNT_MAX 1000

/**
 * @brief : PID controller constants.
 * We used the Ziegler–Nichols method to tune this parameters.
 */

#define KP 0.032
#define KI 0.0008
#define KD 0.016

#define REF 3000

double enemy_sensor_left = 0;
double enemy_sensor_f_left = 0;
double enemy_sensor_front = 0;
double enemy_sensor_f_right = 0;
double enemy_sensor_right = 0;

double actuation = 0;
double error_prev = 0;
double error = 0;
double error_int = 0;
double error_dev = 0;
double measure = 0;
double ref = REF;

int count = 0;
int miss_count = 0;
int tactic = 0;

/**
 * @brief : RC channels configuration
 */

#define RC_NUM_CHANNELS 4

#define DIRECTION 0
#define THROTTLE 1
#define SWITCH 2
#define BUTTON 3

#define DIRECTION_INPUT A0  // Right stick, X axis
#define THROTTLE_INPUT A1   // Left stick, Y axis
#define SWITCH_INPUT A2     // SWA & SWD
#define BUTTON_INPUT A3     // KEY2

uint16_t rc_values[RC_NUM_CHANNELS];
uint32_t rc_start[RC_NUM_CHANNELS];
volatile uint16_t rc_shared[RC_NUM_CHANNELS];

Sabertooth ST(128);

void setup(){
    pinMode(DIRECTION_INPUT, INPUT);
    pinMode(THROTTLE_INPUT, INPUT);
    pinMode(SWITCH_INPUT, INPUT);
    pinMode(BUTTON_INPUT, INPUT);

    enableInterrupt(DIRECTION_INPUT, calc_ch1, CHANGE);
    enableInterrupt(THROTTLE_INPUT, calc_ch2, CHANGE);
    enableInterrupt(SWITCH_INPUT, calc_ch3, CHANGE);
    enableInterrupt(BUTTON_INPUT, calc_ch4, CHANGE);

    pinMode(LED_BUILTIN, OUTPUT);
    SabertoothTXPinSerial.begin(9600);
    ST.autobaud();

    //pinMode(PIN_START_BUTTON, INPUT_PULLUP);
    //attachInterrupt(digitalPinToInterrupt(PIN_START_BUTTON), stop, FALLING);
    //while(!digitalRead(PIN_START_BUTTON));

    //Serial.println("Wait for 5 seconds ... ");


    //delay(5050);

    pacman(); // This is optional, obviously.
    //elephant(); // This is optional, obviously. alternative song.
    //elephant_short(); // This is optional, obviously. alternative song.

    #if DEBUG > NONE
        Serial.println("WAITING FOR SWITCH 'SWA' IN DOWN POSITION...");
    #endif

    while ((rc_values[SWITCH] < 1500 - THRESHOLD) || (rc_values[SWITCH] > 1500 + THRESHOLD)) rc_read_values();
    digitalWrite(LED_BUILTIN, HIGH);

    #if DEBUG > NONE
        Serial.println("START!");
    #endif

}

void loop(){
    rc_read_values();
    #if DEBUG > NONE
        Serial.print("DIRECTION:"); Serial.print(rc_values[DIRECTION]); Serial.print("\t");
        Serial.print("THROTTLE:"); Serial.print(rc_values[THROTTLE]); Serial.print("\t");
        Serial.print("SWITCHES:"); Serial.print(rc_values[SWITCH]); Serial.print("\t");
        Serial.print("BUTTON:"); Serial.println(rc_values[BUTTON]);
    #endif

    if ((rc_values[SWITCH] > 1500 - THRESHOLD) && (rc_values[SWITCH] < 1500 + THRESHOLD)) {

        POWER = ((rc_values[THROTTLE] < 1500 - DRIFT) || (rc_values[THROTTLE] > 1500 + DRIFT)) ? (rc_values[THROTTLE] - 1500.0)/(2*DIVIDER) : 0;


        if ((rc_values[BUTTON] > 2000 - THRESHOLD) && (rc_values[BUTTON] < 2000 + THRESHOLD)) {
            #if ENABLE > NONE
                ST.motor(1, POWER_MAX/DIVIDER);
                ST.motor(2, -POWER_MAX/DIVIDER);
                delay(TURN_TIME);
            #endif

            #if DEBUG > NONE
                Serial.println("SICK 180 DEGREE TURN! :D");
            #endif
        }

        if ((rc_values[DIRECTION] > 1500 - DRIFT) && (rc_values[DIRECTION] < 1500 + DRIFT)) {
            #if ENABLE > NONE
                ST.motor(1, POWER);
                ST.motor(2, POWER);
            #endif

            #if DEBUG > NONE
                if (POWER == 0) {Serial.println("IDLE");}
                else if (POWER > 0) {Serial.print("FORWARD : "); Serial.println(POWER);}
                else if (POWER < 0) {Serial.print("BACKWARD : "); Serial.println(POWER);}
            #endif
        }

        else if (rc_values[DIRECTION] > 1500 + DRIFT) {
            #if ENABLE > NONE
                ST.motor(1, POWER);
                ST.motor(2, -POWER);
            #endif

            #if DEBUG > NONE
                Serial.print("RIGHT : "); Serial.println(POWER);
            #endif
        }

        else if (rc_values[DIRECTION] < 1500 - DRIFT) {
            #if ENABLE > NONE
                ST.motor(1, -POWER);
                ST.motor(2, POWER);
            #endif
            #if DEBUG > NONE
                Serial.print("LEFT : "); Serial.println(POWER);
            #endif
        }

        #if DEBUG > NONE
            delay(100);
        #endif
    }

    else if ((rc_values[SWITCH] > 2000 - THRESHOLD) && (rc_values[SWITCH] < 2000 + THRESHOLD)){

        if(measureEnemy()){

            if(searchPID()){

                POWER = (rc_values[THROTTLE] > 1500 + DRIFT) ? (rc_values[THROTTLE] - 1500.0)/(2*DIVIDER) : 0; // Be aware that the power definition is different from the one in line 134, this is due to our need of using only positive values, nonetheless, this can be changed if the need arise.

                #if ENABLE > NONE
                    ST.motor(1, POWER);
                    ST.motor(2, POWER);
                #endif

                #if DEBUG > NONE
                    if (POWER == 0) {Serial.println("IDLE");}
                    else if (POWER > 0) {Serial.print("FIGHTING : "); Serial.println(POWER);}
                #endif
                //fight();
            }
        }
    }

    else {
        digitalWrite(LED_BUILTIN, LOW);
        ST.motor(1, 0);
        ST.motor(2, 0);

        #if DEBUG > NONE
            Serial.println("STOP!");
        #endif

        while(true);
    }
}
