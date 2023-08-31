 void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // обработка полученного сообщения
  Serial.println(topic);
  Serial.print("Message:");
  String message;
  String command;
  int param;
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }
  Serial.println(message);
  command = message.substring(0, 5);
  Serial.println(command);
  param = message.substring(5).toInt();
  Serial.println(param);
    if (command== "motor") {digitalWrite(motorPin, HIGH);  // Включение мотора
                 delay(1000);
                 digitalWrite(motorPin, LOW);  // Выключение мотора
    }
    if (command=="light"){ if (param>0 && param<=1024){
                      lightTreshold=param;
                      jsonWrite(configSetup, "lightTreshold", lightTreshold); // Получаем значение порога освещения из запроса конвертируем в int сохраняем
                      saveConfig();
                      char msg[80];
                      sprintf(msg,"Порог освещения установлен на: %d",lightTreshold);
                      client.publish(statusTopic.c_str(),msg);
                    }
    }
     if (command=="pretm"){ if (param>0 && param<=10){
                      preTimer=param*60000;//присваиваем новое значение предварительному таймеру
                      jsonWrite(configSetup, "preTimer", param); // сохраняем в json
                      saveConfig();
                      char msg[80];
                      sprintf(msg,"Предварительный таймер установлен на: %d мин",param);
                      client.publish(statusTopic.c_str(),msg);
                    }
    }
     if (command=="timer"){ if (param>0 && param<=60){
                      timerDuration=param*60000;//присваиваем новое значение основного таймера
                      jsonWrite(configSetup, "Interval", param); // сохраняем в json
                      saveConfig();
                      char msg[80];
                      sprintf(msg,"Основной таймер установлен на: %d мин",param);
                      client.publish(statusTopic.c_str(),msg);
                    }
    }
    if (command=="lowpwr"){ if (param>=0 && param<2){
                      lowPower=param;
                      jsonWrite(configSetup, "lowPWR", param); // сохраняем в json
                      saveConfig();
                      if (lowPower){client.publish(statusTopic.c_str(),"Включен энергосберегающий режим");}else{client.publish(statusTopic.c_str(),"Отключен энергосберегающий режим");}
                    }
    }      
    }

  void connectToMqtt() {
    while (!client.connected()) {
      Serial.print("Connecting to MQTT...");
      if (client.connect((clientID + ESP.getChipId()).c_str(), mqttUser.c_str(), mqttPassword.c_str(), statusTopic.c_str(), 1, true, "Клиент отключен")) {
        Serial.println("connected");

        // Подписка на топик
        client.subscribe(cmdTopic.c_str());
        client.publish(statusTopic.c_str(), "Клиент подключен");
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
      }
    }
  }

 //функция сохранения и применения настроек mqtt
  void handle_mqtt_save() {
    useMQTT = HTTP.arg("mq_on").toInt();                  // включение mqtt
    mqttServer = HTTP.arg("mq_ip");                       // адрес сервера
    mqttPort = HTTP.arg("mq_port").toInt();               // порт mqtt
    mqttUser = HTTP.arg("mq_ssid");                       // логин
    mqttPassword = HTTP.arg("mq_pass");                   // пароль
    clientID = HTTP.arg("mq_id");                         // id
    cmdTopic = HTTP.arg("mq_cmd");                        // командный топик;
    statusTopic = HTTP.arg("mq_status");                  // топик статуса;
    jsonWrite(mqttconfigJson, "mq_on", useMQTT);          // сохраняем в json
    jsonWrite(mqttconfigJson, "mq_ip", mqttServer);       //сохраняем в json
    jsonWrite(mqttconfigJson, "mq_port", mqttPort);       // сохраняем в json
    jsonWrite(mqttconfigJson, "mq_ssid", mqttUser);       //сохраняем в json
    jsonWrite(mqttconfigJson, "mq_pass", mqttPassword);   // сохраняем в json
    jsonWrite(mqttconfigJson, "mq_id", clientID);         //сохраняем в json
    jsonWrite(mqttconfigJson, "mq_cmd", cmdTopic);        //сохраняем в json
    jsonWrite(mqttconfigJson, "mq_status", statusTopic);  //сохраняем в json
    writeFile("mqtt_config.json", mqttconfigJson);        //сохраняем в фс
    //переподключаемся к серверу
    client.setServer(mqttServer.c_str(), mqttPort);
    connectToMqtt();
    HTTP.send(200, "text/plain", "OK");
  }

  void init_mqtt(){
    //чтение параметров mqtt
    mqttconfigJson = readFile("mqtt_config.json", 4096);
    Serial.println(mqttconfigJson);
    mqttServer = jsonRead(mqttconfigJson, "mq_ip");
    mqttPort = jsonReadtoInt(mqttconfigJson, "mq_port");
    mqttUser = jsonRead(mqttconfigJson, "mq_ssid");
    mqttPassword = jsonRead(mqttconfigJson, "mq_pass");
    clientID = jsonRead(mqttconfigJson, "mq_id");
    useMQTT = jsonReadtoInt(mqttconfigJson, "mq_on");
    cmdTopic = jsonRead(mqttconfigJson, "mq_cmd");
    statusTopic = jsonRead(mqttconfigJson, "mq_status");
    client.setServer(mqttServer.c_str(), mqttPort);
    client.setCallback(mqttCallback);
  }