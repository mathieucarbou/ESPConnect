#include <MycilaESPConnect.h>

AsyncWebServer server(80);

ESPConnectConfig config;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  // serve your logo here
  server.on("/logo", HTTP_GET, [&](AsyncWebServerRequest* request) {
    // AsyncWebServerResponse* response = request->beginResponse_P(200, "image/png", _binary_data_logo_icon_png_gz_start, _binary_data_logo_icon_png_gz_end - _binary_data_logo_icon_png_gz_start);
    AsyncWebServerResponse* response = request->beginResponse_P(200, "image/png", "", 0);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "public, max-age=900");
    request->send(response);
  });

  // serve your home page here
  server.on("/home", HTTP_GET, [&](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Hello World!");
  });

  // clear persisted config
  server.on("/clear", HTTP_GET, [&](AsyncWebServerRequest* request) {
    Serial.println("Clearing configuration...");
    ESPConnect.clearConfiguration();
    request->send(200);
  });

  // add a rewrite which is only applicable in AP mode and STA mode, but not in Captive Portal mode
  server.rewrite("/", "/home")
    .setFilter([](AsyncWebServerRequest* request) { return ESPConnect.getState() != ESPConnectState::PORTAL_STARTED; });

  // network state listener is required here in async mode
  ESPConnect.listen([](ESPConnectState previous, ESPConnectState state) {
    JsonDocument doc;
    ESPConnect.toJson(doc.to<JsonObject>());
    serializeJsonPretty(doc, Serial);
    Serial.println();

    switch (state) {
      case ESPConnectState::NETWORK_CONNECTED:
      case ESPConnectState::AP_STARTED:
        server.begin();
        break;

      case ESPConnectState::NETWORK_DISCONNECTED:
        server.end();
        break;

      case ESPConnectState::PORTAL_COMPLETE:
        config.apMode = ESPConnect.hasConfiguredAPMode();
        if (config.apMode) {
          Serial.println("====> Captive Portal: Access Point configured");
        } else {
          Serial.println("====> Captive Portal: WiFi configured");
          config.wifiSSID = ESPConnect.getConfiguredWiFiSSID();
          config.wifiPassword = ESPConnect.getConfiguredWiFiPassword();
        }
        // SAVE config somewhere
        break;

      default:
        break;
    }
  });

  ESPConnect.setAutoRestart(true);
  ESPConnect.setBlocking(false);
  ESPConnect.setCaptivePortalTimeout(180);
  ESPConnect.setConnectTimeout(10);

  Serial.println("====> Load config from elsewhere...");
  config.wifiSSID = "arduino";
  config = {
    .wifiSSID = "IoT",
    .wifiPassword = "",
    .apMode = false};

  Serial.println("====> Trying to connect to saved WiFi or will start captive portal in the background...");
  ESPConnect.begin(&server, "arduino", "Captive Portal SSID", "", config);

  Serial.println("====> setup() completed...");
}

void loop() {
  ESPConnect.loop();
}
