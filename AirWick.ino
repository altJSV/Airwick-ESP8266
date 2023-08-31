#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>

// Объект для обнавления с web страницы
ESP8266HTTPUpdateServer httpUpdater;

// Web интерфейс для устройства
ESP8266WebServer HTTP(80);

// Для файловой системы
File fsUploadFile;

#define FLL_VERSION (" Ver.1.0")

const int lightSensorPin = A0;  // Пин, к которому подключен датчик света
const int motorPin = D2;        // Пин, к которому подключен мотор
const int buttonPin = D3;       // Пин, к которому подключена кнопка


unsigned long previousTime = 0;        // Предыдущее время опроса датчика
const unsigned long interval = 10000;  // Интервал опроса датчика (10 секунд)

unsigned long pretimerStartTime = 0;   // Время старта предварительного таймера
unsigned long timerStartTime = 0;      // Время старта таймера интервала распыления
unsigned long timerDuration = 240000;  // Длительность таймера (120 секунд)
unsigned long preTimer = 60000;        // Длительность таймера (60 секунд)
uint16_t lightTreshold = 600;          //порог срабатывания датчика света
bool workmode = false;                 // флаг запуска таймера режима распыления


String configSetup = "{}";
String configJson = "{}";
String mqttconfigJson = "{}";

String mqttServer = "192.168.1.100";
int mqttPort = 1883;
String mqttUser = "login";
String mqttPassword = "pass";
String cmdTopic = "/motor";
String statusTopic = "/status";
String clientID = "ESP8266Client-";  //id клиента
bool useMQTT = true;                 //флаг использования mqtt

WiFiClient espClient;
PubSubClient client(espClient);


// Установка значения порога освещения для включения таймеров  http://192.168.0.101/set_lightTreshold?val=1
void handle_set_lightTreshold() {
  lightTreshold=HTTP.arg("val").toInt();// Получаем значение порога освещения для включения таймеров
  jsonWrite(configSetup, "lightTreshold", lightTreshold); // Получаем значение порога освещения из запроса конвертируем в int сохраняем
  saveConfig();
  HTTP.send(200, "text/plain", "OK");
}

 

  void setup() {
    Serial.begin(115200);
    delay(5);
    Serial.println("");

    pinMode(lightSensorPin, INPUT);
    pinMode(motorPin, OUTPUT);
    pinMode(buttonPin, INPUT_PULLUP);  // Устанавливаем режим входа с подтяжкой вверх для кнопки
    digitalWrite(motorPin, LOW);       // Устанавливаем мотор в выключенное состояние
    pinMode(LED_BUILTIN, OUTPUT);      // Устанавливаем режим пина светодиода на OUTPUT
    digitalWrite(LED_BUILTIN, HIGH);   // Устанавливаем высокий уровень сигнала на пин светодиода (отключаем светодиод)

    //Запускаем файловую систему
    FS_init();
    configSetup = readFile("config.json", 4096);
    jsonWrite(configJson, "SSDP", jsonRead(configSetup, "SSDP"));
    Serial.println(configSetup);
    //чтение порогового значения датчика света
    lightTreshold = jsonReadtoInt(configSetup, "lightTreshold");
    if (lightTreshold==0) lightTreshold=600; 
    //Запускаем WIFI
    WIFIinit();
    // Получаем время из сети
    Time_init();
    //Настраиваем и запускаем SSDP интерфейс
    SSDP_init();
    //Настраиваем и запускаем HTTP интерфейс
    HTTP_init();
    // Подключение к MQTT-серверу
    init_mqtt();
    //Инициализация построения графика
    GRAF_init();
  }

  void loop() {
    HTTP.handleClient();
    delay(1);
    unsigned long currentTime = millis();

    // Опрос датчика каждый заданный интервал секунд
    if (currentTime - previousTime >= interval) {
      previousTime = currentTime;
      int lightLevel = analogRead(lightSensorPin);
      
    Serial.println(lightLevel);
    /*if (client.connected()) //Отправка в топик сообщения об уровне освещения
    {
    char msg[80];
    sprintf(msg,"Уровень освещения: %d",lightLevel);
    client.publish(statusTopic.c_str(),msg);
    }
    */
      if (lightLevel > lightTreshold) {
        // Если свет горит, запускаем предварительный таймер
        if (pretimerStartTime == 0 && workmode == false) {
          Serial.println("pretimer start!");
          pretimerStartTime = currentTime;
        }
      } else {
        // Если свет выключен, останавливаем предварительныйтаймер
        pretimerStartTime = 0;
        Serial.println("pretimer stopped");
        workmode = false;  //отключаем режим распыления
      }
    }
    bool buttonState = digitalRead(buttonPin);  // Считываем состояние кнопки

    if (buttonState == LOW) {
      // Кнопка нажата, запустить мотор на 1 секунду
      Serial.println("Кнопка нажата");
      if (client.connected())  //Отправка в топик сообщения об уровне освещения
      {
        client.publish(statusTopic.c_str(), "Распыляем освежитель по нажатию кнопки");
      }
      digitalWrite(motorPin, HIGH);
      delay(1000);
      digitalWrite(motorPin, LOW);
    }

    // Проверяем состояние предварительного таймера
    if (pretimerStartTime > 0 && currentTime - pretimerStartTime >= preTimer) {
      pretimerStartTime = 0;  // Таймер истек, сбрасываем его состояние
      Serial.println("Switch to work mode");
      workmode = true;               //переходим в режим распыления
      timerStartTime = currentTime;  //запускаем таймер распыления
    }

    // Проверяем состояние основного таймера
    if (timerStartTime > 0 && currentTime - timerStartTime >= timerDuration) {
      //если свет все еще горит перезапускаем таймер, если нет - останавливаем
      if (workmode) {
        timerStartTime = currentTime;
        Serial.println("Timer Restart");
      } else {
        timerStartTime = 0;
        Serial.println("Timer stoped");
      }
      Serial.println("Pulverize!!!");  //выводим в консоль, что мотор заработал
      if (client.connected())          //Отправка в топик сообщения об уровне освещения
      {
        client.publish(statusTopic.c_str(), "Распыляем освежитель");
      }
      digitalWrite(motorPin, HIGH);  // Запускаем мотор на 1 секунду
      delay(1000);
      digitalWrite(motorPin, LOW);  // Выключаем мотор
    }
    // Проверка подключения к MQTT-серверу
    if (!client.connected() && useMQTT) {
      connectToMqtt();
    }
    client.loop();
  }
