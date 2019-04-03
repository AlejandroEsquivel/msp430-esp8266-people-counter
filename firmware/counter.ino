#define ssid "AE iPhone"
#define pass "guestpass"
#define server "tcp.alejandroesquivel.com"
#define port "3100"



#define ECHO_1 P2_0
#define TRIG_1 P2_1

#define ECHO_2 P2_5
#define TRIG_2 P2_4

int threshold = 40;

int counter_1 = 0;
int counter_2 = 0;

int state = -1;
int prev_state = -1;

long last_measurement_taken = 0;

int people = 0;

int prev_people = 0;

// P1_1 GREEN
// P1_2 YELLOW

void sendMeasurement(char* data) {
  Serial.println("AT+CIPSEND=1"); // prepare to send x byte
  delay(1000);
  Serial.println(data);

}

void flashConfig() {
  //  Flash Wifi-Mode: As client
  Serial.println("AT+CWMODE_DEF=1");
  delay(2000);
  // Flash ssid credentials
  Serial.print("AT+CWJAP_DEF=\"");
  Serial.print(ssid);
  Serial.print("\",\"");
  Serial.print(pass);
  Serial.println("\"");
  delay(20 * 1000);
}

void runtimeConfig() {
  Serial.println("ATE0"); // Disable echo
  delay(1000);
  Serial.println("AT+CIPMUX=0"); // Single-Connection Mode
  delay(1000);
}

void connectTcp() {
  Serial.print("AT+CIPSTART=\"TCP\",\"");  // Establish TCP connection
  Serial.print(server);
  Serial.print("\",");
  Serial.println(port);
  delay(10 * 1000);
}

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);
  delay(1000);

  //init pins
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(TRIG_1, OUTPUT);
  pinMode(TRIG_2, OUTPUT);
  pinMode(ECHO_1, INPUT);
  pinMode(ECHO_2, INPUT);

  runtimeConfig();
  connectTcp();
  digitalWrite(GREEN_LED, HIGH); // Turn on RED light when ready
}

void trigger(int TRIG_PIN) {
  //Serial.println("Emitting pulses...");
  digitalWrite(TRIG_PIN, HIGH);  // Enable Trigger
  delayMicroseconds(10); // Pulse for 10us
  digitalWrite(TRIG_PIN, LOW);
}

double takeMeasurement(int TRIG_PIN, int ECHO_PIN) {

  trigger(TRIG_PIN);

  float distance;

  int last_echo = 0;
  int curr_echo = 0;

  double start_time;
  double end_time;

  while (1) {

    curr_echo = digitalRead(ECHO_PIN);

    if (curr_echo > last_echo) {

      //rising edge
      start_time = micros();

    } else if (curr_echo < last_echo) {

      //falling edge
      end_time = micros();
      distance = (end_time - start_time) / 58.0;

      if (distance > 0 && distance <= 2000) {
        return distance;
      }
      else {
        return -1;
      }

    }
    last_echo = curr_echo;
  }
}


// the loop routine runs over and over again forever:
void loop() {
  
  double d1 = takeMeasurement(TRIG_1, ECHO_1);
  double d2 = takeMeasurement(TRIG_2, ECHO_2);

  digitalWrite(RED_LED, LOW);

  if(state != 4){
    if (d1 <= threshold) {
      counter_1 = 1;
      last_measurement_taken = millis();
    }
  
    if (d2 <= threshold) {
      counter_2 = 1;
      last_measurement_taken = millis();
    }
  
    if(counter_1 == 1 && counter_2 == 0){
      state = 1;
    }
    else if(counter_1 == 0 && counter_2 ==1){
      state = 2;
    } else if(counter_1 == 1 && counter_2 ==1){
      state = 3;
    } else {
      state = 0;
    }
  
    if(state == 3 && prev_state == 1){
      state = 4;
      last_measurement_taken = millis();
      people++;
    }
    else if(state == 3 && prev_state == 2){
      state = 4;
      last_measurement_taken = millis();
      people--;
    }
  
  
    //timeout of weird state
    if (millis() - last_measurement_taken >= 500) {
      last_measurement_taken = millis();
      counter_1 = 0;
      counter_2 = 0;
      prev_state = state;
      state = 0;
    }
    /*Serial.print("Prev state: ");
    Serial.print(prev_state);
    Serial.print(", State: ");
    Serial.print(state);
    Serial.print(", People:");
    Serial.println(people);*/

    prev_state = state;
  }
  else if (millis() - last_measurement_taken >= 250) {

     digitalWrite(RED_LED, HIGH);

    //Serial.println(people);
    //Serial.print("People Count: ");
    //Serial.println(people);

    if(prev_people < people){
      sendMeasurement("+");
    }
    else if(prev_people > people){
      sendMeasurement("-");
    }

    counter_1 = 0;
    counter_2 = 0;
    prev_state = state;
    state = 0;
    prev_people = people;
    
    last_measurement_taken = millis();
    
  }


}