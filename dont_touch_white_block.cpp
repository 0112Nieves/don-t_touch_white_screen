#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <cmath>
#include "select_song.hpp"

using namespace sf;
using namespace std;
using json = nlohmann::json;

const int WIDTH = 400;
const int HEIGHT = 700;
const int GRID_W = 100;
const float TOLERANCE = HEIGHT / 300.0f * 1000.0f; // 容錯時間（ms）

struct Tile {
    int lane;
    float time;
    float duration;
    float y;
    bool active;
    bool triggered;
    bool missed; // 新增，紀錄是否失誤（畫成紅色用）
};

vector<Tile> loadChart(const json& chart) {
    vector<Tile> tiles;
    const float minTileDuration = 300.0f;

    for (auto& note : chart) {
        float startTime = note["time"];
        int lane = note["lane"];
        float duration = note["duration"];

        if (duration <= minTileDuration) {
            tiles.push_back({ lane, startTime, duration, 0.0f, true, false, false });
        } else {
            int pieces = ceil(duration / minTileDuration);
            for (int i = 0; i < pieces; ++i) {
                tiles.push_back({ lane, startTime + i * minTileDuration, minTileDuration, 0.0f, true, false, false });
            }
        }
    }
    return tiles;
}

bool handleKeyPress(int lanePressed, vector<Tile>& tiles, float currentTime, int& score, bool& madeMistake) {
    for (auto& tile : tiles) {
        if (!tile.active || tile.triggered)
            continue;

        if (tile.lane == lanePressed) {
            float diff = currentTime - tile.time;
            if (abs(diff) <= TOLERANCE) {
                tile.active = false;
                tile.triggered = true;
                score++;
                return true;
            }
        }
    }

    // 如果找不到正確tile，代表按錯lane！
    madeMistake = true;
    return false;
}

int main() {
    RenderWindow window(VideoMode(WIDTH, HEIGHT), "別踩白塊兒");
    window.setFramerateLimit(60);

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

    string songName = showSongSelection();
    string musicPath = "ogg/" + songName + ".ogg";
    string chartPath = "json/" + songName + ".json";

    Music music;
    if (!music.openFromFile(musicPath)) {
        cerr << "無法載入音樂\n";
        return -1;
    }

    ifstream file(chartPath);
    json chartJson;
    file >> chartJson;

    vector<Tile> chart = loadChart(chartJson);
    vector<Tile> activeTiles;

    Clock globalClock;
    int score = 0;
    size_t chartIndex = 0;
    bool gameOver = false;
    bool mistakeHappened = false;  // 有錯誤發生
    Clock mistakeClock;            // 記錯誤時間

    // 下落設定
    float SPEED = 300.0f; 
    float TARGET_Y = HEIGHT / 2.0f;
    float travelTime = TARGET_Y / SPEED * 1000.0f;

    float firstNoteTime = chart[0].time;
    float musicOffset = firstNoteTime - travelTime;
    if (musicOffset < 0) musicOffset = 0;

    music.setPlayingOffset(milliseconds(static_cast<int>(musicOffset)));
    music.play();

    map<Keyboard::Key, Clock> keyCooldown;
    float cooldownMs = 160;

    while (window.isOpen()) {
        float currentTime = music.getPlayingOffset().asMilliseconds() + musicOffset;

        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();

            if (event.type == Event::KeyPressed && !gameOver && !mistakeHappened) {
                if (keyCooldown[event.key.code].getElapsedTime().asMilliseconds() > cooldownMs) {
                    keyCooldown[event.key.code].restart();

                    bool success = true;
                    bool madeMistake = false;
                    if (event.key.code == Keyboard::A) success = handleKeyPress(0, activeTiles, currentTime, score, madeMistake);
                    if (event.key.code == Keyboard::F) success = handleKeyPress(1, activeTiles, currentTime, score, madeMistake);
                    if (event.key.code == Keyboard::H) success = handleKeyPress(2, activeTiles, currentTime, score, madeMistake);
                    if (event.key.code == Keyboard::L) success = handleKeyPress(3, activeTiles, currentTime, score, madeMistake);

                    if (madeMistake) {
                        mistakeHappened = true;
                        mistakeClock.restart();
                        music.pause();
                    }
                }
            }
        }

        if (!gameOver && !mistakeHappened) {
            while (chartIndex < chart.size() && chart[chartIndex].time <= currentTime + 1500) {
                activeTiles.push_back(chart[chartIndex]);
                chartIndex++;
            }

            for (auto& tile : activeTiles) {
                float elapsed = currentTime - tile.time;
                tile.y = (elapsed / 1000.0f * SPEED) + TARGET_Y;
            }

            for (auto& tile : activeTiles) {
                if (tile.y > HEIGHT && tile.active) {
                    tile.active = false;
                    tile.triggered = true;
                    tile.missed = true; // 超出底部也算miss
                    mistakeHappened = true;
                    mistakeClock.restart();
                    music.pause();
                }
            }
        }

        if (mistakeHappened && mistakeClock.getElapsedTime().asSeconds() > 2.0f) {
            gameOver = true;
        }

        scoreText.setString("Score: " + to_string(score));

        window.clear(Color::White);
        window.draw(scoreText);

        // RectangleShape line(Vector2f(WIDTH, 2));
        // line.setPosition(0, HEIGHT - 150);
        // line.setFillColor(Color(200, 0, 0));
        // window.draw(line);

        float fixedTileHeight = 150;
        for (auto& tile : activeTiles) {
            if (!tile.active && !tile.triggered) continue;

            float totalHeight = tile.duration / 1000.0f * SPEED;
            if (totalHeight < fixedTileHeight) totalHeight = fixedTileHeight;

            int numTiles = ceil(totalHeight / fixedTileHeight);
            float baseY = tile.y - totalHeight;

            for (int i = 0; i < numTiles; ++i) {
                RectangleShape rect(Vector2f(GRID_W, fixedTileHeight));
                rect.setPosition(tile.lane * GRID_W, baseY + i * fixedTileHeight);

                if (tile.missed) {
                    rect.setFillColor(mistakeClock.getElapsedTime().asMilliseconds() / 200 % 2 == 0 ? Color::Red : Color::White); // 閃爍
                } else if (tile.triggered) {
                    rect.setFillColor(Color(128, 128, 128));
                } else {
                    rect.setFillColor(Color::Black);
                }

                window.draw(rect);
            }
        }

        window.display();
    }

    return 0;
}
