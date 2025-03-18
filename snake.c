#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stdio.h>

#define WIDTH  400
#define HEIGHT 400
#define CELL_SIZE 20
#define SNAKE_MAX_LENGTH 100
#define MAX_OBSTACLES 10
#define ANIMATION_STEPS 5

typedef struct {
    float x, y; 
} Point;


Point snake[SNAKE_MAX_LENGTH];
int snake_length = 3;
int head_index = 0;
Point food;
int direction = VK_RIGHT;
int score = 0;
bool running = true;
bool occupied_cells[WIDTH / CELL_SIZE][HEIGHT / CELL_SIZE] = { false };
Point obstacles[MAX_OBSTACLES];
int obstacle_count = 5;
float animation_progress = 0.0f;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MoveSnake(HWND hwnd);
void SpawnFood();
void SpawnObstacles();
void ResetGame(HWND hwnd);
void Render(HDC hdc, HWND hwnd);
void ShowGameOverScreen(HDC hdc, HWND hwnd);
DWORD WINAPI GameLoop(LPVOID lpParam);
void UpdateScore(HWND hwnd);
int LoadHighScore();
void SaveHighScore(int score);
void AnimateSnakeStep(HWND hwnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "Snake Game";
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, "Snake Game",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    srand((unsigned int)time(NULL));
    ResetGame(hwnd);
    CreateThread(NULL, 0, GameLoop, hwnd, 0, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_KEYDOWN:
        if (wParam == VK_SPACE) {
            running = !running;
        } else if ((wParam == VK_UP && direction != VK_DOWN) ||
                   (wParam == VK_DOWN && direction != VK_UP) ||
                   (wParam == VK_LEFT && direction != VK_RIGHT) ||
                   (wParam == VK_RIGHT && direction != VK_LEFT)) {
            direction = wParam;
        }
        break;
    case WM_GETMINMAXINFO: {
        MINMAXINFO* minMaxInfo = (MINMAXINFO*)lParam;
        minMaxInfo->ptMinTrackSize.x = WIDTH;
        minMaxInfo->ptMinTrackSize.y = HEIGHT;
        minMaxInfo->ptMaxTrackSize.x = WIDTH;
        minMaxInfo->ptMaxTrackSize.y = HEIGHT;
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        Render(hdc, hwnd);
        EndPaint(hwnd, &ps);
    } break;
    case WM_DESTROY:
        running = false;
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void MoveSnake(HWND hwnd) {
    Point new_head = snake[head_index];

    if (direction == VK_UP) new_head.y -= 1.0f;
    else if (direction == VK_DOWN) new_head.y += 1.0f;
    else if (direction == VK_LEFT) new_head.x -= 1.0f;
    else if (direction == VK_RIGHT) new_head.x += 1.0f;

    if (new_head.x < 0) new_head.x = WIDTH / CELL_SIZE - 1;
    else if (new_head.x >= WIDTH / CELL_SIZE) new_head.x = 0;
    if (new_head.y < 0) new_head.y = HEIGHT / CELL_SIZE - 1;
    else if (new_head.y >= HEIGHT / CELL_SIZE) new_head.y = 0;

    if (occupied_cells[(int)new_head.x][(int)new_head.y]) {
        running = false;
        return;
    }

    for (int i = 0; i < obstacle_count; i++) {
        if ((int)new_head.x == (int)obstacles[i].x && (int)new_head.y == (int)obstacles[i].y) {
            running = false;
            return;
        }
    }

    Point tail = snake[(head_index + snake_length - 1) % SNAKE_MAX_LENGTH];
    occupied_cells[(int)tail.x][(int)tail.y] = false;

    head_index = (head_index - 1 + SNAKE_MAX_LENGTH) % SNAKE_MAX_LENGTH;
    snake[head_index] = new_head;
    occupied_cells[(int)new_head.x][(int)new_head.y] = true;

    if ((int)new_head.x == food.x && (int)new_head.y == food.y) {
        score += 10;
        snake_length++;
        SpawnFood();
        UpdateScore(hwnd);
    }
}

void AnimateSnakeStep(HWND hwnd) {
    animation_progress += 1.0f / ANIMATION_STEPS;
    if (animation_progress >= 1.0f) {
        animation_progress = 0.0f;
        MoveSnake(hwnd);
    }
    InvalidateRect(hwnd, NULL, TRUE);
}

void SpawnFood() {
    bool valid_position;
    int attempts = 0;
    do {
        valid_position = true;
        food.x = rand() % (WIDTH / CELL_SIZE);
        food.y = rand() % (HEIGHT / CELL_SIZE);
        if (occupied_cells[(int)food.x][(int)food.y]) {
            valid_position = false;
        }
        for (int i = 0; i < obstacle_count; i++) {
            if ((int)obstacles[i].x == (int)food.x && (int)obstacles[i].y == (int)food.y) {
                valid_position = false;
                break;
            }
        }
        attempts++;
        if (attempts > 1000) {
            running = false;
            MessageBox(NULL, "No valid positions for food! Game Over.", "Error", MB_OK);
            return;
        }
    } while (!valid_position);
}

void SpawnObstacles() {
    for (int i = 0; i < obstacle_count; i++) {
        bool valid_position;
        do {
            valid_position = true;
            obstacles[i].x = rand() % (WIDTH / CELL_SIZE);
            obstacles[i].y = rand() % (HEIGHT / CELL_SIZE);
            if (occupied_cells[(int)obstacles[i].x][(int)obstacles[i].y]) {
                valid_position = false;
            }
        } while (!valid_position);

        occupied_cells[(int)obstacles[i].x][(int)obstacles[i].y] = true;
    }
}

void ResetGame(HWND hwnd) {
    snake_length = 3;
    head_index = 0;
    snake[0] = (Point){5.0f, 5.0f};
    snake[1] = (Point){4.0f, 5.0f};
    snake[2] = (Point){3.0f, 5.0f};
    direction = VK_RIGHT;
    score = 0;
    animation_progress = 0.0f;

    memset(occupied_cells, false, sizeof(occupied_cells));
    for (int i = 0; i < snake_length; i++) {
        occupied_cells[(int)snake[i].x][(int)snake[i].y] = true;
    }
    SpawnFood();
    SpawnObstacles();
    UpdateScore(hwnd);
    running = true;
}

void UpdateScore(HWND hwnd) {
    int highScore = LoadHighScore();
    if (score > highScore) {
        highScore = score;
        SaveHighScore(highScore);
    }

    char title[100];
    snprintf(title, sizeof(title), "Snake Game - Score: %d | High Score: %d", score, highScore);
    SetWindowText(hwnd, title);
}

int LoadHighScore() {
    FILE *file = fopen("highscore.txt", "r");
    if (!file) return 0;

    int highScore = 0;
    fscanf(file, "%d", &highScore);
    fclose(file);
    return highScore;
}

void SaveHighScore(int score) {
    FILE *file = fopen("highscore.txt", "w");
    if (file) {
        fprintf(file, "%d", score);
        fclose(file);
    }
}

void ShowGameOverScreen(HDC hdc, HWND hwnd) {
    RECT screenRect = { 0, 0, WIDTH, HEIGHT };
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 0, 0));

    // Display "Game Over" message
    DrawText(hdc, "GAME OVER", -1, &screenRect, DT_CENTER | DT_TOP | DT_SINGLELINE);

    // Display the player's score
    char scoreText[100];
    snprintf(scoreText, sizeof(scoreText), "Your Score: %d", score);
    RECT scoreRect = { 0, HEIGHT / 2 - 20, WIDTH, HEIGHT / 2 + 20 };
    DrawText(hdc, scoreText, -1, &scoreRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // Display restart and quit 
    RECT restartRect = { 0, HEIGHT / 2 + 30, WIDTH, HEIGHT / 2 + 60 };
    DrawText(hdc, "Press R to Restart or Q to Quit", -1, &restartRect, DT_CENTER | DT_SINGLELINE);
}

void Render(HDC hdc, HWND hwnd) {
    HDC backBuffer = CreateCompatibleDC(hdc);
    HBITMAP backBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
    SelectObject(backBuffer, backBitmap);

    // Draw background
    HBRUSH backgroundBrush = CreateSolidBrush(RGB(240, 240, 240));
    RECT backgroundRect = { 0, 0, WIDTH, HEIGHT };
    FillRect(backBuffer, &backgroundRect, backgroundBrush);
    DeleteObject(backgroundBrush);

    // Draw grid
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = SelectObject(backBuffer, gridPen);
    for (int x = 0; x <= WIDTH; x += CELL_SIZE) {
        MoveToEx(backBuffer, x, 0, NULL);
        LineTo(backBuffer, x, HEIGHT);
    }
    for (int y = 0; y <= HEIGHT; y += CELL_SIZE) {
        MoveToEx(backBuffer, 0, y, NULL);
        LineTo(backBuffer, WIDTH, y);
    }
    SelectObject(backBuffer, oldPen);
    DeleteObject(gridPen);

    // Draw the snake
    HBRUSH snakeBrush = CreateSolidBrush(RGB(0, 255, 0));
    for (int i = 0; i < snake_length; i++) {
        int current_index = (head_index + i) % SNAKE_MAX_LENGTH;
        Point current = snake[current_index];

        RECT rect = {
            (int)(current.x * CELL_SIZE),
            (int)(current.y * CELL_SIZE),
            (int)((current.x + 1) * CELL_SIZE),
            (int)((current.y + 1) * CELL_SIZE)
        };
        FillRect(backBuffer, &rect, snakeBrush);
    }
    DeleteObject(snakeBrush);

    // Draw the food
    HBRUSH foodBrush = CreateSolidBrush(RGB(255, 0, 0));
    SelectObject(backBuffer, foodBrush);
    Ellipse(
        backBuffer,
        (int)(food.x * CELL_SIZE),
        (int)(food.y * CELL_SIZE),
        (int)((food.x + 1) * CELL_SIZE),
        (int)((food.y + 1) * CELL_SIZE)
    );
    DeleteObject(foodBrush);

    // Draw obstacles
    HBRUSH obstacleBrush = CreateSolidBrush(RGB(0, 50, 128)); 
    for (int i = 0; i < obstacle_count; i++) {
        RECT rect = {
            (int)(obstacles[i].x * CELL_SIZE),
            (int)(obstacles[i].y * CELL_SIZE),
            (int)((obstacles[i].x + 1) * CELL_SIZE),
            (int)((obstacles[i].y + 1) * CELL_SIZE)
        };
        FillRect(backBuffer, &rect, obstacleBrush);
    }
    DeleteObject(obstacleBrush);

    // Show game-over or paused state
    if (!running) {
        ShowGameOverScreen(backBuffer, hwnd);
    }

    BitBlt(hdc, 0, 0, WIDTH, HEIGHT, backBuffer, 0, 0, SRCCOPY);

    DeleteObject(backBitmap);
    DeleteDC(backBuffer);
}

DWORD WINAPI GameLoop(LPVOID lpParam) {
    HWND hwnd = (HWND)lpParam;

    while (1) {
        if (running) {
            AnimateSnakeStep(hwnd);  // Handle smooth animation
        } else {
            // Show Game Over screen and wait for player input
            HDC hdc = GetDC(hwnd);
            ShowGameOverScreen(hdc, hwnd);
            ReleaseDC(hwnd, hdc);

            if (GetAsyncKeyState('R') & 0x8000) { // Restart the game
                ResetGame(hwnd);
            } else if (GetAsyncKeyState('Q') & 0x8000) { // Quit the game
                PostQuitMessage(0);
                break;
            }
        }
        Sleep(100 / ANIMATION_STEPS); // Adjust sleep for smooth animation
    }
    return 0;
}
