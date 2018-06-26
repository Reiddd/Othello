#include <curses.h>
#include <stdlib.h>
#include <vector>
#include <chrono>
#include <thread>

using namespace std;

class Board {
public:
    Board()
    : board() {
        initscr();
        cbreak();
        noecho();
        curs_set(0);

        start_color();
        init_pair(WHITE , COLOR_BLACK, COLOR_WHITE);
        init_pair(CHOSEN, COLOR_WHITE, COLOR_WHITE);

        init();
    }

    ~Board() { endwin(); }

    void init() {
        vector<vector<int>>(8, vector<int>(8, EMPTY)).swap(board);
        board[3][3]  = BLACK ; board[3][4] = WHITE;
        board[4][4]  = BLACK ; board[4][3] = WHITE;
        board[3][5] |= CHOSEN;
        make_pair(3, 5).swap(user_cord);
        black_turn  = true;
        black_count = 2;
        white_count = 2;

        draw();
    }

    void move(int dy, int dx) {
        board[user_cord.first][user_cord.second] &= ~CHOSEN;

        if (dy) {
            if      (dy == -1) { user_cord.first = max(user_cord.first - 1, 0); }
            else if (dy ==  1) { user_cord.first = min(user_cord.first + 1, 7); }
        }
        if (dx) {
            if      (dx ==  1) { user_cord.second = min(user_cord.second + 1, 7); }
            else if (dx == -1) { user_cord.second = max(user_cord.second - 1, 0); }
        }

        board[user_cord.first][user_cord.second] |= CHOSEN;

        draw();
    }

    bool set() {
        if (board[user_cord.first][user_cord.second] &   EMPTY) {
            board[user_cord.first][user_cord.second] &= ~EMPTY;
        }
        else { return true; }

        board[user_cord.first][user_cord.second]     |= black_turn ?  BLACK :  WHITE;
        if (! refresh_board(black_turn)) {
            board[user_cord.first][user_cord.second] &= black_turn ? ~BLACK : ~WHITE;
            board[user_cord.first][user_cord.second] |= EMPTY;
        }
        else {
            black_turn  = !black_turn;
            black_count = count(BLACK);
            white_count = count(WHITE);
            if (black_count == 0 || white_count == 0 || count(EMPTY) == 0) { return false; }
            draw();
        }

        return true;
    }

private:
    void draw() {
        clear();

        for (int i=0; i<8; i++) {
            for (int j=0; j<8; j++) {
                draw_one(i, j, board[i][j]);
            } }

        mvaddstr(0, 9 * 6, black_turn ? "now:   BLACK" : "now:   WHITE");
        mvaddstr(2, 9 * 6, ("BLACK: " + to_string(black_count)).c_str());
        mvaddstr(4, 9 * 6, ("WHITE: " + to_string(white_count)).c_str());

        refresh();
    }

    void draw_one(int i, int j, int piece) {
        int Y = i * 3;
        int X = j * 6;

        mvaddch(Y    , X    , '+'); mvaddch(Y + 3, X    , '+');
        mvaddch(Y    , X + 6, '+'); mvaddch(Y + 3, X + 6, '+');

        for (int y=1; y<=2; y++) {
            mvaddch(Y + y, X    , '|');
            mvaddch(Y + y, X + 6, '|');
        }
        for (int x=1; x<=5; x++) {
            mvaddch(Y    , X + x, '-');
            mvaddch(Y + 3, X + x, '-');
        }
        if (piece & CHOSEN) {
            attron(COLOR_PAIR(CHOSEN));

            for (int y=1; y<=2; y++) {
                mvaddch(Y + y, X + 1, ' ');
                mvaddch(Y + y, X + 5, ' ');
            }

            attroff(COLOR_PAIR(CHOSEN));
        }

        if (piece & BLACK) {
            for (int y=1; y<=2; y++) {
                mvaddch(Y + y, X + 2, '+');
                mvaddch(Y + y, X + 3, ' ');
                mvaddch(Y + y, X + 4, '+');
            }
        }
        else if (piece & WHITE) {
            attron(COLOR_PAIR(WHITE));

            for (int y=1; y<=2; y++) {
                mvaddch(Y + y, X + 2, '+');
                mvaddch(Y + y, X + 3, ' ');
                mvaddch(Y + y, X + 4, '+');
            }

            attroff(COLOR_PAIR(WHITE));
        }
    }

    bool refresh_board(bool black_turn) {
        bool res = false;

        // up
        if (user_cord.first > 0) {
            for (int i=user_cord.first-1; i>=0; i--) {
                if (board[i][user_cord.second] & EMPTY) {
                    break;
                }
                if (board[i][user_cord.second] & board[user_cord.first][user_cord.second] & (BLACK | WHITE)) {
                    if (user_cord.first - i == 1) { break; }

                    res  = true;

                    for (int j = i+1; j<user_cord.first; j++) {
                        board[j][user_cord.second] &= black_turn ? ~WHITE : ~BLACK;
                        board[j][user_cord.second] |= black_turn ?  BLACK :  WHITE;
                    }

                    break;
                }
            } }

        // down
        if (user_cord.first < 7) {
            for (int i=user_cord.first+1; i<8; i++) {
                if (board[i][user_cord.second] & EMPTY) {
                    break;
                }
                if (board[i][user_cord.second] & board[user_cord.first][user_cord.second] & (BLACK | WHITE)) {
                    if (i - user_cord.first == 1) { break; }

                    res = true;

                    for (int j=user_cord.first+1; j<i; j++) {
                        board[j][user_cord.second] &= black_turn ? ~WHITE : ~BLACK;
                        board[j][user_cord.second] |= black_turn ?  BLACK :  WHITE;
                    }

                    break;
                }
            } }

        // left
        if (user_cord.second > 0) {
            for (int i=user_cord.second-1; i>=0; i--) {
                if (board[user_cord.first][i] & EMPTY) {
                    break;
                }
                if (board[user_cord.first][i] & board[user_cord.first][user_cord.second] & (BLACK | WHITE)) {
                    if (user_cord.second - i == 1) { break; }

                    res = true;

                    for (int j=i+1; j<user_cord.second; j++) {
                        board[user_cord.first][j] &= black_turn ? ~WHITE : ~BLACK;
                        board[user_cord.first][j] |= black_turn ?  BLACK :  WHITE;
                    }

                    break;
                }
            } }

        // right
        if (user_cord.second < 7) {
            for (int i=user_cord.second+1; i<8; i++) {
                if (board[user_cord.first][i] & EMPTY) {
                    break;
                }
                if (board[user_cord.first][i] & board[user_cord.first][user_cord.second] & (BLACK | WHITE)) {
                    if (i - user_cord.second == 1) { break; }

                    res = true;

                    for (int j=user_cord.second+1; j<i; j++) {
                        board[user_cord.first][j] &= black_turn ? ~WHITE : ~BLACK;
                        board[user_cord.first][j] |= black_turn ?  BLACK :  WHITE;
                    }

                    break;
                }
            } }

        // up-left
        if (user_cord.first > 0 && user_cord.second > 0) {
            for (int i=1; user_cord.first-i >= 0  &&  user_cord.second-i >= 0; i++) {
                if (board[user_cord.first-i][user_cord.second-i] & EMPTY) {
                    break;
                }
                if (board[user_cord.first-i][user_cord.second-i] & board[user_cord.first][user_cord.second] & (BLACK | WHITE)) {
                    if (i == 1) { break; }

                    res = true;

                    for (int j=1; j<i; j++) {
                        board[user_cord.first-j][user_cord.second-j] &= black_turn ? ~WHITE : ~BLACK;
                        board[user_cord.first-j][user_cord.second-j] |= black_turn ?  BLACK :  WHITE;
                    }

                    break;
                }
            } }

        // down-right
        if (user_cord.first < 7 && user_cord.second < 7) {
            for (int i=1; user_cord.first+i < 8  &&  user_cord.second+i < 8; i++) {
                if (board[user_cord.first+i][user_cord.second+i] & EMPTY) {
                    break;
                }
                if (board[user_cord.first+i][user_cord.second+i] & board[user_cord.first][user_cord.second] & (BLACK | WHITE)) {
                    if (i == 1) { break; }

                    res = true;

                    for (int j=1; j<i; j++) {
                        board[user_cord.first+j][user_cord.second+j] &= black_turn ? ~WHITE : ~BLACK;
                        board[user_cord.first+j][user_cord.second+j] |= black_turn ?  BLACK :  WHITE;
                    }

                    break;
                }
            } }

        // up-right
        if (user_cord.first > 0 && user_cord.second < 7) {
            for (int i=1; user_cord.first-i >= 0  &&  user_cord.second+i < 8; i++) {
                if (board[user_cord.first-i][user_cord.second+i] & EMPTY) {
                    break;
                }
                if (board[user_cord.first-i][user_cord.second+i] & board[user_cord.first][user_cord.second] & (BLACK | WHITE)) {
                    if (i == 1) { break; }

                    res = true;

                    for (int j=1; j<i; j++) {
                        board[user_cord.first-j][user_cord.second+j] &= black_turn ? ~WHITE : ~BLACK;
                        board[user_cord.first-j][user_cord.second+j] |= black_turn ?  BLACK :  WHITE;
                    }

                    break;
                }
            } }

        // down-left
        if (user_cord.first < 7 && user_cord.second >= 0) {
            for (int i=1; user_cord.first+i < 8  &&  user_cord.second-i >= 0; i++) {
                if (board[user_cord.first+i][user_cord.second-i] & EMPTY) {
                    break;
                }
                if (board[user_cord.first+i][user_cord.second-i] & board[user_cord.first][user_cord.second] & (BLACK | WHITE)) {
                    if (i == 1) { break; }

                    res = true;

                    for (int j=1; j<i; j++) {
                        board[user_cord.first+j][user_cord.second-j] &= black_turn ? ~WHITE : ~BLACK;
                        board[user_cord.first+j][user_cord.second-j] |= black_turn ?  BLACK :  WHITE;
                    }

                    break;
                }
            } }

        return res;
    }

    int count(int status) {
        int res = 0;

        for (int i=0; i<8; i++) {
            for (int j=0; j<8; j++) {
                res += board[i][j] & status ? 1 : 0;
            } }

        return res;
    }

    vector<vector<int>> board;
    pair<int, int>      user_cord;
    bool                black_turn;
    int                 black_count;
    int                 white_count;
    enum                { BLACK  = 1  , WHITE    = 2 , EMPTY = 4, CHOSEN = 8 };
    enum                { STABLE = 16 , UNSTABLE = 32 };
};

class Othello {
public:
    Othello()
    : board() {}

    bool main_loop() {
        board.init();

        while (true) {
            char c = getchar();
            switch (c) {
            case 'w':
            case 'W':
                board.move(-1, 0);
                break;

            case 's':
            case 'S':
                board.move(1, 0);
                break;

            case 'a':
            case 'A':
                board.move(0, -1);
                break;

            case 'd':
            case 'D':
                board.move(0, 1);
                break;

            case ' ':
                if (! board.set()){ return gameover(); }
                break;

            case 'q':
            case 'Q':
                return false;

            default: break;
            }
        }
    }
private:
    bool gameover() {
        clear();
        mvaddstr(4 * 3, 4 * 6, "game over, wanna start again? (Y/N)");
        char c = getch();
        if (c == 'Y' || c == 'y') { return true; }
        else                      { return false; }
    }

    Board board;
};

int main() {
    Othello othello;
    while (othello.main_loop()) { continue; }
}
