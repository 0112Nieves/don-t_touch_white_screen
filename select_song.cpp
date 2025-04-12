#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <iostream>
#include "select_song.hpp"

using namespace sf;
using namespace std;

string showSongSelection() {
    RenderWindow window(VideoMode(400, 600), "選擇歌曲");
    Font font;
    if (!font.loadFromFile("NotoSansTC.ttf")) {
        cerr << "無法載入字體！" << endl;
        return "";
    }

    vector<string> songNames = {
        "卡農", "青石巷", "夜曲", "結婚進行曲",
        "月光奏鳴曲", "給愛麗絲"
    };
    vector<Text> textItems;
    float offsetY = 0.0f;             // Y 軸偏移量 (for scrolling)
    const float scrollSpeed = 30.0f;  // 每次滾動移動距離

    // 設定每首歌的文字
    for (size_t i = 0; i < songNames.size(); ++i) {
        Text text;
        text.setFont(font);
        sf::String sfString = sf::String::fromUtf8(songNames[i].begin(), songNames[i].end());
        text.setString(sfString);
        text.setCharacterSize(36);
        text.setFillColor(Color::Black);
        text.setPosition(100, 100 + i * 100);  // 初始位置
        textItems.push_back(text);
    }

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();

            // 滑鼠滾輪滾動事件
            if (event.type == Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.wheel == Mouse::VerticalWheel) {
                    offsetY += event.mouseWheelScroll.delta * scrollSpeed;

                    float maxOffset = (songNames.size() * 100 + 100 - 600);
                    offsetY = std::min(offsetY, 0.f);               // 不讓超出上界（最上面）
                    offsetY = std::max(offsetY, -maxOffset);        // 不讓滑太下面
                }
            }

            // 滑鼠點擊選擇
            if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

                // 根據滑鼠位置對應歌名的索引
                for (size_t i = 0; i < textItems.size(); ++i) {
                    FloatRect bounds = textItems[i].getGlobalBounds();
                    if (bounds.contains(mousePos)) {
                        window.close();
                        return songNames[i];  // 回傳選中的歌名
                    }
                }
            }
        }

        // 設定每個 Text 位置
        for (size_t i = 0; i < textItems.size(); ++i) {
            textItems[i].setPosition(100, 100 + i * 100 + offsetY);
        }

        window.clear(Color::White);
        for (auto& text : textItems)
            window.draw(text);
        window.display();
    }

    return "";
}

int main()
{
    string str = showSongSelection();
    cout << str << endl;
    return 0;
}
