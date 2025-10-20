import serial
import pynmea2
import time

# --- 設定 ---
# GPSモジュールが接続されているシリアルポート
# (PiのGPIOに接続している場合は /dev/serial0 または /dev/ttyS0)
# (USB-TTLアダプタで接続している場合は /dev/ttyACM0 や /dev/ttyUSB0 など)
SERIAL_PORT = '/dev/serial0' 
# MAX-M10のデフォルトボーレート（異なる場合は変更してください）
BAUD_RATE = 38400

def get_gps_coordinates():
    try:
        # シリアルポートを開く
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"シリアルポート {SERIAL_PORT} を {BAUD_RATE} bpsで開きました...")
        
        while True:
            # 1行読み取り（NMEAセンテンス）
            line = ser.readline().decode('utf-8', errors='ignore').strip()

            if line.startswith('$'):
                try:
                    # NMEAセンテンスを解析
                    msg = pynmea2.parse(line)

                    # GPGGA (GPS Fix Data) または GNRMC (Recommended Minimum) センテンスから情報を取得
                    if msg.sentence_type in ['GGA', 'RMC']:
                        # 有効な測位データがあるかチェック (RMCのステータス='A' または GGAのフィックス='1'/'2')
                        if (msg.sentence_type == 'RMC' and msg.status == 'A') or \
                           (msg.sentence_type == 'GGA' and msg.gps_qual in [1, 2]):
                            
                            # 緯度と経度を出力
                            latitude = msg.latitude
                            longitude = msg.longitude
                            
                            print(f"----------------------------------------")
                            print(f"緯度 (Latitude): {latitude:.6f} {msg.lat_dir}")
                            print(f"経度 (Longitude): {longitude:.6f} {msg.lon_dir}")
                            
                            # GPGGAであれば高度も取得可能
                            if msg.sentence_type == 'GGA' and msg.altitude is not None:
                                print(f"高度 (Altitude): {msg.altitude} {msg.altitude_units}")
                            print(f"最終更新時刻: {msg.timestamp}")
                            
                            return latitude, longitude # 取得できたら関数を終了
                            
                except pynmea2.ParseError as e:
                    # print(f"NMEA解析エラー: {e}")
                    continue
                
            time.sleep(0.1) # 読み取り間隔

    except serial.SerialException as e:
        print(f"シリアルポートのエラー: {e}")
        print("ポート名が正しいか、他のプロセス (gpsdなど) がポートを使用していないか確認してください。")
    except KeyboardInterrupt:
        print("\nプログラムを終了します。")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    get_gps_coordinates()
