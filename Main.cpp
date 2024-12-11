#include <iostream>
#include <string>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

using namespace std;

Mix_Music* g_music = NULL;
Mix_Chunk* point = NULL;
Mix_Chunk* win = NULL;
Mix_Chunk* fly = NULL;
SDL_Window* g_window = NULL;
SDL_Renderer* g_screen = NULL;
SDL_Event g_event;
TTF_Font* g_font = NULL;

enum GameStatue { GameRunning, GameStart, GameOver, GameSkin };
GameStatue gamestatue = GameStart;
bool AppRunning = true;

const int FPS = 40;
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 640;
const int SCREEN_BBP = 32;

const int colorkey_r = 167;
const int colorkey_g = 175;
const int colorkey_b = 180;

enum Skin { Capypara, Crab, Whale };
Skin skin = Capypara;
enum Statue { UP = 1, DOWN = 2 };
const float FALL_SPEED = 0.6;
const float MAXFALLSPEED = 10;

const int MAX_MAP_X = 200;
const int MAX_MAP_Y = 20;
const int TILE_SIZE = 32;
const int MAX_TILE = 10;

class BaseObject
{
protected:
    SDL_Texture* object;
    SDL_Rect rect;
public:
    BaseObject() {
        object = NULL;
        rect.x = rect.y = rect.w = rect.h = 0;
    }
    ~BaseObject() {
        Free();
    }
    void SetRect(const int& x, const int& y) {
        rect.x = x; rect.y = y;
    }
    SDL_Rect GetRect() { return rect; }
    SDL_Texture* GetObject() { return object; }

    virtual void LoadImg(string path, SDL_Renderer* screen) {
        Free();
        SDL_Surface* loadsurface = IMG_Load(path.c_str());
        if (loadsurface != NULL) {
            //xoa nen anh
            //SDL_SetColorKey(loadsurface, SDL_TRUE, SDL_MapRGB(loadsurface->format, colorkey_r, colorkey_g, colorkey_b));
            object = SDL_CreateTextureFromSurface(screen, loadsurface);
            if (object != NULL) {
                rect.h = loadsurface->h;
                rect.w = loadsurface->w;
            }
            SDL_FreeSurface(loadsurface);
        }

    }

    void Show(SDL_Renderer* des, SDL_Rect* clip) {
        SDL_Rect renderquad = { rect.x , rect.y , rect.w , rect.h };
        SDL_RenderCopy(des, object, clip, &renderquad);
    }

    void Free() {
        if (object != NULL) {
            SDL_DestroyTexture(object);
            object = NULL;
            rect.w = rect.h = 0;
        }
    };
};
BaseObject g_background;

typedef struct Map
{
    int start_x_;
    int start_y_;

    int max_x_;
    int max_y_;

    int tile[MAX_MAP_Y][MAX_MAP_X];
    const char* filename_;
};

class TileMap : public BaseObject
{
public:
    TileMap() { ; }
    ~TileMap() { ; }
};

class GameMap
{
private:
    Map game_map_;
    TileMap tile_mat[MAX_TILE];
public:
    GameMap(SDL_Renderer* g_screen) {
        game_map_.start_x_ = game_map_.start_y_ = 0;
        LoadTiles(g_screen);
        LoadMap("resources/map/map.dat");
    }
    ~GameMap() { ; }

    void LoadMap(string path) {
        const char* name = path.c_str();
        FILE* fp = NULL;
        fopen_s(&fp, name, "rb");
        if (fp == NULL) {
            return;
        }
        game_map_.max_x_ = 0;
        game_map_.max_y_ = 0;
        for (int i = 0; i < MAX_MAP_Y; i++) {
            for (int j = 0; j < MAX_MAP_X; j++) {
                fscanf_s(fp, "%d", &game_map_.tile[i][j]);
                int val = game_map_.tile[i][j];
                if (val > 0) {
                    if (j > game_map_.max_x_) {
                        game_map_.max_x_ = j;
                    }
                    if (i > game_map_.max_y_) {
                        game_map_.max_y_ = i;
                    }
                }
            }
        }
        game_map_.max_x_ = (game_map_.max_x_ + 1) * TILE_SIZE;
        game_map_.max_y_ = (game_map_.max_y_ + 1) * TILE_SIZE;

        game_map_.start_x_ = 0;
        game_map_.start_y_ = 0;
        game_map_.filename_ = name;
        fclose(fp);
    };

    void LoadTiles(SDL_Renderer* screen) {
        char file_img[30];
        FILE* fp = NULL;

        for (int i = 0; i < MAX_TILE; i++) {
            sprintf_s(file_img, "resources/map/%d.png", i);

            fopen_s(&fp, file_img, "rb");
            if (fp == NULL) {
                continue;
            }
            fclose(fp);
            tile_mat[i].LoadImg(file_img, screen);
        }
    };

    void DrawMap(SDL_Renderer* screen) {
        int x1 = 0;
        int x2 = 0;

        int y1 = 0;
        int y2 = 0;

        int map_x = 0;
        int map_y = 0;

        map_x = game_map_.start_x_ / TILE_SIZE;
        x1 = (game_map_.start_x_ % TILE_SIZE) * -1;
        x2 = x1 + SCREEN_WIDTH + (x1 == 0 ? 0 : TILE_SIZE);

        map_y = game_map_.start_y_ / TILE_SIZE;
        y1 = (game_map_.start_y_ % TILE_SIZE) * -1;
        y2 = y1 + SCREEN_HEIGHT + (y1 == 0 ? 0 : TILE_SIZE);

        for (int i = y1; i < y2; i += TILE_SIZE) {
            map_x = game_map_.start_x_ / TILE_SIZE;
            for (int j = x1; j < x2; j += TILE_SIZE) {
                int val = game_map_.tile[map_y][map_x];
                if (val > 0) {
                    tile_mat[val].SetRect(j, i);
                    tile_mat[val].Show(screen, NULL);
                }
                map_x++;
            }
            map_y++;
        }
    };

    Map GetMap() const { return game_map_; }
    void SetMap(Map& map_data) { game_map_ = map_data; }
};
GameMap* gamemap;

class Character : public BaseObject
{
private:
    Statue statue;
    int x, y;
    float x_val, y_val;

    float frame;
    int width_frame, height_frame;
    SDL_Rect frame_clip[2];

    int map_x, map_y;

    int coin;
public:
    Character(SDL_Renderer* screen) {
        x = 100;
        y = 270;
        x_val = y_val = 0;
        width_frame = height_frame = 0;
        frame = 0;
        statue = DOWN;
        map_x = map_y = 0;
        coin = 0;
        LoadImg("resources/image/capypara.png", screen);
        width_frame = rect.w / 2;
        height_frame = rect.h;
        if (width_frame > 0 && height_frame > 0) {
            for (int i = 0; i < 2; i++) {
                frame_clip[i].x = i * width_frame;
                frame_clip[i].y = 0;
                frame_clip[i].w = width_frame;
                frame_clip[i].h = height_frame;
            }
        }
    }
    ~Character() { ; }

    void Show(SDL_Renderer* des) {
        SDL_RendererFlip flip = SDL_FLIP_NONE;
        switch (skin)
        {
        case Capypara:
            LoadImg("resources/image/capypara.png", des);
            break;
        case Crab:
            LoadImg("resources/image/crab.png", des);
            break;
        case Whale:
            LoadImg("resources/image/whale.png", des);
            break;
        default:
            break;
        }
        if (statue == UP) {
            flip = SDL_FLIP_VERTICAL;
        }

        frame += 0.2;
        if (frame >= 2)frame = 0;
        rect.x = x - map_x;
        rect.y = y - map_y;

        SDL_Rect* current_clip = &frame_clip[(int)frame];

        SDL_Rect renderquad = { rect.x, rect.y, width_frame, height_frame };
        SDL_RenderCopyEx(des, object, current_clip, &renderquad, 0.0, NULL, flip);
    }

    void HandleInput(SDL_Event event) {
        if ((event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) || event.type == SDL_MOUSEBUTTONDOWN)
        {
            Mix_PlayChannel(-1, fly, 0);
            if (statue == UP) {
                statue = DOWN;
                y_val = 2;
            }
            else {
                statue = UP;
                y_val = 2;
            }
        }
    }

    void move(Map& map_data) {
        x_val = 8;
        y_val += FALL_SPEED;
        if (y_val >= MAXFALLSPEED) {
            y_val = MAXFALLSPEED;
        }
        if (x - map_x > SCREEN_HEIGHT / 2) {
            x_val = 6;
        }

        CheckMap(map_data);
        MoveMap(map_data);
    }

    void CheckMap(Map& map_data) {
        int x1 = 0, y1 = 0;
        int x2 = 0, y2 = 0;

        if (statue == UP) {
            y_val = -y_val;
        }

        x1 = (x + x_val) / TILE_SIZE;
        x2 = (x + width_frame + x_val - 1) / TILE_SIZE;
        y1 = (y + y_val) / TILE_SIZE;
        y2 = (y + height_frame - 1) / TILE_SIZE;

        if (x1 >= 0 && x2 < MAX_MAP_X && y1 >= 0 && y2 < MAX_MAP_Y) {
            if (statue == UP) {
                y1 = (y + 1) / TILE_SIZE;
            }
            if (map_data.tile[y1][x2] == 4 || map_data.tile[y2][x2] == 4) {
                Mix_PlayChannel(-1, win, 0);
                map_data.tile[y1][x2] = 0;
                map_data.tile[y2][x2] = 0;

            }
            if (map_data.tile[y1][x2] == 5 || map_data.tile[y2][x2] == 5) {
                Mix_PlayChannel(-1, point, 0);
                map_data.tile[y1][x2] = 0;
                map_data.tile[y2][x2] = 0;
                coin++;
            }
            else if (map_data.tile[y1][x2] != 0 || map_data.tile[y2][x2] != 0) {
                x = x2 * TILE_SIZE;
                x = x - width_frame + 1;
                x_val = 0;
            }

        }

        x1 = x / TILE_SIZE;
        x2 = (x + TILE_SIZE) / TILE_SIZE;
        y1 = (y + y_val) / TILE_SIZE;
        y2 = (y + height_frame) / TILE_SIZE;
        if (x1 >= 0 && x2 < MAX_MAP_X && y1 >= 0 && y2 < MAX_MAP_Y) {
            if (statue == DOWN) {
                if (map_data.tile[y2][x1] != 0 || map_data.tile[y2][x2] != 0) {
                    y = y2 * TILE_SIZE;
                    y = y - height_frame;
                    y_val = 0;
                }
                if (map_data.tile[y2][x1] == 5 || map_data.tile[y2][x2] == 5) {
                    Mix_PlayChannel(-1, point, 0);
                    map_data.tile[y2][x1] = 0;
                    map_data.tile[y2][x2] = 0;
                    coin++;
                }
            }
            if (statue == UP) {
                if (map_data.tile[y1][x1] != 0 || map_data.tile[y1][x2] != 0) {
                    y = (y1 + 1) * TILE_SIZE - 1;
                    y_val = 0;
                }
                if (map_data.tile[y1][x1] == 5 || map_data.tile[y1][x2] == 5) {
                    Mix_PlayChannel(-1, point, 0);
                    map_data.tile[y1][x1] = 0;
                    map_data.tile[y2][x2] = 0;
                    coin++;
                }
            }

        }

        y += y_val;
        if (y_val < 0)y_val = -y_val;
        x += x_val;

        if (x < map_x || y < 0 || y + height_frame > SCREEN_HEIGHT || x > map_x + SCREEN_WIDTH) {
            gamestatue = GameOver;
        }

    }

    void MoveMap(Map& map_data) {

        map_data.start_x_ += 6;

        if (map_data.start_x_ + SCREEN_WIDTH >= map_data.max_x_) {
            map_data.start_x_ = map_data.max_x_ - SCREEN_WIDTH;
        }
    }

    void SetMapXY(const int map_x_, const int map_y_) { map_x = map_x_; map_y = map_y_; }
    int Getcoin() { return coin; }
};
Character* player;

class StartScreen : public BaseObject {
public:
    SDL_Texture* startTextures[2];
    SDL_Texture* startButtonTexture;
    SDL_Texture* logoTexture;
    SDL_Texture* skinTexture;
    SDL_Rect startRect;
    SDL_Rect startButtonRect;
    SDL_Rect logoRect;
    SDL_Rect skinRect;
    float frame;
public:
    StartScreen(SDL_Renderer* screen) {
        frame = 0;
        SDL_Surface* tempSurface = IMG_Load("resources/image/1.png");
        startTextures[0] = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        tempSurface = IMG_Load("resources/image/2.png");
        startTextures[1] = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        tempSurface = IMG_Load("resources/image/start.png");
        startButtonTexture = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        tempSurface = IMG_Load("resources/image/skin.png");
        skinTexture = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        tempSurface = IMG_Load("resources/image/logo.png");
        logoTexture = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        int w, h;
        SDL_QueryTexture(startTextures[0], nullptr, nullptr, &w, &h);
        startRect = { SCREEN_WIDTH / 2 - 256 / 2, 350, 256, 64 };

        SDL_QueryTexture(startButtonTexture, nullptr, nullptr, &w, &h);
        startButtonRect = { SCREEN_WIDTH / 2 - 128 / 2, 450, 128, 64 };

        SDL_QueryTexture(skinTexture, nullptr, nullptr, &w, &h);
        skinRect = { SCREEN_WIDTH * 3 / 4 - w / 2, 450, w, h };

        SDL_QueryTexture(logoTexture, nullptr, nullptr, &w, &h);
        logoRect = { SCREEN_WIDTH / 2 - w / 2, 0, 512, 256 };
    }

    ~StartScreen() {
        SDL_DestroyTexture(startTextures[0]);
        SDL_DestroyTexture(startTextures[1]);
        SDL_DestroyTexture(startButtonTexture);
        SDL_DestroyTexture(logoTexture);
        SDL_DestroyTexture(skinTexture);
    }

    void Show(SDL_Renderer* des) {
        frame += 0.2;
        if (frame >= 2)frame = 0;
        SDL_RenderCopy(des, startTextures[(int)frame], nullptr, &startRect);
        SDL_RenderCopy(des, startButtonTexture, nullptr, &startButtonRect);
        SDL_RenderCopy(des, logoTexture, nullptr, &logoRect);
        SDL_RenderCopy(des, skinTexture, nullptr, &skinRect);
    }

    void HandleInput(SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            if (mouseX >= startButtonRect.x && mouseX <= (startButtonRect.x + startButtonRect.w) &&
                mouseY >= startButtonRect.y && mouseY <= (startButtonRect.y + startButtonRect.h)) {
                delete player;
                player = new Character(g_screen);
                delete gamemap;
                gamemap = new GameMap(g_screen);
                gamestatue = GameRunning;
            }
            if (mouseX >= skinRect.x && mouseX <= (skinRect.x + skinRect.w) &&
                mouseY >= skinRect.y && mouseY <= (skinRect.y + skinRect.h)) {
                gamestatue = GameSkin;
            }
        }
    }
};
StartScreen* startscreen;


class OverScreen : public BaseObject {
private:
    SDL_Texture* gameOverTexture;
    SDL_Texture* restartButtonTexture;
    SDL_Texture* quitButtonTexture;
    SDL_Texture* skinTexture;
    SDL_Rect gameOverRect;
    SDL_Rect restartButtonRect;
    SDL_Rect quitButtonRect;
    SDL_Rect skinRect;
public:
    OverScreen(SDL_Renderer* screen) {
        SDL_Surface* tempSurface = IMG_Load("resources/image/restart.png");
        restartButtonTexture = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        tempSurface = IMG_Load("resources/image/quit.png");
        quitButtonTexture = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        tempSurface = IMG_Load("resources/image/gameover.png");
        gameOverTexture = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        tempSurface = IMG_Load("resources/image/skin.png");
        skinTexture = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        int w, h;
        SDL_QueryTexture(gameOverTexture, nullptr, nullptr, &w, &h);
        gameOverRect = { SCREEN_WIDTH / 2 - w / 2 , 200, w, h };

        SDL_QueryTexture(restartButtonTexture, nullptr, nullptr, &w, &h);
        restartButtonRect = { SCREEN_WIDTH - 128 - 350, 400, 128, 64 };

        SDL_QueryTexture(quitButtonTexture, nullptr, nullptr, &w, &h);
        quitButtonRect = { 350, 400, 128, 64 };

        SDL_QueryTexture(skinTexture, nullptr, nullptr, &w, &h);
        skinRect = { SCREEN_WIDTH / 2 - w / 2, 400, w, h };
    }

    ~OverScreen() {
        SDL_DestroyTexture(gameOverTexture);
        SDL_DestroyTexture(restartButtonTexture);
        SDL_DestroyTexture(quitButtonTexture);
        SDL_DestroyTexture(skinTexture);

    }

    void Show(SDL_Renderer* des) {
        SDL_RenderCopy(des, gameOverTexture, nullptr, &gameOverRect);
        SDL_RenderCopy(des, restartButtonTexture, nullptr, &restartButtonRect);
        SDL_RenderCopy(des, quitButtonTexture, nullptr, &quitButtonRect);
        SDL_RenderCopy(des, skinTexture, nullptr, &skinRect);
    }

    void HandleInput(SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            if (mouseX >= restartButtonRect.x && mouseX <= (restartButtonRect.x + restartButtonRect.w) &&
                mouseY >= restartButtonRect.y && mouseY <= (restartButtonRect.y + restartButtonRect.h)) {
                delete player;
                player = new Character(g_screen);
                delete gamemap;
                gamemap = new GameMap(g_screen);
                gamestatue = GameRunning;
            }
            if (mouseX >= quitButtonRect.x && mouseX <= (quitButtonRect.x + quitButtonRect.w) &&
                mouseY >= quitButtonRect.y && mouseY <= (quitButtonRect.y + quitButtonRect.h)) {
                AppRunning = false;
            }
            if (mouseX >= skinRect.x && mouseX <= (skinRect.x + skinRect.w) &&
                mouseY >= skinRect.y && mouseY <= (skinRect.y + skinRect.h)) {
                gamestatue = GameSkin;
            }
        }
    }
};
OverScreen* overscreen;

class SkinScreen {
private:
    SDL_Texture* capy;
    SDL_Texture* fish;
    SDL_Texture* whale;
    SDL_Rect capyRect;
    SDL_Rect fishRect;
    SDL_Rect whaleRect;
public:
    SkinScreen(SDL_Renderer* screen) {
        SDL_Surface* tempSurface = IMG_Load("resources/image/capypara.png");
        capy = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        tempSurface = IMG_Load("resources/image/crab.png");
        fish = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        tempSurface = IMG_Load("resources/image/whale.png");
        whale = SDL_CreateTextureFromSurface(screen, tempSurface);
        SDL_FreeSurface(tempSurface);

        int w, h;
        SDL_QueryTexture(capy, nullptr, nullptr, &w, &h);
        capyRect = { SCREEN_WIDTH * 1 / 4 - w / 2 , 250, 128, 128 };

        SDL_QueryTexture(fish, nullptr, nullptr, &w, &h);
        fishRect = { SCREEN_WIDTH * 2 / 4 - w / 2 , 250, 128, 128 };

        SDL_QueryTexture(whale, nullptr, nullptr, &w, &h);
        whaleRect = { SCREEN_WIDTH * 3 / 4 - w / 2 , 250, 128, 128 };

    }

    ~SkinScreen() {
        SDL_DestroyTexture(capy);
        SDL_DestroyTexture(fish);
        SDL_DestroyTexture(whale);
    }

    void Show(SDL_Renderer* des) {
        SDL_Rect renderquad = { 0, 0, 64, 64 };
        SDL_RenderCopy(des, capy, &renderquad, &capyRect);
        SDL_RenderCopy(des, fish, &renderquad, &fishRect);
        SDL_RenderCopy(des, whale, &renderquad, &whaleRect);
    }

    void HandleInput(SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            if (mouseX >= capyRect.x && mouseX <= (capyRect.x + capyRect.w) &&
                mouseY >= capyRect.y && mouseY <= (capyRect.y + capyRect.h)) {
                skin = Capypara;
                gamestatue = GameStart;
            }
            if (mouseX >= fishRect.x && mouseX <= (fishRect.x + fishRect.w) &&
                mouseY >= fishRect.y && mouseY <= (fishRect.y + fishRect.h)) {
                skin = Crab;
                gamestatue = GameStart;
            }
            if (mouseX >= whaleRect.x && mouseX <= (whaleRect.x + whaleRect.w) &&
                mouseY >= whaleRect.y && mouseY <= (whaleRect.y + whaleRect.h)) {
                skin = Whale;
                gamestatue = GameStart;
            }
        }
    }
};
SkinScreen* skinscreen;

class Text {
private:
    string text;
    SDL_Color color;
    SDL_Texture* texture;
    int w; int h;
public:
    Text() {
        color.r = 255;
        color.g = 255;
        color.b = 255;
        w = h = 0;
        texture = NULL;
    };
    ~Text() {
        Free();
    };
    void LoadText(TTF_Font* font, SDL_Renderer* screen) {
        Free();
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), color);
        if (textSurface)
        {
            texture = SDL_CreateTextureFromSurface(screen, textSurface);
            w = textSurface->w;
            h = textSurface->h;
            SDL_FreeSurface(textSurface);
        }
    }
    void Show(SDL_Renderer* screen, string text, int x, int y, int r, int g, int b) {
        SetColor(r, g, b);
        Settext(text);
        LoadText(g_font, screen);
        ShowText(screen, x, y);
    }
    void ShowText(SDL_Renderer* screen, int x, int y) {
        SDL_Rect renderquad = { x,y,w,h };
        SDL_RenderCopyEx(screen, texture, NULL, &renderquad, 0.0, NULL, SDL_FLIP_NONE);
    }
    void Free() {
        SDL_DestroyTexture(texture);
        texture = NULL;

    }
    int Getw() { return w; }
    int Geth() { return h; }
    void SetColor(int r, int g, int b) { color.r = r; color.g = g; color.b = b; }
    void Settext(string text_) { text = text_; }
};
Text* text;

//ham khoi tao game
void InitData() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        cout << "khong khoi tao duoc SDL " << SDL_GetError() << endl;
    }
    if (!(IMG_Init(IMG_INIT_PNG) && IMG_INIT_PNG)) {
        cout << "khong khoi tao duoc SDL image " << IMG_GetError() << endl;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cout << "khong khoi tao duoc nhac " << Mix_GetError();
    }
    if (TTF_Init() == -1) {
        cout << "khong khoi tao duoc font chu " << TTF_GetError();
    }

    //lam game smooth hon ?:)
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    g_window = SDL_CreateWindow("Marinehust",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    g_screen = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    g_font = TTF_OpenFont("resources/font/font.ttf", 20);
    g_music = Mix_LoadMUS("resources/sound/music.mp3");
    fly = Mix_LoadWAV("resources/sound/wing.wav");
    point = Mix_LoadWAV("resources/sound/point.wav");
    win = Mix_LoadWAV("resources/sound/win.wav");
    g_background.LoadImg("resources/image/background1.png", g_screen);

    text = new Text;
    player = new Character(g_screen);
    gamemap = new GameMap(g_screen);
    overscreen = new OverScreen(g_screen);
    startscreen = new StartScreen(g_screen);
    skinscreen = new SkinScreen(g_screen);
}

//ham xu ly du lieu nhap vao
void HandleEvent(SDL_Event& event) {
    if (gamestatue == GameStart) {
        startscreen->HandleInput(event);
    }
    else if (gamestatue == GameRunning) {
        player->HandleInput(event);
    }
    else if (gamestatue == GameOver) {
        overscreen->HandleInput(event);
    }
    else if (gamestatue == GameSkin) {
        skinscreen->HandleInput(event);
    }

}

//ham tinh toan, xu ly va cham
void Update() {
    if (gamestatue == GameRunning) {
        Map map_data = gamemap->GetMap();
        player->SetMapXY(map_data.start_x_, map_data.start_y_);
        player->move(map_data);
        gamemap->SetMap(map_data);
    }
}

//ham hien thi len man hinh
void Show() {
    SDL_SetRenderDrawColor(g_screen, 0Xff, 0Xff, 0Xff, 0Xff);
    SDL_RenderClear(g_screen);

    g_background.Show(g_screen, NULL);
    if (gamestatue == GameStart) {
        text->Show(g_screen, "Author: Ha Thai San - Chu Xuan Dat", 40, SCREEN_HEIGHT - 40, 255, 255, 255);
        text->Show(g_screen, "Press space or click mouse to change gravity", 450, 300, 30, 10, 160);
        startscreen->Show(g_screen);
    }
    else if (gamestatue == GameRunning) {
        player->Show(g_screen);
        gamemap->DrawMap(g_screen);
        string coin = "Coin: " + to_string(player->Getcoin());
        text->Show(g_screen, coin, 40, 40, 255, 255, 255);
    }
    else if (gamestatue == GameOver) {
        overscreen->Show(g_screen);
    }
    else if (gamestatue == GameSkin) {
        skinscreen->Show(g_screen);
    }

    SDL_RenderPresent(g_screen);
}

// ham ket thuc chuong trinh
void Close() {
    g_background.Free();
    SDL_DestroyRenderer(g_screen);
    g_screen = NULL;
    SDL_DestroyWindow(g_window);
    g_window = NULL;
    TTF_CloseFont(g_font);
    g_font = NULL;
    Mix_FreeMusic(g_music);
    g_music = NULL;

    delete player;
    delete overscreen;
    delete startscreen;
    delete skinscreen;
    delete gamemap;

    Mix_FreeChunk(fly);
    Mix_FreeChunk(point);
    Mix_FreeChunk(win);
    SDL_Quit(); IMG_Quit(); Mix_Quit(); TTF_Quit();
}

//ham main
int main(int argc, char* argv[]) {

    InitData();
    Mix_PlayMusic(g_music, -1);

    while (AppRunning)
    {
        Uint32 starttick = SDL_GetTicks();

        while (SDL_PollEvent(&g_event) != 0) {
            if (g_event.type == SDL_QUIT) {
                AppRunning = false;
            }
            HandleEvent(g_event);
        }

        Update();
        Show();

        //xu ly FPS;
        if (SDL_GetTicks() - starttick < FPS) {
            SDL_Delay(FPS - (SDL_GetTicks() - starttick));
        }
    }
    Close();
    return 0;
}