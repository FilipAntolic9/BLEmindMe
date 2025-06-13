import paho.mqtt.client as mqtt
import json
import threading
import time
import random

BROKER = "161.53.133.253"
PORT = 1883
TOPIC = "v1/devices/me/attributes"
CLIENT_ID = "ESP2"
USERNAME = "ESP2"
PASSWORD = "password2"
TOPIC_STATE_UPDATE = f"v1/devices/esp/{USERNAME}/stateUpdate"
TOPIC_USER_LEFT = f"v1/devices/esp/{USERNAME}/userLeft"

known_devices = [
    {
        "deviceId": "f4333f80-4568-11f0-a544-db21b46190ed",
        "deviceName": "Keys"
    },
    {
        "deviceId": "9d7805f0-4224-11f0-a544-db21b46190ed",
        "deviceName": "Wallet"
    }
]

last_distance = 60.0
can_detect_exit = True

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        if f"lightState_{USERNAME}" in payload:
            state = payload[f"lightState_{USERNAME}"]
            print("💡 LED ON" if state else "💡 LED OFF")
        else:
            print(f"\nPrimljena poruka: {payload}")
    except Exception as e:
        print("Greška u obradi poruke:", e)

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✅ ESP2 povezan na broker. Pretplaćujem se na topic...")
        client.subscribe(TOPIC)
        threading.Thread(target=send_state_update, daemon=True).start()
        threading.Thread(target=simulate_hc_sr04, daemon=True).start()
    else:
        print(f"❌ Greška pri spajanju. Kod: {rc}")

def send_state_update():
    while True:
        devices = []
        for dev in known_devices:
            present = random.choice([True, False])
            distance = round(random.uniform(0.5, 3.0), 2)
            devices.append({
                "deviceId": dev["deviceId"],
                "deviceName": dev["deviceName"],
                "distance": distance,
                "present": present
            })
        payload = json.dumps({"devices": devices})
        client.publish(TOPIC_STATE_UPDATE, payload)
        print(f"\nPoslan stateUpdate:\n{payload}")
        time.sleep(10)

def simulate_hc_sr04():
    global last_distance, can_detect_exit

    while True:
        new_distance = round(random.uniform(20.0, 100.0), 2)
        if can_detect_exit and abs(new_distance - last_distance) > 30:
            print("Detektiran izlazak korisnika.")
            send_user_left()
            can_detect_exit = False
            threading.Timer(10, reset_detection).start()
        #else:
        #    print("Nije detektiran izlazak korisnika.")
        last_distance = new_distance
        time.sleep(0.5)

def send_user_left():
    payload = json.dumps({"hasLeft": True})
    client.publish(TOPIC_USER_LEFT, payload)
    print(f"📤 Poslan userLeft:\n{payload}")

def reset_detection():
    global can_detect_exit
    can_detect_exit = True
    print("🔄 Detekcija ponovo omogućena.")

client = mqtt.Client(client_id=CLIENT_ID)
client.username_pw_set(USERNAME, PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

print("🔌 Povezivanje kao ESP2...")
client.connect(BROKER, PORT, 60)
client.loop_forever()
