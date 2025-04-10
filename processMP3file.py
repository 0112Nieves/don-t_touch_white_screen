import librosa
import json
import random

file_base = input("請輸入音樂檔名（不含 .mp3）：")
filename = file_base + ".mp3"

y, sr = librosa.load(filename)
onset_frames = librosa.onset.onset_detect(y=y, sr=sr, units='frames', backtrack=True)
onset_times = librosa.frames_to_time(onset_frames, sr=sr)

lane_count = 4
chart = []

for i in range(len(onset_times)):
    time = onset_times[i]
    millis_time = int(time * 1000)

    if i < len(onset_times) - 1:
        duration = int((onset_times[i + 1] - time) * 1000)
    else:
        duration = 500  # 最後一個預設

    chart.append({
        "time": millis_time,
        "lane": random.randint(0, lane_count - 1),
        "duration": duration
    })

with open(file_base + ".json", "w") as f:
    json.dump(chart, f, indent=2)

print(f"✅ 已輸出譜面檔：{file_base}.json")
