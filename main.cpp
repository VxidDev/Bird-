#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <utility>
#include <vector>

termios orig_term;

int ParseArgs(char* argv[]) {
    try {
        if (argv[1] != NULL) {
            int num = std::stoi(argv[1]);
            return num;
        }
    } catch (std::invalid_argument) {
        std::cerr << "Invalid argument!\n";
        std::exit(1);
    } 

    return 6;
}

class Bird {
    public:
        bool isAlive = true;
        int y = 3;

        void update() {
            if (y < 14 && y > 0) {
                y++;
            } else {
                isAlive = false;
            }
        }

        void jump() {
            y -= 3;
            y = std::clamp(y , 0 , 14);
        }
};

void DrawFrame(Bird& bird , std::vector<std::pair<int , std::pair<int , bool>>> pipes , int score) {
    std::cout << "\033[2J\033[1;1H";
    char display[16][30];

    for (int i = 0 ; i < 16 ; i++) {
        std::fill(std::begin(display[i]), std::end(display[i]) , ' ');
    }

    for (int i = 0; i < pipes.size(); i++) {
        for (int j = 0; j < 15; j++) {
            display[j][pipes[i].first] = '|';
        }
    }

    for (int i = 0; i < pipes.size(); i++) {
        int start = pipes[i].second.first;
        int end = pipes[i].second.first + 4;
        for (int j = start; j < end; j++) {
            display[j][pipes[i].first] = ' '; 
        }
    }

    std::fill(std::begin(display[14]) , std::end(display[14]) , '=');
    std::fill(std::begin(display[0]) , std::end(display[0]) , '=');
    
    std::string scr = std::string("Score: ") + std::to_string(score);
    for (int i = 0; i < scr.length(); i++) {
        display[15][i] = scr[i];
    }

    if (display[bird.y][10] == ' ') {
        display[bird.y][10] = 'O';
    } else {
        bird.isAlive = false;
    }

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 30; j++) {
            std::cout << display[i][j];
        }
        std::cout << "\n";
    }
}

bool CheckJump() {
    char c = getchar();
    if (c != EOF) {
        if (c == ' ') return true;
    }
    return false;
}

void EnableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_term);
    termios t = orig_term;
    t.c_lflag &= ~ICANON;
    t.c_lflag &= ~ECHO;   
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void DisableRawMode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

void GenNewPipe(std::vector<std::pair<int , std::pair<int , bool>>>& pipes , std::uniform_int_distribution<>& rand , std::mt19937& gen) {
    std::pair<int , std::pair<int , bool>> pipe = {29 , { rand(gen) , false} };

    pipes.push_back(pipe);
}

void MovePipes(std::vector<std::pair<int , std::pair<int , bool>>>& pipes , int& score) {
    for (int i = 0; i < pipes.size(); i++) {
        if (pipes[i].first <= 0) {
            pipes.erase(pipes.begin() + i);
            continue;
        } else if (pipes[i].first < 10 && pipes[i].second.second == false) {
            pipes[i].second.second = true;
            score++;
        }
        pipes[i].first--;
    }
}

int main(int argc , char* argv[]) {
    EnableRawMode();
    fcntl(STDIN_FILENO , F_SETFL , O_NONBLOCK);
    int fps = ParseArgs(argv);

    if (fps <= 0) {
        std::cerr << "FPS must be greater than 0.\n";
        return 0;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(2 , 9);

    Bird bird = Bird();
    std::vector<std::pair<int , std::pair<int , bool>>> pipes;

    int ticksBeforeNextPipe = 0;
    int score = 0;

    while (bird.isAlive) {
        ticksBeforeNextPipe++;
        if (CheckJump()) bird.jump();
        DrawFrame(bird , pipes , score);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / fps));
        bird.update();
        if (ticksBeforeNextPipe > 10) {
            GenNewPipe(pipes , dist , gen);
            ticksBeforeNextPipe = 0;
        }
        MovePipes(pipes , score); 
    }

    std::cout << "\nGame over!" << " Score: " << score << std::endl;

    DisableRawMode();

    return 0;
}