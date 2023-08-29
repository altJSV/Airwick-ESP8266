void outData(){
    jsonWrite(configJson, "time", GetTime());
    jsonWrite(configJson, "gpio0", digitalRead(0));
    jsonWrite(configJson, "A0", analogRead(A0));
    jsonWrite(configJson, "interval", timerDuration/60000); 
    jsonWrite(configJson, "pretimer", preTimer/60000);
  }
