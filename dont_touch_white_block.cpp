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
const float SPEED = 300.0f; // 每秒 pixel
const float TARGET_Y = HEIGHT - 150;
const float TOLERANCE = HEIGHT / SPEED * 1000.0f;

struct Tile {
    int lane;
    float time;       // 開始時間（ms）
    float duration;   // 持續時間（ms）
    float y;
    bool active;
    bool triggered;
};

vector<Tile> loadChart(const string& filename) {
    vector<Tile> tiles;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "無法讀取譜面檔案\n";
        return tiles;
    }

    json chart;
    file >> chart;

    for (auto& note : chart) {
        Tile tile;
        tile.time = note["time"];
        tile.lane = note["lane"];
        tile.duration = note["duration"];
        tile.active = true;
        tile.triggered = false;
        tile.y = -tile.duration / 1000.0f * SPEED;
        tiles.push_back(tile);
    }
    return tiles;
}

bool handleKeyPress(int lanePressed, vector<Tile>& tiles, float currentTime, int& score) {
    for (auto& tile : tiles) {
        if (!tile.active || tile.triggered || tile.lane != lanePressed)
            continue;

        float diff = currentTime - tile.time;
        cout << diff << endl;

        if (abs(diff) <= TOLERANCE) {
            tile.active = false;
            tile.triggered = true;
            score++;
            printf("success\n");
            return true;  // 成功命中
        } else if (diff > TOLERANCE) {
            // 錯過的 tile 已經太久了，應該記 miss，不處理
            tile.active = false;
            tile.triggered = true;
            return false;  // miss（因為你按到太晚的 tile）
        } else {
            // 還沒到時間，可能按早了，但我們不當作錯
            continue;
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
    vector<Tile> chart = loadChart("canon.json");
    vector<Tile> activeTiles;

    Clock globalClock;
    int score = 0;
    size_t chartIndex = 0;
    bool gameOver = false;

    music.play();

    while (window.isOpen()) {
        float currentTime = music.getPlayingOffset().asMilliseconds();

        Event event;
        // if (gameOver) {
        //     return 0;
        // }
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();
            
            // 控制鍵（A/F/H/L）
            if (event.type == Event::KeyPressed && !gameOver) {
                bool success = true;
                if (event.key.code == Keyboard::A) success = handleKeyPress(0, activeTiles, currentTime, score);
                if (event.key.code == Keyboard::F) success = handleKeyPress(1, activeTiles, currentTime, score);
                if (event.key.code == Keyboard::H) success = handleKeyPress(2, activeTiles, currentTime, score);
                if (event.key.code == Keyboard::L) success = handleKeyPress(3, activeTiles, currentTime, score);
            
                // if (!success) {
                //     cout << "❌ 錯按，遊戲結束" << endl;
                //     gameOver = true;
                //     music.stop();
                // }
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
        for (auto& tile : activeTiles) {
            if (!tile.active) continue;

            // 根據 tile 的 duration 計算高度
            float height = tile.duration / 300.0f * SPEED;
            tile.y -= height;
            RectangleShape rect(Vector2f(GRID_W, height));
            rect.setPosition(tile.lane * GRID_W, tile.y);
            rect.setFillColor(Color::Black);
            window.draw(rect);
        }

        window.display();
    }

    return 0;
}
