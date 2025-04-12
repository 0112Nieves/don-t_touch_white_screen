#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp> // 加這行才能用 sf::Music
#include <vector>
#include <string>
#include <iostream>

using namespace sf;
using namespace std;

string showSongSelection() {
    RenderWindow window(VideoMode(500, 700), "選擇歌曲");

    // 載入並播放背景音樂
    Music bgm;
    if (!bgm.openFromFile("ogg/Carefree.ogg")) {  // 請確認音樂檔案路徑正確
        cerr << "無法載入背景音樂！" << endl;
    } else {
        bgm.setLoop(true); // 讓音樂一直播放
        bgm.play();
    }

    Font font;
    if (!font.loadFromFile("NotoSansTC.ttf")) {
        cerr << "無法載入字體！" << endl;
        return "";
    }

    vector<string> songNames = {
        "卡農", "青石巷", "夜曲", "結婚進行曲",
        "月光奏鳴曲", "給愛麗絲", "花之舞"
    };

    float offsetY = 0.0f;
    const float scrollSpeed = 40.0f;
    const float buttonHeight = 80.f;
    const float spacing = 30.f;
    const float buttonWidth = 360.f;
    const float startX = 70.f;
    const float startY = 80.f;

    while (window.isOpen()) {
        Event event;
        Vector2f mousePos = Vector2f(Mouse::getPosition(window));

        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();

            if (event.type == Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.wheel == Mouse::VerticalWheel) {
                    offsetY += event.mouseWheelScroll.delta * scrollSpeed;
                    float maxOffset = songNames.size() * (buttonHeight + spacing) - 700 + startY;
                    offsetY = min(offsetY, 0.f);
                    offsetY = max(offsetY, -maxOffset);
                }
            }

            if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                for (size_t i = 0; i < songNames.size(); ++i) {
                    FloatRect buttonRect(startX, startY + i * (buttonHeight + spacing) + offsetY, buttonWidth, buttonHeight);
                    if (buttonRect.contains(mousePos)) {
                        window.close();
                        return songNames[i];
                    }
                }
            }
        }

        window.clear(Color(245, 245, 245)); // 背景淡灰白

        for (size_t i = 0; i < songNames.size(); ++i) {
            float posY = startY + i * (buttonHeight + spacing) + offsetY;

            RectangleShape button(Vector2f(buttonWidth, buttonHeight));
            button.setPosition(startX, posY);
            button.setFillColor(Color(255, 255, 255, 200));
            button.setOutlineThickness(1.5f);
            button.setOutlineColor(Color(220, 220, 220));

            if (button.getGlobalBounds().contains(mousePos)) {
                button.setFillColor(Color(255, 255, 255, 240));
                button.setOutlineColor(Color(180, 180, 180));
            }

            Text text;
            text.setFont(font);
            text.setCharacterSize(28);
            text.setFillColor(Color(50, 50, 50));
            text.setString(sf::String::fromUtf8(songNames[i].begin(), songNames[i].end()));

            FloatRect textBounds = text.getLocalBounds();
            text.setOrigin(textBounds.left + textBounds.width / 2.0f,
                           textBounds.top + textBounds.height / 2.0f);
            text.setPosition(startX + buttonWidth / 2.0f, posY + buttonHeight / 2.0f);

            window.draw(button);
            window.draw(text);
        }

        window.display();
    }

    return "";
}