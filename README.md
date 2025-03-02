# M5LLMServer

M5StackとLLMモジュールを使用して、HTTPリクエストでLLMと対話できるサーバーを構築するプロジェクトです。

## 概要

このプロジェクトは、M5Stack上でWebサーバーを実行し、LLMモジュールとの通信を可能にします。HTTP GETリクエストを通じて質問を送信し、LLMからの回答を受け取ることができます。

### 主な機能

- WiFi経由でアクセス可能なWebサーバー
- シンプルなHTTP APIインターフェース
- タイムアウト処理による安定した応答
- mDNSによる簡単なアクセス

## 必要なハードウェア

- M5Stack本体（Basic/Core2/CoreS3のいずれか）
- M5Stack用LLMモジュール
- WiFi環境

## セットアップ手順

1. **ハードウェアの接続**
   - LLMモジュールをM5Stackに接続します
   - USB経由で電源を供給します

2. **設定ファイルの準備**
   ```cpp
   // config.cpp
   const char* ssid = "your-wifi-ssid";
   const char* password = "your-wifi-password";
   ```

3. **ピン設定の確認**
   使用するM5Stackモデルに応じて、適切なシリアル通信ピンを選択してください：
   ```cpp
   // Basic
   Serial2.begin(115200, SERIAL_8N1, 16, 17);
   // Core2
   Serial2.begin(115200, SERIAL_8N1, 13, 14);
   // CoreS3
   Serial2.begin(115200, SERIAL_8N1, 18, 17);
   ```

4. **コードのコンパイルとアップロード**
   - Arduino IDEまたはPlatformIOを使用してコードをコンパイルし、M5Stackにアップロードします
   - 正常に起動すると、ディスプレイにIPアドレスが表示されます

## 使用方法

### APIエンドポイント

1. **ルートエンドポイント（/）**
   - メソッド: GET
   - 説明: サーバーの動作確認用
   - レスポンス: "M5Stack LLM Server"

2. **質問エンドポイント（/ask）**
   - メソッド: GET
   - パラメータ: question（質問文）
   - 説明: LLMに質問を送信し、回答を取得
   - レスポンス: LLMからの回答テキスト
   - タイムアウト: 60秒

### 使用例

```bash
# サーバーの動作確認
curl "http://m5llm.local/"

# LLMへの質問
curl "http://m5llm.local/ask?question=What%20is%20the%20capital%20of%20Japan?"
```

## エラーレスポンス

- 400: 質問が空の場合
- 404: 存在しないエンドポイントにアクセスした場合
- 405: GETメソッド以外でアクセスした場合
- 408: 処理がタイムアウトした場合（60秒）
- 429: 別のリクエストを処理中の場合

## トラブルシューティング

1. **WiFiに接続できない場合**
   - WiFiの認証情報（SSIDとパスワード）が正しいか確認
   - M5Stackが WiFiルーターの範囲内にあるか確認

2. **LLMモジュールが認識されない場合**
   - モジュールが正しく接続されているか確認
   - シリアル通信ピンの設定が使用しているM5Stackモデルと一致しているか確認

3. **応答が遅い/タイムアウトする場合**
   - より短い質問を試してみる
   - WiFi接続状態を確認
   - LLMモジュールとの接続を確認

## ライセンス

[MIT License](LICENSE)に基づいて公開されています。

Copyright (c) 2025 Yuya Aizawa
