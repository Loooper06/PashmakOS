#include <Arduino.h>
#include "app/PashmakApp.h"
#include "web/WebDashboard.h"

PashmakApp app;
WebDashboard dashboard;

void setup()
{
  Serial.begin(115200);
  app.begin();
  dashboard.begin(app);
}

void loop()
{
  app.update();
  dashboard.update();
}
