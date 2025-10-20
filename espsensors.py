import socket
import time
import sys

# --- UDP設定 ---
LOCAL_UDP_PORT = 4210  # ESP32と同じポート
BUFFER_SIZE = 255      # 受信バッファサイズ
# Raspberry PiのIPアドレス (APモードでは192.168.4.1になることが多いですが、
# '0.0.0.0' に設定することで全てのインターフェースでリッスンします)
LOCAL_IP = '0.0.0.0'

# --- 送信機のIPアドレス定義 ---
# ESP32のIPアドレスは、送信機側で固定IPとして設定されている必要があります。
SENDER_3CH_IP = '192.168.4.201' 
SENDER_1CH_IP = '192.168.4.202'

# --- データ格納用グローバル変数 ---
adc1_0 = 0
adc1_1 = 0
adc1_2 = 0
adc2_0 = 0

# --- Setup 関数 ---
def setup():
    """初期設定とUDPソケットの作成"""
    global udp_socket
    
    print("Raspberry Pi UDP Receiver Startup.")
    
    # UDPソケットを作成 (IPv4, UDP)
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    try:
        # 指定したポートでリッスンを開始
        udp_socket.bind((LOCAL_IP, LOCAL_UDP_PORT))
        
        print(f"UDP listening on IP: {LOCAL_IP}, Port: {LOCAL_UDP_PORT}")
        
    except OSError as e:
        print(f"[ERROR] Failed to bind socket: {e}")
        print("Please check if the port is already in use or if you have permissions.")
        sys.exit(1)

    # 4要素のCSVヘッダーを出力
    print("ADC1_0,ADC1_1,ADC1_2,ADC2_0")

# --- Loop 関数 ---
def loop():
    """UDPパケットの受信、解析、CSV出力のメインループ"""
    global adc1_0, adc1_1, adc1_2, adc2_0
    
    # ソケットが閉じていないかチェック
    if 'udp_socket' not in globals():
        return

    try:
        # ノンブロッキングモードでUDPパケットを受信
        data, addr = udp_socket.recvfrom(BUFFER_SIZE)
        
        # 受信データがあれば処理
        if data:
            # 送信元IPアドレスを取得
            remote_ip = addr[0]
            
            # バイトデータを文字列にデコード (UTF-8)
            incoming_str = data.decode('utf-8').strip()
            
            # --- 1. 3要素CSV送信機からのデータかチェック ---
            if remote_ip == SENDER_3CH_IP:
                parts = incoming_str.split(',')
                if len(parts) == 3:
                    try:
                        adc1_0 = int(parts[0])
                        adc1_1 = int(parts[1])
                        adc1_2 = int(parts[2])
                        # デバッグ用
                        # print(f"3CH Received from {remote_ip}: {adc1_0},{adc1_1},{adc1_2}", file=sys.stderr)
                    except ValueError:
                        # 整数に変換できないデータは無視
                        pass
                        
            # --- 2. 1要素CSV送信機からのデータかチェック ---
            elif remote_ip == SENDER_1CH_IP:
                try:
                    adc2_0 = int(incoming_str)
                    # デバッグ用
                    # print(f"1CH Received from {remote_ip}: {adc2_0}", file=sys.stderr)
                except ValueError:
                    # 整数に変換できないデータは無視
                    pass
            
            # 他のIPアドレスからのパケットは無視されます
            
    except BlockingIOError:
        # パケットが来ていない場合は何もしない (すぐに次の処理へ進む)
        pass
    except Exception as e:
        # 予期せぬエラー
        # print(f"[ERROR] An unexpected error occurred: {e}", file=sys.stderr)
        pass

    # --- CSV出力 ---
    # 受信があったかどうかに関わらず、現在の最新の4つの値をCSV形式で出力
    data_string = f"{adc1_0},{adc1_1},{adc1_2},{adc2_0}"
    
    print(data_string)
    sys.stdout.flush() # 即座にコンソールに書き出す

    # 受信頻度を確保するためのわずかな遅延 (ESP32側の delay(100) に合わせる)
    time.sleep(0.1)

# --- メイン処理 ---
if __name__ == "__main__":
    setup()
    
    # ソケットをノンブロッキングに設定
    # これにより、パケットがない場合でも loop()が遅延なく実行されます
    udp_socket.setblocking(0)

    try:
        while True:
            loop()
    except KeyboardInterrupt:
        print("\n--- Script terminated by user (Ctrl+C) ---")
    finally:
        if 'udp_socket' in globals():
            udp_socket.close()
            print("UDP socket closed.")
