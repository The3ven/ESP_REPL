#include <Arduino.h>

void setup() {
  pinMode(2, OUTPUT);
  Serial.println("Starting");
}

void loop() {
  int x = 10;
  while (x > 0) {
    digitalWrite(2, HIGH);
    delay(500);
    digitalWrite(2, LOW);
    delay(500);
    x--;
    Serial.print("Count: ");
    Serial.println(x);
  }
}
