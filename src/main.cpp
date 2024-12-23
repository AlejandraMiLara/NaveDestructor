#include "raylib.h"
#include <vector>
#include <memory>
#include <cstdlib>
#include <algorithm>

Texture2D playerTexture; 
Texture2D slowEnemyTexture; 
Texture2D diagonalEnemyTexture; 
Texture2D zigzagEnemyTexture;

float speedModifier = 1.0f;


Music generalMusic;
Music gameOverMusic;

class StarParticle {
public:
    float x, y;
    float speedY;
    Color color;

    StarParticle(float x, float y, float speedY, Color color)
        : x(x), y(y), speedY(speedY), color(color) {}

    void Update() {
        y += speedY;
        if (y > GetScreenHeight()) {
            y = 0;
        }
    }

    void Draw() {
        DrawCircle(static_cast<int>(x), static_cast<int>(y), 2, color);
    }
};


class Particle {
public:
    float x, y;
    float speedX, speedY;
    float alpha;
    Color color;

    Particle(float x, float y, float speedX, float speedY, Color color)
        : x(x), y(y), speedX(speedX), speedY(speedY), alpha(1.0f), color(color) {}

    void Update() {
        x += speedX;
        y += speedY;
        alpha -= 0.02f;
        if (alpha < 0.0f) alpha = 0.0f;
    }

    void Draw() {
        Color c = color;
        c.a = static_cast<unsigned char>(alpha * 255);
        DrawCircle(static_cast<int>(x), static_cast<int>(y), 3, c);
    }

    bool IsActive() const {
        return alpha > 0.0f;
    }
};


class GameObject {
protected:
    float x, y;
    float width, height;

public:
    GameObject(float x, float y, float width, float height)
        : x(x), y(y), width(width), height(height) {}

    virtual void Update() = 0;
    virtual void Draw() = 0;

    bool CheckCollision(GameObject* other) {
        return (x < other->x + other->width &&
                x + width > other->x &&
                y < other->y + other->height &&
                y + height > other->y);
    }

    virtual ~GameObject() {}
};

class Enemy : public GameObject {
protected:
    float speed;

public:
    bool isColliding = false;

    Enemy(float x, float y, float width, float height, float speed)
        : GameObject(x, y, width, height), speed(speed) {}

    virtual ~Enemy() {}
};

class SlowEnemy : public Enemy {
public:
    SlowEnemy(float x, float y)
        : Enemy(x, y, 30, 30, 1.0f) {}

    void Update() override {
        y += speed * speedModifier;
    }

    void Draw() override {
        DrawTexture(slowEnemyTexture, static_cast<int>(x), static_cast<int>(y), WHITE);
    }
};

class DiagonalEnemy : public Enemy {
public:
    DiagonalEnemy(float x, float y)
        : Enemy(x, y, 30, 30, 1.0f) {}

    void Update() override {
        y += speed * speedModifier + 2;
        x += (speed / 2) * speedModifier + 2;
        if (x + width > GetScreenWidth() || x < 0) {
            speed = -speed;
        }
    }

    void Draw() override {
        DrawTexture(diagonalEnemyTexture, static_cast<int>(x), static_cast<int>(y), WHITE);
    }
};

class ZigzagEnemy : public Enemy {
public:
    bool movingRight;

    ZigzagEnemy(float x, float y)
        : Enemy(x, y, 30, 30, 1.0f), movingRight(true) {}

    void Update() override {
        y += speed * speedModifier;
        if (movingRight) {
            x += speed * speedModifier + 2;
            if (x + width > GetScreenWidth()) {
                movingRight = false;
            }
        } else {
            x -= speed * speedModifier+2;
            if (x < 0) {
                movingRight = true;
            }
        }
    }

    void Draw() override {
        DrawTexture(zigzagEnemyTexture, static_cast<int>(x), static_cast<int>(y), WHITE);
    }
};


class Projectile : public GameObject {
public:
    bool isColliding = false;

    Projectile(float x, float y)
        : GameObject(x, y, 5, 10) {}

    void Update() override {
        y -= 5;
    }

    void Draw() override {
        DrawRectangle(x, y, width, height, RED);
    }

    bool IsOffScreen() { return y < 0; }
};


class Player : public GameObject {
private:
    std::vector<std::unique_ptr<Projectile>>& projectiles;
    std::vector<Particle> particles;

public:
    Player(float x, float y, std::vector<std::unique_ptr<Projectile>>& projectiles)
        : GameObject(x, y, 40, 40), projectiles(projectiles) {}

    void Update() override {
        if (IsKeyDown(KEY_RIGHT)) x += 5;
        if (IsKeyDown(KEY_LEFT)) x -= 5;

        if (x < 0) x = 0;
        if (x + width > GetScreenWidth()) x = GetScreenWidth() - width;

        if (IsKeyPressed(KEY_SPACE)) {
            projectiles.push_back(std::make_unique<Projectile>(x + width / 2 - 2.5, y));
        }

        particles.push_back(Particle(x + width / 2, y + height, GetRandomValue(-2, 2) / 10.0f, 1.0f, WHITE));

        for (auto& particle : particles) {
            particle.Update();
        }
        particles.erase(std::remove_if(particles.begin(), particles.end(),
                                       [](const Particle& p) { return !p.IsActive(); }),
                        particles.end());
    }

    void Draw() override {
        DrawTexture(playerTexture, static_cast<int>(x), static_cast<int>(y), WHITE);
        for (auto& particle : particles) {
            particle.Draw();
        }
    }

    void SetPosition(float newX, float newY) {
        x = newX;
        y = newY;
    }
};


void ResetGame(Player& player, std::vector<std::unique_ptr<GameObject>>& enemies,
               std::vector<std::unique_ptr<Projectile>>& projectiles, int& score, int& lives) {
    enemies.clear();
    projectiles.clear();
    score = 0;
    lives = 5;
    player.SetPosition(400, 500);
}

int main() {
    InitWindow(800, 600, "Nave Destructor: Manejo de Colisiones");
    InitAudioDevice();
    SetTargetFPS(60);

    playerTexture = LoadTexture("src/nave.png");
    slowEnemyTexture = LoadTexture("src/pato1.png");
    diagonalEnemyTexture = LoadTexture("src/patito3.png");
    zigzagEnemyTexture = LoadTexture("src/patito2.png");

    generalMusic = LoadMusicStream("src/musica_general.mp3");
    gameOverMusic = LoadMusicStream("src/musica_fin.mp3");

    PlayMusicStream(generalMusic);

    std::vector<std::unique_ptr<GameObject>> enemies;
    std::vector<std::unique_ptr<Projectile>> projectiles;
    std::vector<StarParticle> starParticles;

    for (int i = 0; i < 100; i++) {
        starParticles.emplace_back(GetRandomValue(0, GetScreenWidth()), GetRandomValue(0, GetScreenHeight()), GetRandomValue(1, 5) / 2.0f, WHITE);
    }

    Player player(400, 500, projectiles);

    int score = 0;
    int lives = 5;
    bool isGameOver = false;

    speedModifier = 1.0f + score / 100.0f;

    while (!WindowShouldClose()) {
        UpdateMusicStream(generalMusic);
        UpdateMusicStream(gameOverMusic);

        if (!isGameOver) {
            int enemyType = GetRandomValue(0, 2);
            if (GetRandomValue(0, 100) < 2) {
                float x = GetRandomValue(0, 770);
                if (enemyType == 0) {
                    enemies.push_back(std::make_unique<SlowEnemy>(x, 0));
                } else if (enemyType == 1) {
                    enemies.push_back(std::make_unique<DiagonalEnemy>(x, 0));
                } else if (enemyType == 2) {
                    enemies.push_back(std::make_unique<ZigzagEnemy>(x, 0));
                }
            }

            player.Update();
            for (auto& projectile : projectiles) {
                projectile->Update();
            }
            for (auto& enemy : enemies) {
                enemy->Update();
                if (!static_cast<Enemy*>(enemy.get())->isColliding &&
                    enemy->CheckCollision(&player)) {
                    static_cast<Enemy*>(enemy.get())->isColliding = true;
                    lives--;
                    if (lives <= 0) {
                        isGameOver = true;
                        StopMusicStream(generalMusic);
                        PlayMusicStream(gameOverMusic);
                    }
                }
            }

            for (auto& projectile : projectiles) {
                for (auto& enemy : enemies) {
                    if (!projectile->isColliding && !static_cast<Enemy*>(enemy.get())->isColliding &&
                        projectile->CheckCollision(enemy.get())) {
                        projectile->isColliding = true;
                        static_cast<Enemy*>(enemy.get())->isColliding = true;
                        score += 10;
                    }
                }
            }

            enemies.erase(
                std::remove_if(enemies.begin(), enemies.end(),
                               [](const std::unique_ptr<GameObject>& enemy) {
                                   return static_cast<Enemy*>(enemy.get())->isColliding;
                               }),
                enemies.end());

            projectiles.erase(
                std::remove_if(projectiles.begin(), projectiles.end(),
                               [](const std::unique_ptr<Projectile>& projectile) {
                                   return projectile->IsOffScreen() || projectile->isColliding;
                               }),
                projectiles.end());


            for (auto& star : starParticles) {

                star.Update();
            }
        } else {
            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame(player, enemies, projectiles, score, lives);
                isGameOver = false;
                StopMusicStream(gameOverMusic);
                PlayMusicStream(generalMusic);
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (isGameOver) {
            DrawText("GAME OVER", 300, 250, 40, RED);
            DrawText(TextFormat("Score: %d", score), 320, 300, 20, WHITE);
            DrawText("Press ENTER to Restart", 260, 350, 20, WHITE);

            if (IsKeyPressed(KEY_ENTER)) {
                ResetGame(player, enemies, projectiles, score, lives);
                isGameOver = false;
            }
        } else {
            DrawText(TextFormat("Score: %d", score), 10, 10, 20, WHITE);
            DrawText(TextFormat("Lives: %d", lives), 10, 40, 20, WHITE);

            for (auto& star : starParticles) {
                star.Draw();
            }

            player.Draw();
            for (auto& enemy : enemies) {
                enemy->Draw();
            }
            for (auto& projectile : projectiles) {
                projectile->Draw();
            }
        }

        EndDrawing();
    }

    CloseWindow();

    UnloadTexture(playerTexture);
    UnloadTexture(slowEnemyTexture);
    UnloadTexture(diagonalEnemyTexture);
    UnloadTexture(zigzagEnemyTexture);
    UnloadMusicStream(generalMusic);
    UnloadMusicStream(gameOverMusic);

    CloseAudioDevice();

    return 0;
}