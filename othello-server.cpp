#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/bind/placeholders.hpp>
#include <queue>
#include <vector>
#include <memory>
#include <iostream>
#include <chrono>


class OthelloServer: public std::enable_shared_from_this<OthelloServer> {
typedef boost::asio::ip::tcp         tcp;
typedef std::shared_ptr<tcp::socket> socket_ptr;
public:
    OthelloServer()
     : thread_pool(2),
       timer1(ios),
       timer2(ios),
       duration(3),
       acceptor(ios),
       endpoint(tcp::v4(), 9000),
       delimeters("\n") {}
    ~OthelloServer() { thread_pool.join(); }

    void main_loop() {
    /* 1. accept a new connection
     * 2. pass the accepted socket to the next step -> read
     */
        socket_ptr sock = std::make_shared<tcp::socket>(ios);
        boost::system::error_code ec;

        acceptor.accept(*sock, ec);

        if (ec) {
            server_error(std::move(ec));
            return;
        }

        boost::asio::post(thread_pool, boost::bind(&OthelloServer::socket_handler, shared_from_this(), sock));
    }

    void run() {
        ios.run();
    }

private:
    void socket_handler(std::shared_ptr<tcp::socket> sock) {
    /* 1. read from the socket, the message is seperated by delimeter: \n
     * 2. put incomming command and related arguments into a vector, and pass it to the next step -> handle the command
     */
        boost::asio::streambuf    streambuf;
        boost::system::error_code ec;
        std::vector<std::string>  command;

        while (boost::asio::read_until(*sock, streambuf, delimeters, ec)) {
            if (ec) {
                server_error(std::move(ec));
                return;
            }

            boost::asio::streambuf::const_buffers_type raw_data = streambuf.data();
            std::string                                content(boost::asio::buffers_begin(raw_data),
                                                               boost::asio::buffers_begin(raw_data) + streambuf.size() - delimeters.size());

            command.push_back(content);
            streambuf.consume(streambuf.size());
        }

        if (command.empty()   ||   ! command_handler(std::move(command), sock, ec)) {
            char* msg; msg = const_cast<char*>(std::to_string((int)MSG_ERROR).c_str());
            boost::asio::write(*sock, boost::asio::buffer(msg, sizeof(msg)), ec);
        }
    }

    void server_error(boost::system::error_code&& ec) {
    /* 1. generate the error message based on error_code's content.
     *    the message is in the form of:
     *        OthelloServer::SERVER_ERROR + \n + ec::what()
     * 2. send the message to player1 and player2
     * 3. print the message onto the screen
     */
        char* message; message = const_cast<char*>((std::to_string((int)SERVER_ERROR) + "\n" + ec.message()).c_str());

        boost::asio::write(*player1, boost::asio::buffer(message, sizeof(message)), ec);
        boost::asio::write(*player2, boost::asio::buffer(message, sizeof(message)), ec);
    }

    bool command_handler(std::vector<std::string>&& command, std::shared_ptr<tcp::socket> sock, boost::system::error_code& ec) {
        /* ADD */
        if (command[0] == std::to_string((int)ADD)) {
        /* 1. push the player into the queue
         * 2. check if the queue has 2 players
         *        if (has 2 players) {
         *            pop 2 players out as player1 and player2;
         *            restart the socket_handler for both players to read subsequent instruction;
         *        } else {
         *            tell the incoming player to wait;
         *            restart the main_loop waiting for other connections;
         *        }
         */
            if (command.size() != 1) {
                return false;
            }

            socket_queue.push(sock);

            if (socket_queue.size() == 2) {
                player1.reset(); player1 = socket_queue.front(); socket_queue.pop();
                player2.reset(); player2 = socket_queue.front(); socket_queue.pop();

                char* msg1; msg1 = const_cast<char*>(std::to_string((int)PLAYER1).c_str());
                char* msg2; msg2 = const_cast<char*>(std::to_string((int)PLAYER2).c_str());

                boost::asio::write(*player1, boost::asio::buffer(msg1, sizeof(msg1)), ec);
                boost::asio::write(*player2, boost::asio::buffer(msg2, sizeof(msg2)), ec);

                boost::asio::post(thread_pool, boost::bind(&OthelloServer::socket_handler, shared_from_this(), player1));
                boost::asio::post(thread_pool, boost::bind(&OthelloServer::socket_handler, shared_from_this(), player2));

                timer1.expires_after(duration);
                timer2.expires_after(duration);
                timer1.async_wait(boost::bind(&OthelloServer::timer_handler, shared_from_this(), 1, boost::placeholders::_1));
                timer2.async_wait(boost::bind(&OthelloServer::timer_handler, shared_from_this(), 2, boost::placeholders::_1));
            } else {
                char* msg; msg = const_cast<char*>(std::to_string((int)WAITING).c_str());
                boost::asio::write(*sock   , boost::asio::buffer(msg , sizeof(msg)) , ec);

                boost::asio::post(thread_pool, boost::bind(&OthelloServer::main_loop     , shared_from_this()));
            }

            return true;
        }

        /* LEAVE */
        else if (command[0] == std::to_string((int)LEAVE)) {
        /* 1. send LEAVE message to another player
         * 2. restart the main_loop waiting for other connections
         */
            if (command.size() != 1) {
                return false;
            }

            /* LEAVE PLAYER1(PLAYER2) */
            char* message; message = const_cast<char*>(std::to_string((int)LEAVE).c_str());

            if (sock == player2) {
                boost::asio::write(*player1, boost::asio::buffer(message, sizeof(message)), ec);
            } else if (sock == player1) {
                boost::asio::write(*player2, boost::asio::buffer(message, sizeof(message)), ec);
            } else {
                while (!socket_queue.empty()) { socket_queue.pop(); }
            }

            timer1.cancel();
            timer2.cancel();

            boost::asio::post(thread_pool, boost::bind(&OthelloServer::main_loop, shared_from_this()));

            return true;
        }

        /* MOVE y x*/
        else if (command[0] == std::to_string((int)MOVE)) {
        /* 1. send the message to both players
         * 2. restart the socket_handler for the current player
         */
            if (command.size() != 3) {
                return false;
            }

            /* MOVE PLAYER1(PLAYER2) y x */
            char* message; message = const_cast<char*>((std::to_string((int)MOVE) + "\n" + command[1] + "\n" + command[2]).c_str());
            if (sock == player2) {
                boost::asio::write(*player1, boost::asio::buffer(message, sizeof(message)), ec);
                timer2.expires_after(duration);
                timer2.async_wait(boost::bind(&OthelloServer::timer_handler, shared_from_this(), 2, boost::placeholders::_1));
            } else {
                boost::asio::write(*player2, boost::asio::buffer(message, sizeof(message)), ec);
                timer1.expires_after(duration);
                timer1.async_wait(boost::bind(&OthelloServer::timer_handler, shared_from_this(), 1, boost::placeholders::_1));
            }

            boost::asio::post(thread_pool, boost::bind(&OthelloServer::socket_handler, shared_from_this(), sock));

            return true;
        }

        /* WIN */
        else if (command[0] == std::to_string((int)WIN)) {
            if (command.size() != 1) {
                return false;
            }

            char* lose_msg; lose_msg = const_cast<char*>(std::to_string((int)LOSE).c_str());

            if (sock == player1) {
                boost::asio::write(*player2, boost::asio::buffer(lose_msg, sizeof(lose_msg)), ec);
            } else {
                boost::asio::write(*player1, boost::asio::buffer(lose_msg, sizeof(lose_msg)), ec);
            }

            timer1.cancel();
            timer2.cancel();

            boost::asio::post(thread_pool, boost::bind(&OthelloServer::main_loop, shared_from_this()));

            return true;
        }

        /* LOSE */
        else if (command[0] == std::to_string((int)LOSE)) {
            if (command.size() != 1) {
                return false;
            }

            const char* win_msg; win_msg = const_cast<char*>(std::to_string((int)WIN).c_str());

            if (sock == player1) {
                boost::asio::write(*player2, boost::asio::buffer(win_msg , sizeof(win_msg)) , ec);
            } else {
                boost::asio::write(*player1, boost::asio::buffer(win_msg , sizeof(win_msg)) , ec);
            }

            timer1.cancel();
            timer2.cancel();

            boost::asio::post(thread_pool, boost::bind(&OthelloServer::main_loop, shared_from_this()));

            return true;
        }

        return false;
    }

    void timer_handler(int flag, boost::system::error_code ec) {
        char* message; message = const_cast<char*>(std::to_string((int)TIME_OUT).c_str());

        if (flag == 1) {
            boost::asio::write(*player2, boost::asio::buffer(message, sizeof(message)), ec);
        } else if (flag == 2) {
            boost::asio::write(*player1, boost::asio::buffer(message, sizeof(message)), ec);
        }

        timer1.cancel();
        timer2.cancel();

        boost::asio::post(thread_pool, boost::bind(&OthelloServer::main_loop, shared_from_this()));
    }

    boost::asio::io_service     ios;
    boost::asio::thread_pool    thread_pool;
    boost::asio::steady_timer   timer1;
    boost::asio::steady_timer   timer2;
    std::chrono::minutes        duration;
    tcp::acceptor               acceptor;
    tcp::endpoint               endpoint;
    std::queue<socket_ptr>      socket_queue;
    socket_ptr                  player1;
    socket_ptr                  player2;
    std::string                 delimeters;

    enum {
        ADD  = 0    , LEAVE    , MOVE   , WIN    , LOSE   , TIME_OUT,
        SERVER_ERROR, MSG_ERROR, PLAYER1, PLAYER2, WAITING
    };
};

int main() {
    OthelloServer server;
    server.main_loop();
    server.run();
}
