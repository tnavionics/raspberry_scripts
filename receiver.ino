// 複数のESP32送信機からUDPデータを受信し、4つの値をCSV出力する受信機コード

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <math.h>

// --- Wi-Fi設定 (APモード用) ---
// このESP32が作成するアクセスポイントのSSIDとパスワード
const char *ssid = "ESP32_Batt_Monitor"; 
const char *password = "password123";   

// --- UDP設定 ---
const unsigned int localUdpPort = 4210; // 送信側とポートを合わせる
WiFiUDP Udp;
char incomingPacket[255];

// --- 送信機のIPアドレス定義 ---
// APモードではIPが192.168.4.xになります。送信機側もこの範囲で固定IPを設定する必要があります。
// 3要素CSVを送信するESP32のIPアドレス
const IPAddress SENDER_3CH_IP(192, 168, 4, 201); 
// 1要素CSVを送信するESP32のIPアドレス
const IPAddress SENDER_1CH_IP(192, 168, 4, 202); 

// --- データ格納用グローバル変数 ---
int adc1_0 = 0; 
int adc1_1 = 0; 
int adc1_2 = 0; 
int adc2_0 = 0;
float speed; 

// --- Setup 関数 ---
void setup() {
  Serial.begin(115200); 

  // ★ 変更点: APモード（ソフトアクセスポイント）で起動します
  // IPアドレスは自動的に 192.168.4.1 になります
  WiFi.softAP(ssid, password);
  
  Serial.println("\nWiFi AP Started.");
  Serial.print("Access Point SSID: ");
  Serial.println(ssid);
  Serial.print("Receiver (AP) IP Address: ");
  // APモードのIPアドレスは 192.168.4.1 です
  Serial.println(WiFi.softAPIP()); 

  // UDPポートの開始
  Udp.begin(localUdpPort);
  Serial.print("UDP listening on port: ");
  Serial.println(localUdpPort);
  
  while (!Serial) { ; }
  
  // 4要素のCSVヘッダーを出力
  Serial.println("ADC1_0,ADC1_1,ADC1_2,ADC2_0");
}

// --- Loop 関数 ---
void loop() {

  // UDPパケットを受信
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    // 送信元IPアドレスを取得
    IPAddress remoteIp = Udp.remoteIP();
    int len = Udp.read(incomingPacket, 255);
    
    if (len > 0) {
      incomingPacket[len] = '\0';
      String incomingStr = String(incomingPacket);

      // --- 1. 3要素CSV送信機からのデータかチェック ---
      if (remoteIp == SENDER_3CH_IP) {
        // 例: "1234,5678,9012"
        int firstComma = incomingStr.indexOf(',');
        int secondComma = incomingStr.indexOf(',', firstComma + 1);

        if (firstComma != -1 && secondComma != -1) {
          adc1_0 = incomingStr.substring(0, firstComma).toInt();
          adc1_1 = incomingStr.substring(firstComma + 1, secondComma).toInt();
          adc1_2 = incomingStr.substring(secondComma + 1).toInt();
          // デバッグ用
          // Serial.printf("3CH Received from %s: %d,%d,%d\n", remoteIp.toString().c_str(), adc1_0, adc1_1, adc1_2);
//          speed = (-0.6238 + sqrt(sq(0.6238)-4*(9.3222)*(14049-adc1_2)))/(2*9.3222);
        }
      } 
      // --- 2. 1要素CSV送信機からのデータかチェック ---
      else if (remoteIp == SENDER_1CH_IP) {
        // 例: "55555"
        adc2_0 = incomingStr.toInt();
        // デバッグ用
        // Serial.printf("1CH Received from %s: %d\n", remoteIp.toString().c_str(), adc2_0);
      }
      // 他のIPアドレスからのパケットは無視されます
    }
  }

  // --- CSV出力 ---
  // 受信があったかどうかに関わらず、現在の最新の4つの値をCSV形式で出力
  String dataString = 
      String(adc1_0) + "," +
      String(adc1_1) + "," +
      String(adc1_2) + "," +
      String(adc2_0);
      
  Serial.println(dataString);
//  Serial.println(speed); 

  // 受信頻度を確保するためのわずかな遅延
  delay(100); 
}
