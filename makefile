all:
	g++ -I./include -c select_song.cpp
	g++ -I./include -c dont_touch_white_block.cpp
	g++ select_song.o dont_touch_white_block.o -o play -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
