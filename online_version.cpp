#include <curses.h>
#include <stdlib.h>
#include <limits.h>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;

constexpr int BLACK  = 1;
constexpr int WHITE  = 2;
constexpr int EMPTY  = 4;
constexpr int CHOSEN = 8;

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

    void arbitrary_move(int y, int x) {
        board[user_cord.first][user_cord.second] &= ~CHOSEN;

        if (y < 0 || y > 7 || x < 0 || x > 7) {
            return;
        }

        user_cord.first  = y;
        user_cord.second = x;

        board[user_cord.first][user_cord.second] |= CHOSEN;
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

    vector<string> get_cord() {
        return { to_string(user_cord.first), to_string(user_cord.second) };
    }

    vector<vector<int>> get_board() {
        return board;
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
    enum                { STABLE = 16 , UNSTABLE = 32 };
};

/* ---------------------------------------------------------------------------------------------------------------- */

class Ai {
public:
    Ai(): board(), visited() {}

    pair<int, int> move(Board& B) {
        board = B.get_board();
        vector<vector<int>>(8, vector<int>(8, -1)).swap(visited);
        return easy_ai();
    }

private:
    pair<int, int> easy_ai() {
        for (int i=0; i<8; i++) {
            for (int j=0; j<8; j++) {
                if (board[i][j] & BLACK) {
                    if (i > 0             &&  board[i-1][j  ] & EMPTY  &&  visited[i-1][j  ] == -1) { visited[i-1][j  ] = simulate(i-1, j  ); }
                    if (i < 7             &&  board[i+1][j  ] & EMPTY  &&  visited[i+1][j  ] == -1) { visited[i+1][j  ] = simulate(i+1, j  ); }
                    if (j > 0             &&  board[i  ][j-1] & EMPTY  &&  visited[i  ][j-1] == -1) { visited[i  ][j-1] = simulate(i  , j-1); }
                    if (j < 7             &&  board[i  ][j+1] & EMPTY  &&  visited[i  ][j+1] == -1) { visited[i  ][j+1] = simulate(i  , j+1); }
                    if ((i > 0 && j > 0)  &&  board[i-1][j-1] & EMPTY  &&  visited[i-1][j-1] == -1) { visited[i-1][j-1] = simulate(i-1, j-1); }
                    if ((i > 0 && j < 7)  &&  board[i-1][j+1] & EMPTY  &&  visited[i-1][j+1] == -1) { visited[i-1][j+1] = simulate(i-1, j+1); }
                    if ((i < 7 && j > 0)  &&  board[i+1][j-1] & EMPTY  &&  visited[i+1][j-1] == -1) { visited[i+1][j-1] = simulate(i+1, j-1); }
                    if ((i > 7 && j > 7)  &&  board[i+1][j+1] & EMPTY  &&  visited[i+1][j+1] == -1) { visited[i+1][j+1] = simulate(i+1, j+1); }
                } } }

        pair<int, int> res = make_pair(INT_MAX, INT_MAX);
        int temp = 0;

        for (int i=0; i<8; i++) {
            for (int j=0; j<8; j++) {
                if (visited[i][j] > temp) {
                    temp = visited[i][j];
                    res  = make_pair(i, j);
                } } }

        return res;
    }

    int simulate(int Y, int X) {
        board[Y][X] &= ~EMPTY;
        board[Y][X] |=  WHITE;
        int res = 0;

        // up
        if (Y > 0) {
            for (int i=Y-1; i >= 0; i--) {
                if (board[i][X] & EMPTY) {
                    break;
                }
                if (board[i][X] & WHITE) {
                    if (Y - i == 1) { break; }
                    res += Y - i - 1;
                    break;
                }
            } }

        // down
        if (Y < 7) {
            for (int i=Y+1; i < 8; i++) {
                if (board[i][X] & EMPTY) {
                    break;
                }
                if (board[i][X] & WHITE) {
                    if (i - Y == 1) { break; }
                    res += i - Y - 1;
                    break;
                }
            } }

        // left
        if (X > 0) {
            for (int i=X-1; i >= 0; i--) {
                if (board[Y][i] & EMPTY) {
                    break;
                }
                if (board[Y][i] & WHITE) {
                    if ( X - i == 1) { break; }
                    res += X - i - 1;
                    break;
                }
            } }

        // right
        if (X < 7) {
            for (int i=X+1; i < 8; i++) {
                if (board[Y][i] & EMPTY) {
                    break;
                }
                if (board[Y][i] & WHITE) {
                    if (i - X == 1) { break; }
                    res += i - X - 1;
                    break;
                }
            } }

        // up-left
        if (Y > 0 && X > 0) {
            for (int i=1; Y-i >= 0 && X-i >= 0; i++) {
                if (board[Y-i][X-i] & EMPTY) {
                    break;
                }
                if (board[Y-i][X-i] & WHITE) {
                    if (i == 1) { break; }
                    res += i - 1;
                    break;
                }
            } }

        // down-right
        if (Y < 7 && X < 7) {
            for (int i=1; Y+i < 8 && X+i < 8; i++) {
                if (board[Y+i][X+i] & EMPTY) {
                    break;
                }
                if (board[Y+i][X+i] & WHITE) {
                    if (i == 1) { break; }
                    res += i - 1;
                    break;
                }
            } }

        // up-right
        if (Y > 0 && X < 7) {
            for (int i=1; Y-i >= 0 && X+i < 8; i++) {
                if (board[Y-i][X+i] & EMPTY) {
                    break;
                }
                if (board[Y-i][X+i] & WHITE) {
                    if (i == 1) { break; }
                    res += i - 1;
                    break;
                }
            } }

        // down-left
        if (Y < 7 && X > 0) {
            for (int i=1; Y+i < 8 && X-i >= 0; i++) {
                if (board[Y+i][X-i] & EMPTY) {
                    break;
                }
                if (board[Y+i][X-i] & WHITE) {
                    if (i == 1) { break; }
                    res += i - 1;
                    break;
                }
            } }

        board[Y][X] &= ~WHITE;
        board[Y][X] |=  EMPTY;
        return res;
    }

    vector<vector<int>> board;
    vector<vector<int>> visited;
};

/* ---------------------------------------------------------------------------------------------------------------- */

class Othello: public enable_shared_from_this {
typedef boost::asio::ip::tcp    tcp;
typedef shared_ptr<tcp::socket> socket_ptr;
public:
    Othello()
    : board(),
      ai(),
      has_ai(false),
      is_online(false),
      timer(ios),
      timer_deadline(180),
      can_play(false) {}

    bool main_loop() {
        if (! user_interface()) { return false; }

        if (is_online && !init_connection()) { return connection_error(); }

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
                if (is_online  &&  ! can_play) {
                    break;
                }
                if (! board.set()) {
                    if (is_online) { send(LOSE, vector<string>()); }
                    return gameover();
                }
                if (has_ai) {
                    pair<int, int> temp = ai.move(board);
                    if (temp.first == INT_MAX && temp.second == INT_MAX) {
                        gameover();
                    }

                    board.arbitrary_move(temp.first, temp.second);
                    if (! board.set()) {
                        return gameover();
                    }
                }
                if (is_online) {
                    can_play = false;
                    vector<string> cmd = board.get_cord();
                    send(MOVE, move(cmd));
                    vector<string> cmd = receive();
                    if (! cmd.empty()  &&  command_handler(cmd)) {
                        can_play = true;
                    } else {
                        return connection_error();
                    }
                }
                break;

            case 'q':
            case 'Q':
                return false;

            default: break;
            }
        }
    }

private:
    bool user_interface() {
        clear();
        mvaddstr(2 * 3, 4 * 6, "Please choose a mode");
        mvaddstr(4 * 3, 5 * 6, "1. Human VS Human");
        mvaddstr(5 * 3, 5 * 6, "2. Human VS PC");
        mvaddstr(6 * 3, 5 * 6, "3. Online Battle with others");
        mvaddstr(7 * 3, 5 * 6, "Press 1, 2, 3 to choose a mode or q to quit");

        char c;
        while (c = getch()) {
            switch (c) {
            case '1':
                has_ai    = false;
                is_online = false;
                return true;

            case '2':
                has_ai    = true;
                is_online = false;
                return true;

            case '3':
                has_ai    = false;
                is_online = true;
                return true;

            case 'q':
            case 'Q':
                return false;
            }
        }
    }

    bool init_connection() {
        sock = new tcp::socket(ios);

        tcp::resolver resolver(ios);
        tcp::resolver::results_type endpoints = resolver.resolve(tcp::resolver::query("www.reidblog.cn",
                                                                                      boost::lexical_cast<string>(9000)));
        boost::system::error_code ec;
        boost::asio::connect(*sock, endpoints, ec);
        if (ec) {
            return false;
        }

        send(ADD, vector<string>());
        vector<string> command = receive();
        if (!command.empty()) {
            return command_handler(move(command));
        } else {
            return false;
        }
    }

    bool gameover() {
        clear();
        mvaddstr(4 * 3, 4 * 6, "game over, wanna start again? (Y/N)");
        char c;
        while (c = getch()) {
            if (c == 'Y' || c == 'y') { return true; }
            else if (c == 'N' || c == 'n') { return false; }
        }
    }

    bool connection_error() {
        sock.reset();
        clear();
        mvaddstr(4 * 3, 4 * 6, "connection error, wanna try again? (Y/N)");
        char c;
        while (c = getch()) {
            if (c == 'Y' || c == 'y') { return true; }
            else if (c == 'N' || c == 'n') { return false; }
        }
    }

    bool send(int flag, vector<string>&& args) {
        boost::system::error_code ec;

        string message = to_string(flag);
        for (int i=0; i<args.size(); i++) {
            message += args[i];
        }
        char* msg; msg = const_cast<char*>(message.c_str());

        boost::asio::write(*sock, boost::asio::buffer(msg, sizeof(msg)), ec);
        if (ec) {
            return false;
        }
        return true;
    }

    vector<string> receive() {
        vector<string> command;
        boost::asio::streambuf streambuf;
        boost::system::error_code ec;
        static string delimeters = "\n";

        while (boost::asio::read_until(*sock, streambuf, delimeters, ec)) {
            boost::asio::streambuf::const_buffers_type raw_data = streambuf.data();
            string                                     content(boost::asio::buffers_begin(raw_data),
                                                               boost::asio::buffers_begin(raw_data) + streambuf.size() - delimeters.size());

            if (! content.empty()) {
                command.push_back(content);
            }

            streambuf.consume(streambuf.size());
        }

        return command;
    }

    bool command_handler(vector<string>&& command) {
        if (command[0] == to_string((int)PLAYER1)) {
            online_id = PLAYER1;
            timer.cancel();
            timer_deadline = 180;
            can_play = true;
            return true;
        }
        else if (command[0] == to_string((int)PLAYER2)) {
            online_id = PLAYER2;
            timer.cancel();
            timer_deadline = 180;
            can_play = false;
            return true;
        }
        else if (command[0] == to_string((int)WAITING)) {
            timer.expires_after(chrono::seconds(1));
            timer.async_wait(boost::bind(&Othello::timer_handler, shared_from_this(), timer_deadline, placeholders::_1));
            return true;
        }
        else if (command[0] == to_string((int)LEAVE)) {
            return false;
        }
        else if (command[0] == to_string((int)MOVE)) {
            board.arbitrary_move(command[1], command[2]);
            if (! board.set()) {
                send(LEAVE, vector<string>());
                return false;
            }
            can_play = true;
            return true;
        }
        else if (command[0] == to_string((int)WIN)) {
            return false;
        }
        else if (command[0] == to_string((int)LOSE)) {
            return false;
        }
        else if (command[0] == to_string((int)TIME_OUT)) {
            return false;
        }
        else if (command[0] == to_string((int)SERVER_ERROR)) {
            return false;
        }
        else if (command[0] == to_string((int)MSG_ERROR)) {
            return false;
        }

        return false;
    }

    void timer_handler(int& deadline, boost::system::error_code& ec) {
        if (deadline == 0) {
            send(LEAVE, vector<string>());
            clear();
            mvaddstr(4 * 3, 4 * 6, "No players found, press Ctrl + C to exit...");
            return;
        }

        clear();
        mvaddstr(4 * 3, 4 * 6, "Waiting for other players to join...");
        mvaddstr(4 * 3, 4 * 6, (to_string(deadline) + "s left").c_str());

        deadline--;
        timer.expires_after(chrono::seconds(1));
        timer.async_wait(boost::bind(&Othello::timer_handler, shared_from_this(), timer_deadline, placeholders::_1));
    }

    Board                       board;
    Ai                          ai;
    bool                        has_ai;
    bool                        is_online;
    boost::asio::io_service     ios;
    socket_ptr                  sock;
    int                         online_id;
    boost::asio::steady_timer   timer;
    int                         timer_deadline;
    bool                        can_play;

    enum {
        ADD  = 0    , LEAVE    , MOVE   , WIN    , LOSE   , TIME_OUT,
        SERVER_ERROR, MSG_ERROR, PLAYER1, PLAYER2, WAITING
    };
};

int main() {
    Othello othello;
    while (othello.main_loop()) { continue; }
}
