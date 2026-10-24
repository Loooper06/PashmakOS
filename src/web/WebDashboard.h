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
    void handleTimerStatusGet();
    void handleTimerControlPost();
    void handleTimerSettingsGet();
    void handleTimerSettingsPost();
    void handleQuietHoursGet();
    void handleQuietHoursPost();
    void handleReminderSettingsGet();
    void handleReminderSettingsPost();
    void handleEventLog();
    void handleDiagnostics();
    void handleDashboardSettingsGet();
    void handleDashboardSettingsPost();
    void handleWifiSettingsGet();
    void handleWifiSettingsPost();
    void handleOtaUpload();
    void handleOtaComplete();
    bool _otaUploadStarted = false;
    bool _otaUploadFailed = false;
    size_t _otaUploadBytes = 0;
    void handleNotFound();
};
