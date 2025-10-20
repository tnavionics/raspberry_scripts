// ADS1115 (3ch ADC) のデジタル値をUDPでCSV形式で送信するESP32-C3コード

#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_ADS1X15.h>
#include <Arduino.h>

// ★★★ ADS1115のインスタンスを作成 ★★★
Adafruit_ADS1115 ads; 

// --- Wi-Fi設定 ---
// 受信機APと同じ設定にしてください
const char *ssid = "ESP32_Batt_Monitor"; 
const char *password = "password123"; 

// --- UDP設定 ---
WiFiUDP Udp;
// ★ 修正: 送信先は受信機APのIPアドレス (常に 192.168.4.1)
const IPAddress remoteIP(192, 168, 4, 1); 
const unsigned int remotePort = 4210; 

// --- 送信機自身の静的IPアドレス ---
// ★ 修正: 受信機側でSENDER_3CH_IPとして定義されている IP (192.168.4.201)
const IPAddress SENDER_IP(192, 168, 4, 201);
// ★ 修正: ゲートウェイは受信機APのIP (192.168.4.1)
const IPAddress GATEWAY(192, 168, 4, 1);
// ★ 修正: サブネットマスクはAPモードの標準 (255.255.255.0)
const IPAddress SUBNET(255, 255, 255, 0);

// データを送信するためのタイマー変数
unsigned long lastSendTime = 0;
// 100msごとにデータを送信
unsigned long sendInterval = 100; 

void setup() {
  Serial.begin(115200); 
  delay(100);

  // --- 1. 静的IPでのWi-Fi接続 (IPアドレスを固定) ---
  Serial.print("Setting static IP: ");
  Serial.println(SENDER_IP);
  // APモードのネットワーク設定に合わせて静的IPを設定
  WiFi.config(SENDER_IP, GATEWAY, SUBNET);

  Serial.print("Connecting to AP: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("Local IP Address (SENDER_3CH_IP): ");
  Serial.println(WiFi.localIP());

  // --- 2. UDPポートの開始 ---
  // クライアントモードなので、ここではポートリスニングは必須ではありませんが、一応開始しておきます
//  Udp.begin(localUdpPort);
  
  // ------------------------------------
  // 3. ADS1115の初期化
  // ------------------------------------
  Wire.begin(); // I2C通信を開始
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115.");
    while (1);
  }

  // ADS1115設定確認メッセージ
  Serial.println("ADS1115 initialized with default GAIN_TWOTHIRDS (+/- 6.144V)");
  Serial.println("Setup Complete. Starting UDP transmission...");
}

void loop() {
  // 指定した間隔でADC値を送信
  if (millis() - lastSendTime >= sendInterval) {
    // 1. ADS1115からデジタル値を取得
    // AIN0, AIN1, AIN2の3チャンネルを読み取ります
    int16_t adc0 = ads.readADC_SingleEnded(0);
    int16_t adc1 = ads.readADC_SingleEnded(1);
    int16_t adc2 = ads.readADC_SingleEnded(2);
    
    // 2. CSV形式の文字列を生成: "adc0,adc1,adc2"
    String dataToSend = 
        String(adc0) + "," + 
        String(adc1) + "," + 
        String(adc2);
    
    // 3. 送信先にデータを送信
    Udp.beginPacket(remoteIP, remotePort);
    Udp.print(dataToSend); 
    Udp.endPacket();

    // シリアルモニタに表示
    Serial.print("3CH Sent to ");
    Serial.print(remoteIP);
    Serial.print(": ");
    Serial.println(dataToSend);
    
    lastSendTime = millis();
  }
}
