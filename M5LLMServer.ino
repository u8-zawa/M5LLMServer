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
bool timeout_occurred = false;
TaskHandle_t timeout_task_handle = NULL;

void timeoutCheckTask(void* parameter) {
  unsigned long start_time = millis();

  while (processing) {
    if (millis() - start_time > LLM_PROCESSING_TIMEOUT) {
      // タイムアウト発生
      timeout_occurred = true;
      processing = false;
      current_result = "Processing timeout. Please try with a simpler question.";
      M5.Display.println("\nTimeout occurred!");
      server.send(408, "text/plain", "Processing timeout. Please try with a simpler question.");
      break;
    }
    delay(1000);
  }

  timeout_task_handle = NULL;
  vTaskDelete(NULL);
}

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
  timeout_occurred = false;
  current_result = "";

  // タイムアウト監視タスクを開始
  xTaskCreate(
    timeoutCheckTask,
    "timeoutTask",
    4096,
    NULL,
    1,
    &timeout_task_handle);

  // LLMモジュールで処理
  module_llm.llm.inferenceAndWaitResult(llm_work_id, question.c_str(), [](String& result) {
    if (!timeout_occurred) {  // タイムアウトが発生していない場合のみ結果を設定
      M5.Display.printf("%s", result.c_str());
      current_result = result;
    }
  });
  M5.Display.println();

  processing = false;

  // タイムアウトタスクが実行中の場合は削除
  if (timeout_task_handle != NULL) {
    vTaskDelete(timeout_task_handle);
    timeout_task_handle = NULL;
  }

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

  // LLMモジュールの初期化
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

  // WiFiの設定
  M5.Display.printf(">> Connecting to WiFi..\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptTime = millis();
  bool connectionSuccess = false;

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttemptTime > CONNECTION_TIMEOUT) {
      M5.Display.println("\nWiFi connection timeout!");
      break;
    }
    delay(500);
    M5.Display.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    connectionSuccess = true;
    M5.Display.printf("\nConnected!\nIP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    M5.Display.println("\nFailed to connect to WiFi!");
    M5.Display.println("Please check your WiFi credentials");
    M5.Display.println("or network availability.");
    // WiFiをオフにして電力消費を抑える
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    return;  // セットアップを中止
  }

  if (MDNS.begin("m5llm")) {
    M5.Display.println("MDNS responder started");
  }

  // ルーティング設定
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