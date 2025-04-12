#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <cmath>

using namespace sf;
using namespace std;
using json = nlohmann::json;

const int WIDTH = 400;
const int HEIGHT = 700;
const int GRID_W = 100;
const float TARGET_Y = HEIGHT - 150;
const float TOLERANCE = HEIGHT / 300.0f * 1000.0f;

struct Tile {
    int lane;
    float time;       // 開始時間（ms）
    float duration;   // 持續時間（ms）
    float y;
    bool active;
    bool triggered;
};

vector<Tile> loadChart(const json& chart, float speed) {
    vector<Tile> tiles;
    const float minTileDuration = 300.0f; // 每一小段 tile 的持續時間 (ms)

    for (auto& note : chart) {
        float startTime = note["time"];
        int lane = note["lane"];
        float duration = note["duration"];

        if (duration <= minTileDuration) {
            // 普通短 tile，直接加入
            Tile tile;
            tile.time = startTime;
            tile.lane = lane;
            tile.duration = duration;
            tile.y = -tile.duration / 1000.0f * speed;
            tile.active = true;
            tile.triggered = false;
            tiles.push_back(tile);
        } else {
            // 長音：拆成多個 tile
            int numPieces = ceil(duration / minTileDuration);
            for (int i = 0; i < numPieces; ++i) {
                Tile tile;
                tile.time = startTime + i * minTileDuration;
                tile.lane = lane;
                tile.duration = minTileDuration;
                tile.y = -tile.duration / 1000.0f * speed;
                tile.active = true;
                tile.triggered = false;
                tiles.push_back(tile);
            }
        }
    }

    return tiles;
}

float calculateSpeedFromChart(const vector<Tile>& chart, float targetY) {
    if (chart.empty()) return 300.0f; // fallback

    float appearTime = chart[0].time;         // 第一顆 note 的時間（ms）
    float travelTime = 2000.0f;               // 我希望它花 2 秒掉下來（你可以改這個值）
    float speed = targetY / (travelTime / 1000.0f); // px/sec
    return speed;
}

bool handleKeyPress(int lanePressed, vector<Tile>& tiles, float currentTime, int& score) {
    printf("clicked\n");
    for (auto& tile : tiles) {
        if (!tile.active || tile.triggered || tile.lane != lanePressed)
            continue;
        float diff = currentTime - tile.time;

        if (abs(diff) <= TOLERANCE) {
            tile.active = false;
            tile.triggered = true;
            score++;
            return true;  // 成功命中
        } else if (diff > TOLERANCE) {
            // 錯過的 tile 已經太久了，應該記 miss，不處理
            tile.active = false;
            tile.triggered = true;
            return false;  // miss（因為你按到太晚的 tile）
        }
    }

    // 沒有可按的 tile，也不當作 miss（提前按）
    return true;
}


int main() {
    RenderWindow window(VideoMode(WIDTH, HEIGHT), "別踩白塊兒");
    window.setFramerateLimit(60);

    // 字型與分數文字
    Font font;
    if (!font.loadFromFile("Arial.ttf")) {
        cerr << "無法載入字型\n";
        return -1;
    }
    Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(30);
    scoreText.setFillColor(Color::Black);
    scoreText.setPosition(WIDTH / 2 - 50, 10);

    // 音樂
    Music music;
    if (!music.openFromFile("canon.ogg")) {
        cerr << "無法載入音樂\n";
        return -1;
    }

    // 譜面資料
    ifstream file("canon.json");
    json chartJson;
    file >> chartJson;

    // 計算 SPEED
    float appearTime = chartJson[0]["time"];
    float travelTime = 2000.0f; // 你希望 tile 要花幾毫秒下來（例如 2 秒）
    float SPEED = TARGET_Y / (travelTime / 1000.0f);  // px/sec

    // 用算出來的 SPEED 載入 chart
    vector<Tile> chart = loadChart(chartJson, SPEED);
    vector<Tile> activeTiles;

    Clock globalClock;
    int score = 0;
    size_t chartIndex = 0;
    bool gameOver = false;

    // 提前播放 offset，讓第一個 tile 出現在畫面中間
    float startOffset = chart[0].time - (TARGET_Y / SPEED * 1000.0f);
    if (startOffset < 0) startOffset = 0; // 防止小於 0
    music.setPlayingOffset(milliseconds(static_cast<int>(startOffset)));
    music.play();
    bool started = true;

    map<Keyboard::Key, Clock> keyCooldown;
    float cooldownMs = 120;

    while (window.isOpen()) {
        float currentTime = music.getPlayingOffset().asMilliseconds() + startOffset;

        Event event;
        // if (gameOver) {
        //     return 0;
        // }
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();
            
            // 控制鍵（A/F/H/L）
            if (event.type == Event::KeyPressed && !gameOver) {
                // 取得這個鍵的 cooldown 時間
                if (keyCooldown[event.key.code].getElapsedTime().asMilliseconds() > cooldownMs) {
                    keyCooldown[event.key.code].restart(); // 重設 cooldown
            
                    bool success = true;
                    if (event.key.code == Keyboard::A) success = handleKeyPress(0, activeTiles, currentTime, score);
                    if (event.key.code == Keyboard::F) success = handleKeyPress(1, activeTiles, currentTime, score);
                    if (event.key.code == Keyboard::H) success = handleKeyPress(2, activeTiles, currentTime, score);
                    if (event.key.code == Keyboard::L) success = handleKeyPress(3, activeTiles, currentTime, score);
            
                    if (!success) {
                        cout << "Game Over!" << endl;
                        gameOver = true;
                        music.stop();
                    }
                }
            }
        }

        // 推進譜面到 activeTiles
        while (chartIndex < chart.size() && chart[chartIndex].time <= currentTime + 1500) {
            activeTiles.push_back(chart[chartIndex]);
            chartIndex++;
        }

        // 更新 tile Y
        for (auto& tile : activeTiles) {
            float elapsed = currentTime - tile.time;
            tile.y = (elapsed / 1000.0f * SPEED);  // 計算當前的垂直位置
        }

        // 清理錯過的 tile
        for (auto& tile : activeTiles) {
            if (tile.y > HEIGHT && tile.active) {  // 如果 tile 超過了畫面底部
                tile.active = false;  // 設置為不活躍
                tile.triggered = true;  // 標記為已錯過
                cout << "Missed tile at lane " << tile.lane << endl;
            }
        }

        // 更新文字
        scoreText.setString("Score: " + to_string(score));

        // 畫面更新
        window.clear(Color::White);
        window.draw(scoreText);

        // 判定線
        RectangleShape line(Vector2f(WIDTH, 2));
        line.setPosition(0, HEIGHT);
        line.setFillColor(Color(200, 0, 0));
        window.draw(line);

        // 畫 tile
        float fixedTileHeight = 150;
        for (auto& tile : activeTiles) {
            if (!tile.active && !tile.triggered) continue;

            // 算出總高度
            float totalHeight = tile.duration / 1000.0f * SPEED;
            if (totalHeight < fixedTileHeight) totalHeight = fixedTileHeight;

            // 算要畫幾個 tile（至少畫 1 個）
            int numTiles = ceil(totalHeight / fixedTileHeight);

            // 起始位置
            float baseY = tile.y - totalHeight;

            for (int i = 0; i < numTiles; ++i) {
                RectangleShape rect(Vector2f(GRID_W, fixedTileHeight));
                rect.setPosition(tile.lane * GRID_W, baseY + i * fixedTileHeight);

                if (tile.triggered) {
                    rect.setFillColor(Color(128, 128, 128)); // 灰色
                } else {
                    rect.setFillColor(Color::Black); // 尚未按的黑色
                }

                window.draw(rect);
            }
        }        

        window.display();
    }

    return 0;
}
