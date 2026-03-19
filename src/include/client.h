#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <utility>
#include <vector>
#include <set>
#include <random>
#include <algorithm>

extern int rows;         // The count of rows of the game map.
extern int columns;      // The count of columns of the game map.
extern int total_mines;  // The count of mines of the game map.

// You MUST NOT use any other external variables except for rows, columns and total_mines.

// Global game state
std::vector<std::vector<char>> visible_map;      // '?' = unknown, 'X' = mine visited, digit = visited number, '@' = marked mine
std::set<std::pair<int, int>> safe_cells;       // Cells we know are safe
std::set<std::pair<int, int>> known_mines;      // Cells we know are mines
std::set<std::pair<int, int>> flagged_cells;    // Cells we've marked as mines
std::vector<std::pair<int, int>> moves_to_make; // List of moves to execute
std::mt19937 rng;                               // Random number generator for fallback

/**
 * @brief The definition of function Execute(int, int, bool)
 *
 * @details This function is designed to take a step when player the client's (or player's) role, and the implementation
 * of it has been finished by TA. (I hope my comments in code would be easy to understand T_T) If you do not understand
 * the contents, please ask TA for help immediately!!!
 *
 * @param r The row coordinate (0-based) of the block to be visited.
 * @param c The column coordinate (0-based) of the block to be visited.
 * @param type The type of operation to a certain block.
 * If type == 0, we'll execute VisitBlock(row, column).
 * If type == 1, we'll execute MarkMine(row, column).
 * If type == 2, we'll execute AutoExplore(row, column).
 * You should not call this function with other type values.
 */
void Execute(int r, int c, int type);

/**
 * @brief The definition of function InitGame()
 *
 * @details This function is designed to initialize the game. It should be called at the beginning of the game, which
 * will read the scale of the game map and the first step taken by the server (see README).
 */
void InitGame() {
  // TODO (student): Initialize all your global variables!

  // Read the first move
  int first_row, first_column;
  std::cin >> first_row >> first_column;

  // Initialize random generator
  rng.seed(std::random_device{}());

  // Make the first move
  Execute(first_row, first_column, 0);
}

/**
 * @brief The definition of function ReadMap()
 *
 * @details This function is designed to read the game map from stdin when playing the client's (or player's) role.
 * Since the client (or player) can only get the limited information of the game map, so if there is a 3 * 3 map as
 * above and only the block (2, 0) has been visited, the stdin would be
 *     ???
 *     12?
 *     01?
 */
void ReadMap() {
  visible_map.assign(rows, std::vector<char>(columns));

  for (int i = 0; i < rows; i++) {
    std::string row;
    std::cin >> row;
    for (int j = 0; j < columns; j++) {
      visible_map[i][j] = row[j];

      // Update our knowledge based on what's visible
      if (row[j] >= '0' && row[j] <= '8') {
        // It's a visited number, add as safe
        safe_cells.insert({i, j});
      } else if (row[j] == '@') {
        // It's marked as a mine
        flagged_cells.insert({i, j});
        known_mines.insert({i, j});
      }
    }
  }
}

/**
 * @brief The definition of function Decide()
 *
 * @details This function is designed to decide the next step when playing the client's (or player's) role. Open up your
 * mind and make your decision here! Caution: you can only execute once in this function.
 */
void Decide() {
  // Strategy 1: Look for cells where it's safe to click
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      // Check visited cells with numbers
      if (visible_map[i][j] >= '0' && visible_map[i][j] <= '8') {
        int number = visible_map[i][j] - '0';
        int unknown_neighbors = 0;
        int flagged_neighbors = 0;

        // Count neighbors
        for (int di = -1; di <= 1; di++) {
          for (int dj = -1; dj <= 1; dj++) {
            if (di == 0 && dj == 0) continue;
            int ni = i + di, nj = j + dj;
            if (ni >= 0 && ni < rows && nj >= 0 && nj < columns) {
              if (visible_map[ni][nj] == '?') {
                unknown_neighbors++;
              }
              if (visible_map[ni][nj] == '@') {
                flagged_neighbors++;
              }
            }
          }
        }

        // Rule 1: If number equals flagged count and there are unknowns, mark all unknowns
        if (number == flagged_neighbors && unknown_neighbors > 0) {
          for (int di = -1; di <= 1; di++) {
            for (int dj = -1; dj <= 1; dj++) {
              if (di == 0 && dj == 0) continue;
              int ni = i + di, nj = j + dj;
              if (ni >= 0 && ni < rows && nj >= 0 && nj < columns && visible_map[ni][nj] == '?') {
                Execute(ni, nj, 0); // Visit the safe cell
                return;
              }
            }
          }
        }

        // Rule 2: If number minus flagged equals unknown count, all unknowns are mines
        if (number - flagged_neighbors == unknown_neighbors && unknown_neighbors > 0) {
          for (int di = -1; di <= 1; di++) {
            for (int dj = -1; dj <= 1; dj++) {
              if (di == 0 && dj == 0) continue;
              int ni = i + di, nj = j + dj;
              if (ni >= 0 && ni < rows && nj >= 0 && nj < columns && visible_map[ni][nj] == '?') {
                Execute(ni, nj, 1); // Mark as mine
                return;
              }
            }
          }
        }
      }
    }
  }

  // Strategy 2: Try auto-explore on numbered cells with enough flags
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (visible_map[i][j] >= '0' && visible_map[i][j] <= '8') {
        int number = visible_map[i][j] - '0';
        int flagged_neighbors = 0;
        bool has_unknown_neighbors = false;

        for (int di = -1; di <= 1; di++) {
          for (int dj = -1; dj <= 1; dj++) {
            if (di == 0 && dj == 0) continue;
            int ni = i + di, nj = j + dj;
            if (ni >= 0 && ni < rows && nj >= 0 && nj < columns) {
              if (visible_map[ni][nj] == '@') {
                flagged_neighbors++;
              } else if (visible_map[ni][nj] == '?') {
                has_unknown_neighbors = true;
              }
            }
          }
        }

        // If flags match number, try auto-explore
        if (flagged_neighbors == number && has_unknown_neighbors) {
          Execute(i, j, 2); // AutoExplore
          return;
        }
      }
    }
  }

  // Strategy 3: Find a random unknown cell to click
  std::vector<std::pair<int, int>> unknown_cells;
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (visible_map[i][j] == '?') {
        unknown_cells.push_back({i, j});
      }
    }
  }

  if (!unknown_cells.empty()) {
    // Pick a random cell
    std::uniform_int_distribution<int> dist(0, unknown_cells.size() - 1);
    auto [r, c] = unknown_cells[dist(rng)];
    Execute(r, c, 0); // Visit the random cell
    return;
  }
}

#endif