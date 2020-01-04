/**
 * @function measureEnemy : This function reads the diffuse reflex sensors and determines an enemy is detected.
 * @param : none
 * @return : Returns 0 if an enemy is detected, 1 otherwise.
 */


bool measureEnemy(){
    enemy_sensor_left = !digitalRead(PIN_SENSOR_LEFT);
    enemy_sensor_f_left = !digitalRead(PIN_SENSOR_F_LEFT);
    enemy_sensor_front = !digitalRead(PIN_SENSOR_FRONT);
    enemy_sensor_f_right = !digitalRead(PIN_SENSOR_F_RIGHT);
    enemy_sensor_right = !digitalRead(PIN_SENSOR_RIGHT);

    #if DEBUG > NONE
        Serial.print("\n>>> LOG: mediciones de enemigo: \t");
        Serial.print(enemy_sensor_left);
        Serial.print("\t");
        Serial.print(enemy_sensor_f_left);
        Serial.print("\t");
        Serial.print(enemy_sensor_front);
        Serial.print("\t");
        Serial.print(enemy_sensor_f_right);
        Serial.print("\t");
        Serial.print(enemy_sensor_right);
    #endif
        
    return (enemy_sensor_left) || (enemy_sensor_f_left) || (enemy_sensor_front) || (enemy_sensor_f_right) || (enemy_sensor_right);
}


void rc_read_values() {
    noInterrupts();
    memcpy(rc_values, (const void *)rc_shared, sizeof(rc_shared));
    interrupts();
}


void calc_input(uint8_t channel, uint8_t input_pin) {
    if (digitalRead(input_pin) == HIGH) {
        rc_start[channel] = micros();
    } 
    else {
        uint16_t rc_compare = (uint16_t)(micros() - rc_start[channel]);
        rc_shared[channel] = rc_compare;
    }
}


void calc_ch1() { calc_input(DIRECTION, DIRECTION_INPUT); }
void calc_ch2() { calc_input(THROTTLE, THROTTLE_INPUT); }
void calc_ch3() { calc_input(SWITCH, SWITCH_INPUT); }
void calc_ch4() { calc_input(BUTTON, BUTTON_INPUT); }
