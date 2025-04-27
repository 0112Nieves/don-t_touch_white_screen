import librosa
import json
import random

file_base = input("請輸入音樂檔名（不含 .mp3）：")
filename = "mp3/" + file_base + ".mp3"

y, sr = librosa.load(filename)

# 🎵 自動偵測 BPM！
tempo, _ = librosa.beat.beat_track(y=y, sr=sr)

# 依據 BPM 動態設定 MIN_GAP
# 每拍的時間（毫秒） = 60秒 / BPM * 1000
beat_duration_ms = 60000 / tempo

# 最小間隔設定成「1/2拍」比較合理
MIN_GAP = int(beat_duration_ms * 0.5)

onset_frames = librosa.onset.onset_detect(y=y, sr=sr, units='frames', backtrack=True)
onset_times = librosa.frames_to_time(onset_frames, sr=sr)

lane_count = 4
chart = []

last_lane = -1
lane_end_times = [0 for _ in range(lane_count)]

for i in range(len(onset_times)):
    time = onset_times[i]
    millis_time = int(time * 1000)

    if i < len(onset_times) - 1:
        duration = int((onset_times[i + 1] - time) * 1000)
    else:
        duration = 500

    # 這裡一樣，篩選不同 lane + 加上最小間隔
    possible_lanes = [l for l in range(lane_count) if l != last_lane and millis_time >= lane_end_times[l] + MIN_GAP]

    if not possible_lanes:
        print(f"⚠️ {millis_time} ms 找不到可用的 lane，略過")
        continue

    lane = random.choice(possible_lanes)
    last_lane = lane

    lane_end_times[lane] = millis_time + duration

    chart.append({
        "time": millis_time,
        "lane": lane,
        "duration": duration
    })

with open("json/" + file_base + ".json", "w") as f:
    json.dump(chart, f, indent=2)

print(f"✅ 已輸出譜面檔：{file_base}.json")
