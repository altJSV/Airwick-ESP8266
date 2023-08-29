void outData(){
    jsonWrite(configJson, "time", GetTime());
    jsonWrite(configJson, "gpio0", digitalRead(0));
    jsonWrite(configJson, "A0", analogRead(A0));
    jsonWrite(configJson, "interval", timerDuration/60000); 
    jsonWrite(configJson, "pretimer", preTimer/60000);
  }

void GRAF_init() {
  HTTP.on("/analog.json", HTTP_GET, []() {
    String data = graf(analogRead(A0),20,10000);
    HTTP.send(200, "application/json", data);
  });
}