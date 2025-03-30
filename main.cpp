#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <vector>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <fstream>
//khởi tạo thư viện

using namespace std;

const int SCREEN_WIDTH = 1200;// chiều dài màn hình
const int SCREEN_HEIGHT = 750;//chiều cao màn hình
const int PLAYER_SPEED = 3;//tốc đọ xuất phát của người chơi
const int JUMP_POWER = 10;//khả năng bật nhảy của người chơi
const int GRAVITY = 1;//hằng số trọng lực

int Currentspeed= PLAYER_SPEED;//tốc độ hiện tại của người chơi
bool soundFinished = false;//kiểm tra điều kiện để tắt hết hiệu ứng âm thanh khi chơi và bật hiệu ứng thua cuộc

//lớp người chơi gồm các thông số cơ bản như kích thước,tọa độ,ảnh của nhân vật,độ rơi của nhân vật,khả năng nhảy.
//(không có tốc độ vì để thuận tiện cho code,ta cho các chướng ngại vật tiến gần về phía người chơi)
class Player {
public:
    int x, y, width, height;
    int velocityY;
    bool isJumping;
    SDL_Texture* texture;

    Player(int startX, int startY, SDL_Texture* tex) {
        x = startX;
        y = startY;
        width = 50;
        height = 50;
        velocityY = 0;
        isJumping = false;
        texture = tex;
    }

    void update() {
        velocityY += GRAVITY;
        y += velocityY;
        if (y >= SCREEN_HEIGHT - height) {
            y = SCREEN_HEIGHT - height;
            velocityY = 0;
        }
        if (y <= 0){
            y=0;
        }
    }

    void jump() {
        isJumping = true;
        velocityY = -JUMP_POWER;
    }

    void render(SDL_Renderer* renderer, int cameraX) {
        SDL_Rect rect = { x - cameraX, y, width, height };
        SDL_RenderCopy(renderer, texture, NULL, &rect);
    }
};

//lớp chướng ngại vật bao gồm kích thước,tọa độ,và tốc độ tiến về phía người chơi
class Obstacle {
public:
    int x, y, width, height;
    SDL_Texture* texture;

    Obstacle(int locationx, int locationy, int LCHeight, SDL_Texture* tex) {
        x = locationx;
        y = locationy;
        width = 70;
        height = LCHeight;
        texture = tex;
    }

    void render(SDL_Renderer* renderer, int cameraX) {
        SDL_Rect rect = { x - cameraX, y, width, height };
        SDL_RenderCopy(renderer, texture, NULL, &rect);
    }
};

//hàm tái tạo chướng ngại vật khi nó đi qua hết màn hình

void updateObstacles(vector<Obstacle>& obstacles) {
    for (size_t i = 0; i < obstacles.size(); i += 2) {
        obstacles[i].x -= Currentspeed;
        obstacles[i + 1].x = obstacles[i].x;

        // Khi chướng ngại vật ra khỏi màn hình, đặt lại đúng vị trí
        if (obstacles[i].x < -70) {
            obstacles[i].x = SCREEN_WIDTH + 200;  // Reset tại vị trí cố định
            obstacles[i + 1].x = obstacles[i].x;
        }
    }
}

// hàm hiểm thị điểm lên màn hình(ngoài ra còn để hiển thị thêm nhiều phần khác)

void Renderscore(SDL_Renderer* renderer, TTF_Font* font, string Score,int x,int y) {
    SDL_Color color = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, Score.c_str(), color);
    if (!surface) {
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);


    int textW = 0, textH = 0;
    SDL_QueryTexture(texture, NULL, NULL, &textW, &textH);
    SDL_Rect scorerect = {x, y, textW, textH};

    SDL_RenderCopy(renderer, texture, NULL, &scorerect);
    SDL_DestroyTexture(texture);
}

//hàm kiểm tra va chạm giữa nhân vật và chướng ngại vật
bool checkCollision(const SDL_Rect& rect1, const SDL_Rect& rect2) {
    return rect1.x < rect2.x + rect2.w &&
           rect1.x + rect1.w > rect2.x &&
           rect1.y < rect2.y + rect2.h &&
           rect1.y + rect1.h > rect2.y;
}

//hàm khởi tạo cửa sổ,tiêu đề
bool initSDL(SDL_Window*& window, SDL_Renderer*& renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return false;
    }
    window = SDL_CreateWindow("Flappy Bird", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    return renderer != nullptr;
}

//hàm kiểm tra nên dùng âm thanh gì
void onSoundFinished(int channel) {
    soundFinished = true;
}

//hàm load ảnh
SDL_Texture* loadTexture(const char* path, SDL_Renderer* renderer) {
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (!loadedSurface) {
        return nullptr;
    }
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    SDL_FreeSurface(loadedSurface);
    return newTexture;
}

//hàm main
int main(int argc, char* argv[]) {
    int score = 0;
    bool startGame = false;
    bool gameOver = false; // Trạng thái game
    int dem = 0;
    int demscore =0;
    int musicback=0;
    ifstream file;
    file.open("Highestscore.txt");
    int highestscore;
    file >> highestscore;
    file.close();
    if (TTF_Init() == -1) return -1;
    TTF_Font* font = TTF_OpenFont("letter.ttf", 28);
    if (!font) return -1;
    SDL_Init(SDL_INIT_AUDIO);
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    Mix_Chunk* sound = Mix_LoadWAV("backgroundmusic.wav");
    Mix_ChannelFinished(onSoundFinished);
    Mix_PlayChannel(-1, sound, -1);
     //Đợi âm thanh kết thúc
    if (!sound){
        return -1;
    }
    //khởi tạo các loại âm thanh
    Mix_Chunk* jumpSound = Mix_LoadWAV("jump.wav");
    if (!jumpSound){
        return -1;
    }
    Mix_Chunk* Scoresound = Mix_LoadWAV("score.wav");
    if (!Scoresound){
        return -1;
    }
    Mix_Chunk* Highscoresound = Mix_LoadWAV("highscore.wav");
    if (!Highscoresound){
        return -1;
    }
    Mix_Chunk* Gameoversound = Mix_LoadWAV("gameover.wav");
    if (!Gameoversound){
        return -1;
    }
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    if (!initSDL(window, renderer)) return -1;

    IMG_Init(IMG_INIT_PNG);
    //khởi tạo các loại hình ảnh
    SDL_Texture* backgroundTexture = loadTexture("background.png", renderer);
    SDL_Texture* playerTexture = loadTexture("player.png", renderer);
    SDL_Texture* obstacleTexture = loadTexture("Obstacle.png", renderer);
    if (!backgroundTexture || !playerTexture || !obstacleTexture){
        return -1;
    }
    //khởi tạo nhân vật và chướng ngại vật
    Player player(300, SCREEN_HEIGHT - 400, playerTexture);
    vector<Obstacle> Obs = {
        Obstacle(SCREEN_WIDTH - 500, SCREEN_HEIGHT - 300, 300, obstacleTexture),
        Obstacle(SCREEN_WIDTH - 500, 0, 200, obstacleTexture),
        Obstacle(SCREEN_WIDTH - 200, SCREEN_HEIGHT - 250, 250, obstacleTexture),
        Obstacle(SCREEN_WIDTH - 200, 0, 250, obstacleTexture),
        Obstacle(SCREEN_WIDTH + 100, SCREEN_HEIGHT - 350, 350, obstacleTexture),
        Obstacle(SCREEN_WIDTH + 100, 0, 150, obstacleTexture),
        Obstacle(SCREEN_WIDTH + 400, SCREEN_HEIGHT - 300, 300, obstacleTexture),
        Obstacle(SCREEN_WIDTH + 400, 0, 200, obstacleTexture),
        Obstacle(SCREEN_WIDTH + 700, SCREEN_HEIGHT - 225, 225, obstacleTexture),
        Obstacle(SCREEN_WIDTH + 700, 0, 275, obstacleTexture)
    };

    bool quit = false;
    SDL_Event e;
    int cameraX = 0;//biến carema di chuyển theo nhân vật

    while (!quit) {
        //bắt đầu vòng lặp game
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                quit = true;
            else if (e.type == SDL_KEYDOWN) {
                if (!startGame){//trước khi vào game cần nhấn enter
                    if(e.key.keysym.sym == SDLK_RETURN){
                        startGame = true;
                    }
                }
                else if (!gameOver) {//kiểm tra xem người chơi đã thua chưa,
                    if (e.key.keysym.sym == SDLK_SPACE) {
                        player.jump();//nhấn space để nhảy lên
                        //Mix_PlayChannel(-1,jumpsound,0);
                    }
                } else {//nếu người chơi đã thua cần nhấn r để mọi thứ bắt đầu lại
                    if (e.key.keysym.sym == SDLK_r) {
                         //Restart game
                         //Mọi thứ được khởi tạo lại từ đầu
                        Mix_PlayChannel(-1, sound, -1);
                        gameOver = false;
                        score = 0;
                        dem = 0;
                        demscore =0;
                        musicback =0;
                        Currentspeed = PLAYER_SPEED;
                        player = Player(300, SCREEN_HEIGHT - 300, playerTexture);
                        Obs = {
                            Obstacle(SCREEN_WIDTH - 500, SCREEN_HEIGHT - 300, 300, obstacleTexture),
                            Obstacle(SCREEN_WIDTH - 500, 0, 200, obstacleTexture),
                            Obstacle(SCREEN_WIDTH - 200, SCREEN_HEIGHT - 250, 250, obstacleTexture),
                            Obstacle(SCREEN_WIDTH - 200, 0, 250, obstacleTexture),
                            Obstacle(SCREEN_WIDTH + 100, SCREEN_HEIGHT - 350, 350, obstacleTexture),
                            Obstacle(SCREEN_WIDTH + 100, 0, 150, obstacleTexture),
                            Obstacle(SCREEN_WIDTH + 400, SCREEN_HEIGHT - 300, 300, obstacleTexture),
                            Obstacle(SCREEN_WIDTH + 400, 0, 200, obstacleTexture),
                            Obstacle(SCREEN_WIDTH + 700, SCREEN_HEIGHT - 225, 225, obstacleTexture),
                            Obstacle(SCREEN_WIDTH + 700, 0, 275, obstacleTexture)
                        };
                    }
                }
            }
        }
        if (!startGame){
            //trước khi bắt đầu game sẽ hiển thị mọi thứ kèm dòng chữ"Press Enter to start game"
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
            player.render(renderer, cameraX);
            for (auto& obs : Obs) {
                obs.render(renderer, cameraX);
            }
            //hiển thị score,highest score
            string Score = "Your score is: " + to_string(score);
            Renderscore(renderer, font, Score,50,50);
            string Highestscore = "Highest score is: " + to_string(highestscore);
            Renderscore(renderer,font,Highestscore,300,50);
            Renderscore(renderer,font,"Press Enter to start game",300,300);
            SDL_RenderPresent(renderer);
            continue;
        }
        else if (!gameOver) {
            //bắt đầu các hàm khi game chạy
            updateObstacles(Obs);
            if (player.x > Obs[dem].x + Obs[dem].width) {//kiểm tra người đã di chuyển qua cột chưa
                score++;
                dem += 2;
                if(score <= highestscore ||  demscore !=0){//chạy hiệu ứng ăn điểm nếu người chơi không phá kỷ lục cũ
                    Mix_PlayChannel(-1,Scoresound,0);
                }
                else{//chạy hiệu ứng phá kỷ lục
                    Mix_PlayChannel(-1,Highscoresound,0);
                }
                if(score == highestscore+1){
                    demscore++;
                }
                if(score % 15 == 0 && Currentspeed <8 && score != 0) {
                    Currentspeed++;//với mỗi 15 điểm người chơi đạt được,con chim sẽ tăng 1 tốc độ và tối đa là 8;
                }  // Chuyển sang cặp chướng ngại vật tiếp theo
            if (dem >= Obs.size()) {
                dem = 0;  // Quay lại từ đầu danh sách
                }
            }
            if(score > highestscore){
                highestscore = score;
            }

            player.update();

             //Kiểm tra va chạm
            for (auto& obs : Obs) {
                SDL_Rect playerRect = { player.x, player.y, player.width, player.height };
                SDL_Rect obstacleRect = { obs.x, obs.y, obs.width, obs.height };
                //nếu va chạm người chơi sẽ thua
                if (checkCollision(playerRect, obstacleRect)) {
                    gameOver = true;
                    break;
                }//nếu người chơi chạm đỉnh cửa sổ hoặc đáy cửa sổ đều tính là thua
                if (player.y == SCREEN_HEIGHT - 50 || player.y == 0){
                    gameOver = true;
                    break;
                }
            }
            cameraX = player.x - SCREEN_WIDTH / 2;//camera đuổi theo nhân vật
            if (cameraX < 0) cameraX = 0;
        }
        // Vẽ mọi thứ lên màn hình
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);
        player.render(renderer, cameraX);
        for (auto& obs : Obs) {
            obs.render(renderer, cameraX);
        }
        //hiển thị điểm
        string Score = "Your score is: " + to_string(score);
        Renderscore(renderer, font, Score,50,50);
        string Highestscore = "Highest score is: " + to_string(highestscore);
        Renderscore(renderer,font,Highestscore,300,50);
        // Nếu game over, hiển thị "GAME OVER"

        if (gameOver) {//nếu thua
            if(musicback ==0){
                Mix_HaltChannel(-1);//dừng tất cả nhạc/hiệu ứng đang phát
                Mix_PlayChannel(-1,Gameoversound,0);//phát nhạc thua
                musicback ++;
            }
            Renderscore(renderer, font, "GAME OVER! Press 'R' to Restart.",50,100);
            ofstream file2;
            //mở file ghi điểm cao nhất và ghi vào kết quả cao nhất của người chơi để khi thoát game điểm cao nhất vẫn được lưu cho lần chơi tiếp theo
            file2.open("Highestscore.txt");
            if (file2.is_open()) {
                file2 << highestscore; // Ghi điểm mới
                file2.close();
            }
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    while (!soundFinished) {
            SDL_Delay(100);
        }
    Mix_FreeChunk(jumpSound); // Giải phóng âm thanh
    Mix_FreeChunk(sound);
    Mix_FreeChunk(sound);
    Mix_CloseAudio();
    // Giải phóng tài nguyên
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(obstacleTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
