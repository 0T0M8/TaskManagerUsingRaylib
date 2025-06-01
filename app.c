#include "raylib.h"
#include <sqlite3.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_INPUT_LEN 256
#define MAX_TASKS 100

typedef struct {
    int id;
    char title[MAX_INPUT_LEN];
    bool completed;
} Task;

typedef enum {
    SCREEN_REGISTRATION,
    SCREEN_LOGIN,
    SCREEN_DASHBOARD
} ScreenState;

// Function Prototypes
void AddTask(const char *username, const char *title, sqlite3 *db);
int FetchTasks(const char *username, Task tasks[], sqlite3 *db);
void MarkTaskComplete(int taskId, sqlite3 *db);
void DeleteTask(int taskId, sqlite3 *db);
void DrawDashboard(const char *username, sqlite3 *db);
void HandleTaskInput(bool *focused, char input[], int maxLength);
void DrawTasks(Task tasks[], int taskCount, sqlite3 *db);

int main(void) {
    InitWindow(800, 600, "Task Manager");
    SetTargetFPS(60);

    sqlite3 *db;
    if (sqlite3_open("users.db", &db)) {
        printf("Failed to open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // Create the tasks table
    const char *createTaskTableQuery =
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY, "
        "username TEXT, "
        "title TEXT, "
        "completed INTEGER);";
    sqlite3_exec(db, createTaskTableQuery, NULL, NULL, NULL);

    char loggedInUsername[MAX_INPUT_LEN] = "testuser"; // Simulated logged-in user
    ScreenState currentScreen = SCREEN_DASHBOARD;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (currentScreen == SCREEN_DASHBOARD) {
            DrawDashboard(loggedInUsername, db);
        }

        EndDrawing();
    }

    sqlite3_close(db);
    CloseWindow();
    return 0;
}

// Add a new task
void AddTask(const char *username, const char *title, sqlite3 *db) {
    const char *insertQuery = "INSERT INTO tasks (username, title, completed) VALUES (?, ?, 0);";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, insertQuery, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, title, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// Fetch tasks for a specific user
int FetchTasks(const char *username, Task tasks[], sqlite3 *db) {
    const char *selectQuery = "SELECT id, title, completed FROM tasks WHERE username = ?;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, selectQuery, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    int taskCount = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && taskCount < MAX_TASKS) {
        tasks[taskCount].id = sqlite3_column_int(stmt, 0);
        strcpy(tasks[taskCount].title, (const char *)sqlite3_column_text(stmt, 1));
        tasks[taskCount].completed = sqlite3_column_int(stmt, 2);
        taskCount++;
    }
    sqlite3_finalize(stmt);
    return taskCount;
}

// Mark a task as completed
void MarkTaskComplete(int taskId, sqlite3 *db) {
    const char *updateQuery = "UPDATE tasks SET completed = 1 WHERE id = ?;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, updateQuery, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, taskId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// Delete a task
void DeleteTask(int taskId, sqlite3 *db) {
    const char *deleteQuery = "DELETE FROM tasks WHERE id = ?;";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, deleteQuery, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, taskId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// Handle task input box
void HandleTaskInput(bool *focused, char input[], int maxLength) {
    if (*focused && IsKeyPressed(KEY_BACKSPACE) && strlen(input) > 0) {
        input[strlen(input) - 1] = '\0';
    } else if (*focused) {
        int key = GetCharPressed();
        if (key > 0 && key < 128 && strlen(input) < maxLength - 1) {
            input[strlen(input)] = (char)key;
            input[strlen(input) + 1] = '\0';
        }
    }
}

// Draw the dashboard
void DrawDashboard(const char *username, sqlite3 *db) {
    static Task tasks[MAX_TASKS];
    static int taskCount = 0;
    static char newTaskTitle[MAX_INPUT_LEN] = "";
    static bool taskInputFocused = false;

    DrawText(TextFormat("Welcome, %s!", username), 20, 20, 30, DARKGRAY);

    // Task input box
    DrawRectangle(20, 80, 400, 40, LIGHTGRAY);
    if (taskInputFocused) {
        DrawRectangleLines(20, 80, 400, 40, BLUE);
    } else {
        DrawRectangleLines(20, 80, 400, 40, GRAY);
    }
    DrawText(strlen(newTaskTitle) > 0 ? newTaskTitle : "Enter new task title...", 25, 90, 20, strlen(newTaskTitle) > 0 ? BLACK : GRAY);

    // Add Task button
    bool addTaskHovered = GetMouseX() > 440 && GetMouseX() < 540 && GetMouseY() > 80 && GetMouseY() < 120;
    Color addTaskColor = addTaskHovered ? DARKGRAY : LIGHTGRAY;
    DrawRectangle(440, 80, 100, 40, addTaskColor);
    DrawText("Add Task", 450, 90, 20, BLACK);

    // Handle input focus and button clicks
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (GetMouseX() > 20 && GetMouseX() < 420 && GetMouseY() > 80 && GetMouseY() < 120) {
            taskInputFocused = true;
        } else {
            taskInputFocused = false;
        }
        if (addTaskHovered && strlen(newTaskTitle) > 0) {
            AddTask(username, newTaskTitle, db);
            memset(newTaskTitle, 0, sizeof(newTaskTitle));
            taskCount = FetchTasks(username, tasks, db);
        }
    }

    HandleTaskInput(&taskInputFocused, newTaskTitle, MAX_INPUT_LEN);

    // Fetch and display tasks
    if (taskCount == 0) {
        taskCount = FetchTasks(username, tasks, db);
    }
    DrawTasks(tasks, taskCount, db);
}

// Draw tasks and their actions
void DrawTasks(Task tasks[], int taskCount, sqlite3 *db) {
    for (int i = 0; i < taskCount; i++) {
        Color textColor = tasks[i].completed ? GRAY : BLACK;
        DrawText(tasks[i].title, 50, 150 + i * 50, 20, textColor);

        // Complete button
        DrawRectangle(600, 150 + i * 50, 60, 40, LIGHTGRAY);
        DrawText("Done", 610, 160 + i * 50, 20, DARKGRAY);

        // Delete button
        DrawRectangle(670, 150 + i * 50, 60, 40, RED);
        DrawText("Del", 685, 160 + i * 50, 20, WHITE);

        bool completeHovered = GetMouseX() > 600 && GetMouseX() < 660 && GetMouseY() > (150 + i * 50) && GetMouseY() < (190 + i * 50);
        bool deleteHovered = GetMouseX() > 670 && GetMouseX() < 730 && GetMouseY() > (150 + i * 50) && GetMouseY() < (190 + i * 50);

        if (completeHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            MarkTaskComplete(tasks[i].id, db);
            taskCount = FetchTasks("testuser", tasks, db);
        }

        if (deleteHovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            DeleteTask(tasks[i].id, db);
            taskCount = FetchTasks("testuser", tasks, db);
        }
    }
}
