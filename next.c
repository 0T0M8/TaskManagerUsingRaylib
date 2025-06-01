#include "raylib.h"
#include <string.h>
#include <sqlite3.h>
#include <stdio.h>
#include <openssl/sha.h>

#define MAX_INPUT_LEN 256

typedef enum {
    SCREEN_REGISTRATION,
    SCREEN_LOGIN,
    SCREEN_DASHBOARD
} ScreenState;

typedef struct {
    char username[MAX_INPUT_LEN];
    char password[MAX_INPUT_LEN];
    bool usernameFocused;
    bool passwordFocused;
} UserData;

void DrawTextInput(int x, int y, int width, int height, char *buffer, bool focused, const char *placeholder);
void ShowPopup(const char *message, Color bgColor);
bool RegisterUser(const char *username, const char *password, sqlite3 *db);
bool LoginUser(const char *username, const char *password, sqlite3 *db);
void HashPassword(const char *password, char *hashedPassword);
void DrawDashboard(const char *username);

int main(void) {
    InitWindow(800, 600, "Task Manager");
    SetTargetFPS(60);

    sqlite3 *db;
    if (sqlite3_open("users.db", &db)) {
        printf("Failed to open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    const char *createTableQuery = "CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, username TEXT UNIQUE, password TEXT);";
    if (sqlite3_exec(db, createTableQuery, NULL, NULL, NULL) != SQLITE_OK) {
        printf("Failed to create table: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    ScreenState currentScreen = SCREEN_REGISTRATION;
    UserData registration = {"", "", false, false};
    UserData login = {"", "", false, false};
    char loggedInUsername[MAX_INPUT_LEN] = "";
    char popupMessage[256] = "";
    bool showPopup = false;
    Color popupColor = RED;

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        if (showPopup && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            showPopup = false;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (currentScreen) {
            case SCREEN_REGISTRATION: {
                DrawText("Register", 350, 120, 40, DARKGRAY);
                DrawTextInput(250, 200, 300, 40, registration.username, registration.usernameFocused, "Username");
                DrawTextInput(250, 270, 300, 40, registration.password, registration.passwordFocused, "Password");
                bool hoverRegister = mouse.x > 300 && mouse.x < 500 && mouse.y > 350 && mouse.y < 400;
                Color registerColor = hoverRegister ? DARKGRAY : LIGHTGRAY;
                DrawRectangle(300, 350, 200, 50, registerColor);
                DrawText("Register", 355, 365, 20, BLACK);

                if (showPopup) {
                    ShowPopup(popupMessage, popupColor);
                }

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    registration.usernameFocused = mouse.x > 250 && mouse.x < 550 && mouse.y > 200 && mouse.y < 240;
                    registration.passwordFocused = mouse.x > 250 && mouse.x < 550 && mouse.y > 270 && mouse.y < 310;
                }

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse.x > 300 && mouse.x < 500 && mouse.y > 350 && mouse.y < 400) {
                    if (strlen(registration.username) > 0 && strlen(registration.password) > 0) {
                        if (RegisterUser(registration.username, registration.password, db)) {
                            strcpy(popupMessage, "Registration successful! Redirecting to login...");
                            popupColor = GREEN;
                            currentScreen = SCREEN_LOGIN;
                        } else {
                            strcpy(popupMessage, "Username already exists or invalid input!");
                            popupColor = RED;
                        }
                    } else {
                        strcpy(popupMessage, "Please fill in all fields!");
                        popupColor = RED;
                    }
                    showPopup = true;
                }
                break;
            }
            case SCREEN_LOGIN: {
                DrawText("Login", 350, 120, 40, DARKGRAY);
                DrawTextInput(250, 200, 300, 40, login.username, login.usernameFocused, "Username");
                DrawTextInput(250, 270, 300, 40, login.password, login.passwordFocused, "Password");
                bool hoverLogin = mouse.x > 300 && mouse.x < 500 && mouse.y > 350 && mouse.y < 400;
                Color loginColor = hoverLogin ? DARKGRAY : LIGHTGRAY;
                DrawRectangle(300, 350, 200, 50, loginColor);
                DrawText("Login", 355, 365, 20, BLACK);

                if (showPopup) {
                    ShowPopup(popupMessage, popupColor);
                }

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    login.usernameFocused = mouse.x > 250 && mouse.x < 550 && mouse.y > 200 && mouse.y < 240;
                    login.passwordFocused = mouse.x > 250 && mouse.x < 550 && mouse.y > 270 && mouse.y < 310;
                }

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse.x > 300 && mouse.x < 500 && mouse.y > 350 && mouse.y < 400) {
                    if (LoginUser(login.username, login.password, db)) {
                        strcpy(popupMessage, "Login successful! Redirecting to dashboard...");
                        popupColor = GREEN;
                        strcpy(loggedInUsername, login.username);
                        currentScreen = SCREEN_DASHBOARD;
                    } else {
                        strcpy(popupMessage, "Invalid username or password!");
                        popupColor = RED;
                    }
                    showPopup = true;
                }
                break;
            }
            case SCREEN_DASHBOARD: {
                DrawDashboard(loggedInUsername);
                break;
            }
        }

        EndDrawing();
    }

    sqlite3_close(db);
    CloseWindow();
    return 0;
}

void DrawTextInput(int x, int y, int width, int height, char *buffer, bool focused, const char *placeholder) {
    Color borderColor = focused ? BLUE : LIGHTGRAY;
    DrawRectangleLines(x, y, width, height, borderColor);
    if (strlen(buffer) > 0) {
        DrawText(buffer, x + 5, y + 8, 20, BLACK);
    } else if (!focused) {
        DrawText(placeholder, x + 5, y + 8, 20, GRAY);
    }
}

void ShowPopup(const char *message, Color bgColor) {
    DrawRectangle(200, 250, 400, 100, bgColor);
    DrawText(message, 220, 290, 20, WHITE);
}

bool RegisterUser(const char *username, const char *password, sqlite3 *db) {
    char hashedPassword[SHA256_DIGEST_LENGTH * 2 + 1];
    HashPassword(password, hashedPassword);

    const char *insertQuery = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, insertQuery, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashedPassword, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool LoginUser(const char *username, const char *password, sqlite3 *db) {
    char hashedPassword[SHA256_DIGEST_LENGTH * 2 + 1];
    HashPassword(password, hashedPassword);

    const char *selectQuery = "SELECT 1 FROM users WHERE username = ? AND password = ?;";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hashedPassword, -1, SQLITE_STATIC);

    bool authenticated = sqlite3_step(stmt) == SQLITE_ROW;

    sqlite3_finalize(stmt);
    return authenticated;
}

void HashPassword(const char *password, char *hashedPassword) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)password, strlen(password), hash);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hashedPassword + (i * 2), "%02x", hash[i]);
    }
    hashedPassword[SHA256_DIGEST_LENGTH * 2] = '\0';
}

void DrawDashboard(const char *username) {
    DrawText(TextFormat("Welcome, %s!", username), 250, 100, 30, DARKGRAY);
    DrawText("Here is your dashboard", 250, 150, 20, GRAY);
    // Add task-related UI components here
}
