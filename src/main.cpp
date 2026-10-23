
#include <Arduino.h>
#include "app/PashmakApp.h"

PashmakApp app;

void setup()
{
  Serial.begin(115200);
  app.begin();
}

void loop()
{
  app.update();
}
