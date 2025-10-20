// MCP3421 (1ch ADC) のデジタル値をUDPで単一値として送信するESP32-C3コード

#include <Wire.h>
#include <Adafruit_MCP3421.h> 
#include <WiFi.h>
#include <WiFiUdp.h>

// --- Wi-Fi設定 ---
// 受信機APと同じ設定にしてください
const char* ssid = "ESP32_Batt_Monitor"; 
const char* password = "password123";

// --- UDP設定 ---
WiFiUDP Udp;
// ★ 修正: 受信機APのIPアドレス (常に 192.168.4.1)
const IPAddress remoteIP(192, 168, 4, 1); 
const unsigned int remotePort = 4210; 

// --- 送信機自身の静的IPアドレス ---
// ★ 修正: 受信機側でSENDER_1CH_IPとして定義されている IP (192.168.4.202)
const IPAddress SENDER_IP(192, 168, 4, 202);
// ★ 修正: ゲートウェイは受信機APのIP (192.168.4.1)
const IPAddress GATEWAY(192, 168, 4, 1);
// ★ 修正: サブネットマスクはAPモードの標準 (255.255.255.0)
const IPAddress SUBNET(255, 255, 255, 0);

// MCP3421のインスタンス
Adafruit_MCP3421 mcp;

void setup() {
  Serial.begin(115200);
  Wire.begin(); // I2C通信を開始

  // ★ 削除: while (!Serial) { delay(10); } ループを削除しました。
  // これにより、USB接続なしでコードがすぐに実行されます。

  // --- 静的IPでのWi-Fi接続 (IPアドレスを固定) ---
  // Serial.printは残しておくと、後でUSBを繋いだときに情報が見られて便利です。
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
  Serial.print("Local IP Address (SENDER_1CH_IP): ");
  Serial.println(WiFi.localIP());

  // --- MCP3421初期化 ---
  if (!mcp.begin(0x68, &Wire)) { 
    Serial.println("Failed to find MCP3421 chip. Check wiring/address.");
    // ADCが見つからない場合は、処理を停止します。
    while (1) {
      delay(10); 
    }
  }
  Serial.println("MCP3421 Found!");

  // Options: GAIN_1X, GAIN_2X, GAIN_4X, GAIN_8X
  mcp.setGain(GAIN_8X);
  mcp.setResolution(RESOLUTION_18_BIT); 
  // モードを連続測定 (Continuous) に設定
  mcp.setMode(MODE_CONTINUOUS); 

  Serial.println("ADC Setup Complete. Sending 1-Channel data...");
}

void loop() {
    // Check if MCP3421 has completed a conversion in continuous mode
    if (mcp.isReady()) {
        int32_t adcValue = mcp.readADC(); // Read ADC raw value
        
        // 単一のADC値を文字列に変換
        String dataString = String(adcValue);
        
        // --- UDP送信処理 ---
        Udp.beginPacket(remoteIP, remotePort);
        Udp.print(dataString); // 受信機が想定する単一値として送信
        Udp.endPacket();

        // シリアルモニタにも値を出力
        Serial.print("1CH Sent: ");
        Serial.println(dataString);
    }
    
    // CPU負荷を下げるため、わずかな遅延を入れる
    delay(10);
}
