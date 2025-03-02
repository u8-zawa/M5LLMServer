/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <M5Unified.h>
#include <M5ModuleLLM.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"

WebServer server(80);

M5ModuleLLM module_llm;
String llm_work_id;
String current_result;
bool processing = false;
const unsigned long CONNECTION_TIMEOUT = 30000;  // 30 seconds timeout

void handleRoot() {
  server.send(200, "text/plain", "M5Stack LLM Server");
}

void handleAsk() {
  // GETリクエストのみを受け付ける
  if (server.method() != HTTP_GET) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  // 処理中の場合は拒否
  if (processing) {
    server.send(429, "text/plain", "Server is busy processing another request");
    return;
  }

  // クエリパラメータから質問を取得
  String question = server.arg("question");
  if (question.length() == 0) {
    server.send(400, "text/plain", "Question cannot be empty");
    return;
  }

  // 処理中フラグを立てる
  processing = true;
  current_result = "";

  // LLMモジュールで処理
  module_llm.llm.inferenceAndWaitResult(llm_work_id, question.c_str(), [](String& result) {
    M5.Display.printf("%s", result.c_str());
    current_result = result;
  });
  M5.Display.println();

  // 処理完了
  processing = false;

  // server.send(200, "text/plain", "handleAsk: complete");
  server.send(200, "text/plain", "handleAsk: " + current_result);
}

void handleNotFound() {
  String message = "Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  M5.begin();
  M5.Display.setTextSize(2);
  M5.Display.setTextScroll(true);

  // Initialize Module LLM
  // Serial2.begin(115200, SERIAL_8N1, 16, 17);  // Basic
  Serial2.begin(115200, SERIAL_8N1, 13, 14);  // Core2
  // Serial2.begin(115200, SERIAL_8N1, 18, 17);  // CoreS3
  module_llm.begin(&Serial2);

  M5.Display.printf(">> Check ModuleLLM connection..\n");
  while (1) {
    if (module_llm.checkConnection()) {
      break;
    }
  }

  M5.Display.printf(">> Reset ModuleLLM..\n");
  module_llm.sys.reset();

  M5.Display.printf(">> Setup LLM..\n");
  m5_module_llm::ApiLlmSetupConfig_t llm_config;
  llm_config.max_token_len = 1023;
  llm_work_id = module_llm.llm.setup(llm_config);

  // Setup WiFi
  M5.Display.printf(">> Connecting to WiFi..\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Display.print(".");
  }

  M5.Display.printf("\nConnected!\nIP: %s\n", WiFi.localIP().toString().c_str());

  if (MDNS.begin("m5llm")) {
    M5.Display.println("MDNS responder started");
  }

  // Setup server routes
  server.on("/", handleRoot);
  server.on("/ask", handleAsk);
  server.onNotFound(handleNotFound);

  server.begin();
  M5.Display.println("HTTP server started");
  M5.Display.println("Ready to accept requests!");
  M5.Display.println();
}

void loop() {
  server.handleClient();
  delay(2);
}