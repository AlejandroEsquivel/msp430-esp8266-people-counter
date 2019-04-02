#define ssid "AE iPhone"
#define pass "guestpass"
#define server "tcp.alejandroesquivel.com"
#define port "3100"

#define LED RED_LED


#define ECHO_1 P2_0
#define TRIG_1 P2_1

#define ECHO_2 P2_5
#define TRIG_2 P2_4

int threshold = 40;

int counter_1 = 0;
int counter_2 = 0;

unsigned long people_measure_start = 0;
unsigned long last_count_taken = 0;

int allow_double_measurement = 0;

int previous_count = 0;
int count = 0;

int people = 0;

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
  pinMode(LED, OUTPUT);   
  pinMode(TRIG_1, OUTPUT);
  pinMode(TRIG_2, OUTPUT);
  pinMode(ECHO_1, INPUT);
  pinMode(ECHO_2, INPUT);

  runtimeConfig();
  connectTcp();
  digitalWrite(LED,HIGH); // Turn on RED light when ready
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

  /*Serial.print("Distance 1: ");
  Serial.print(d1);
  Serial.print(" | ");
  Serial.println(counter_1);
  Serial.print("Distance 2: ");
  Serial.print(d2);
  Serial.print(" | ");
  Serial.println(counter_2);
  Serial.print("People: ");
  Serial.print(people);
  Serial.print("\n");*/

  if (allow_double_measurement == 1 || (d1 > threshold || d2 > threshold)) {

    if (counter_1 == 0 && d1 <= threshold) {
      counter_1 = 1;
      last_count_taken = millis();
      allow_double_measurement = 1;
      if (counter_2 == 1) {
        allow_double_measurement = 0;
        people_measure_start = millis();
        count--;

      }
    }

    if (counter_2 == 0 && d2 <= threshold) {
      counter_2 = 1;
      last_count_taken = millis();
      allow_double_measurement = 1;
      //raising edge
      if (counter_1 == 1) {
        allow_double_measurement = 0;
        people_measure_start = millis();
        count++;
      }
    }

  }

  //timeout of weird state
  if (millis() - last_count_taken > 1000) {
    last_count_taken = millis();
    counter_1 = 0;
    counter_2 = 0;
  }

  if (counter_2 == 1 && counter_1 == 1 && millis() - people_measure_start >= 250) {

    /*Serial.print("Time elapsed: ");
      Serial.print(millis() - people_measure_start);
      Serial.println();*/

    people_measure_start = 0;
    if (count > previous_count) {
      people++;
      sendMeasurement("+");
    }
    else if (count < previous_count) {
      people--;
      sendMeasurement("-");
    }

    if (people < 0) {
      people = 0;
    }

    previous_count = count;

    /*Serial.print("People: ");
    Serial.print(people);
    Serial.print("\n");
    Serial.print("Count: ");
    Serial.print(count);
    Serial.print("\n");*/

    people_measure_start = millis();

    counter_1 = 0;
    counter_2 = 0;

  }

  delay(50);

}