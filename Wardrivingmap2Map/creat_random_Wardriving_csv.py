import pandas as pd
import random
from datetime import datetime, timedelta
import string

# === Configuration ===
NUM_ENTRIES = 100
USE_FIXED_LOCATION = False
BASE_LAT = 40.75
BASE_LON = -74.0
OFFSET_RANGE = 0.005  # random offset range: +/- this value
CSV_FILENAME = "random.csv"

def generate_random_data(num_entries=100, use_fixed_location=False, base_lat=40.75, base_lon=-74.0, offset_range=0.1):
    data = []
    base_time = datetime(2025, 5, 2, 12, 30)

    if use_fixed_location:
        lat = round(base_lat, 6)
        lon = round(base_lon, 6)

    for i in range(num_entries):
        current_time = base_time + timedelta(seconds=i * 30)

        if not use_fixed_location:
            lat = round(random.uniform(base_lat - offset_range, base_lat + offset_range), 6)
            lon = round(random.uniform(base_lon - offset_range, base_lon + offset_range), 6)

        altitude = round(random.uniform(20, 35), 1)
        satellites = random.randint(5, 10)
        Accuracy = random.randint(0, 7)

        ssid_pool = string.ascii_letters + string.digits
        ssid = ''.join(random.sample(ssid_pool, 10))
        bssid = ':'.join([f"{random.randint(0, 255):02X}" for _ in range(6)])
        channel = random.randint(1, 11)
        rssi = random.randint(-80, -30)
        encryption = random.choice(["WPA2", "WPA3", "WEP"])

        row = [
            current_time.strftime('%Y-%m-%d'),
            current_time.strftime('%H:%M:%S'),
            lat, lon, altitude, satellites, Accuracy,
            ssid, bssid, channel, rssi, encryption
        ]
        data.append(row)

    return pd.DataFrame(data, columns=[
        "Date", "Time_UTC", "Latitude", "Longitude", "Altitude",
        "Satellites", "Accuracy", "SSID", "BSSID", "Channel", "RSSI", "Encryption"
    ])


# === Generate and save CSV ===
df = generate_random_data(
    num_entries=NUM_ENTRIES,
    use_fixed_location=USE_FIXED_LOCATION,
    base_lat=BASE_LAT,
    base_lon=BASE_LON,
    offset_range=OFFSET_RANGE
)

df.to_csv(CSV_FILENAME, index=0)
print(f"Random data saved to '{CSV_FILENAME}'")

