/*                                                                                                         
  .g8"""bgd                                        db                            `7MM                     
.dP'     `M                                                                        MM                     
dM'       `   .gP"Ya  `7Mb,od8 `7MMpMMMb.pMMMb.  `7MM  `7MMpMMMb.   ,6"Yb.    ,M""bMM   ,pW"Wq.  `7Mb,od8 
MM           ,M'   Yb   MM' "'   MM    MM    MM    MM    MM    MM  8)   MM  ,AP    MM  6W'   `Wb   MM' "' 
MM.    `7MMF'8M""""""   MM       MM    MM    MM    MM    MM    MM   ,pm9MM  8MI    MM  8M     M8   MM     
`Mb.     MM  YM.    ,   MM       MM    MM    MM    MM    MM    MM  8M   MM  `Mb    MM  YA.   ,A9   MM     
  `"bmmmdPY   `Mbmmd' .JMML.   .JMML  JMML  JMML..JMML..JMML  JMML.`Moo9^Yo. `Wbmd"MML. `Ybmd9'  .JMML.  */

#define BLYNK_TEMPLATE_ID "TMPL2qDNYy2Q9"
#define BLYNK_TEMPLATE_NAME "Germinador"
#define BLYNK_FIRMWARE_VERSION "0.1.0"
#define BLYNK_PRINT Serial
//#define BLYNK_DEBUG
#define APP_DEBUG

#define D5 14      // Lampara
#define D0 16      // Ventiladores
#define D1 5       // Bomba de agua
#define DHT11PIN 4 // DHT11 - D2

#include "BlynkEdgent.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHTesp.h"

DHTesp dht;

int switch_lamp_state, switch_fans_state = 0;

int timer_on, timer_off, server_time; // Variables para guardar horario y comparar en servidor
String timeRangeString = ""; // String para mostrar valor en blynk Label

float dht_humidity, dht_temperature;
bool fans_control, pump_control; // Boolean para comparar estados

int analog_hum_value, mapped_hum_value;

// Ajusta la obtencion de la hora a UTC-3 Uruguay
const long utcOffsetInSeconds = -10800;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "south-america.pool.ntp.org", utcOffsetInSeconds);

void setup() {
  
  Serial.begin(115200);
  delay(100);
  
  BlynkEdgent.begin();
  timeClient.begin();

  pinMode(D5, OUTPUT);
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);

  dht.setup(4, DHTesp::DHT11);
}

void loop() {

  BlynkEdgent.run();
  timeClient.update();
  delay(dht.getMinimumSamplingPeriod()); // Periodo de actualizacion del DHT11
  getTimeServer();
  userInputTimer();
  fans();
  waterPump();

  dht_humidity = dht.getHumidity();
  dht_temperature = dht.getTemperature();
  analog_hum_value = analogRead(A0);

  // Mapeo del valor analogico del sensor de humedad a 0-100%
  mapped_hum_value = map(analog_hum_value, 600, 1023, 100, 0);

  // Envia los datos obtenidos a widgets (Blynk)
  Blynk.virtualWrite(V1, mapped_hum_value);
  Blynk.virtualWrite(V0, dht_humidity);
  Blynk.virtualWrite(V4, dht_temperature);
}

void waterPump() {

  // Verifica la humedad del suelo y activa la bomba de agua si es necesario
  if (mapped_hum_value <= 10 && !pump_control) {
    digitalWrite(D1, HIGH);
    pump_control = true;
    Serial.println("Water pump on");
  } else if (mapped_hum_value >= 30 && pump_control) {
    digitalWrite(D1, LOW);
    pump_control = false;
    Serial.println("Water pump off");
  }
}
/*
void waterPump() {

  unsigned long pump_time = 0; // Variable local para comparar los segundos de riego

  // Verifica la humedad del suelo y activa la bomba de agua si es necesario
  if (mapped_hum_value < 10 && !pump_control) {
    digitalWrite(D1, HIGH);
    pump_control = true;
    pump_time = millis();  // Registra el tiempo de inicio cuando se enciende la bomba
    Serial.println("Water pump on");
  } else if (pump_control && millis() - pump_time >= 10000) {
    digitalWrite(D1, LOW);
    pump_control = false;
    Serial.println("Water pump off");
  } else if (mapped_hum_value > 30 && pump_control) {
    digitalWrite(D1, LOW);
    pump_control = false;
    Serial.println("Water pump off (humidity > 30%)");
  }
}
*/
void fans() {

  // Verifica la temperatura del sensor DHT11 y activa los ventiladores
  if (dht_temperature >= 25 && !fans_control) {
    digitalWrite(D0, HIGH);
    fans_control = true;
    Serial.println("Fans on");
    Blynk.virtualWrite(V7, 1);
  } else if (dht_temperature <= 24 && fans_control) {
    digitalWrite(D0, LOW);
    fans_control = false;
    Serial.println("Fans off");
    Blynk.virtualWrite(V7, 0);
  }
}

void getTimeServer() {

  // Convierte la hora y minutos obtenida del servidor a segundos
  int HH = timeClient.getHours();
  int MM = timeClient.getMinutes();
  int SS = timeClient.getSeconds();
  server_time = 3600*HH + 60*MM + SS;
}

void userInputTimer() {

  // Compara si los valores introducidos son iguales a los obtenidos del servidor 
  if (timer_on == server_time) {
    digitalWrite(D5, HIGH); // Enciende la lampara
    Blynk.virtualWrite(V3, 1);
  } else if (timer_off == server_time) {
    digitalWrite(D5, LOW);  // Apaga la lampara
    Blynk.virtualWrite(V3, 0);
  }
}

void timerToString() {

  // Convierte los segundos ingresados en el timer a horas y minutos
  int start_hours = timer_on / 3600;
  int start_minutes = (timer_on % 3600) / 60;
  
  int end_hours = timer_off / 3600;
  int end_minutes = (timer_off % 3600) / 60;

  // Convierte enteros en un formato string (00:00 - 00:00)
  String timeRangeString = String(start_hours);
  if (start_minutes < 10) {
    timeRangeString += ":0" + String(start_minutes);
  } else {
    timeRangeString += ":" + String(start_minutes);
  }
  timeRangeString += " - ";
  timeRangeString += String(end_hours);
  if (end_minutes < 10) {
    timeRangeString += ":0" + String(end_minutes);
  } else {
    timeRangeString += ":" + String(end_minutes);
  }

  Serial.print("Timer on: ");
  Serial.println(timeRangeString);
  Blynk.virtualWrite(V6, timeRangeString); // Envia la cadena de texto a widget V6 (Label)
}

// Maneja el cambio en el estado del widget V5 (Timer)
BLYNK_WRITE(V5) {

  // Obtiene los valores ingresados por el usuario en segundos
  timer_on = param[0].asInt();
  timer_off = param[1].asInt();

  timerToString();
}

// Maneja el cambio en el estado del widget V3 (Lampara)
BLYNK_WRITE(V3) {

  switch_lamp_state = param.asInt(); // Lee el estado del switch

  if (switch_lamp_state == 1) {
    Serial.println("Light on");
    digitalWrite(D5, HIGH);
  } else {
    Serial.println("Light off");
    digitalWrite(D5, LOW);
  }
}

// Maneja el cambio en el estado del widget V7 (Ventiladores)
BLYNK_WRITE(V7) {

  switch_fans_state = param.asInt(); // Lee el estado del switch

  if (switch_fans_state == 1) {
    Serial.println("Fans on");
    digitalWrite(D0, HIGH);
  } else {
    Serial.println("Fans off");
    digitalWrite(D0, LOW);
  }
}