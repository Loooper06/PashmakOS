#pragma once

#include <Arduino.h>
#include <WebServer.h>

class PashmakApp;

class WebDashboard
{
public:
    void begin(PashmakApp &app);
    void update();

private:
    WebServer _server{80};
    PashmakApp *_app = nullptr;
    bool _started = false;

    void handleRoot();
    void handleStats();
    void handlePetSelection();
    void handlePetInteraction();
    void handleTimerSettingsGet();
    void handleTimerSettingsPost();
    void handleQuietHoursGet();
    void handleQuietHoursPost();
    void handleReminderSettingsGet();
    void handleReminderSettingsPost();
    void handleDiagnostics();
    void handleWifiSettingsGet();
    void handleWifiSettingsPost();
    void handleNotFound();
};
