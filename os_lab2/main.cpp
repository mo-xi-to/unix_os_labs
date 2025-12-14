#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <fcntl.h>
#include <cerrno>

using namespace std;

constexpr int PORT = 8080;
constexpr int BUFFER_SIZE = 1024;

volatile sig_atomic_t wasSigHup = 0;
volatile sig_atomic_t wasSigInt = 0;

void sigHupHandler(int) {
    wasSigHup = 1;
}

void sigIntHandler(int) {
    wasSigInt = 1;
}

int main() {
    struct sigaction sa{};
    sa.sa_flags = 0;

    sa.sa_handler = sigHupHandler;
    sigaction(SIGHUP, &sa, nullptr);

    sa.sa_handler = sigIntHandler;
    sigaction(SIGINT, &sa, nullptr);

    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigaddset(&blockedMask, SIGINT);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return 1;
    }

    cout << "Server started on port " << PORT << endl;
    cout << "PID: " << getpid() << endl;

    vector<int> clients;
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    int addrlen = sizeof(address);

    while (!wasSigInt) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int fd : clients) {
            FD_SET(fd, &readfds);
            if (fd > max_sd) max_sd = fd;
        }

        int activity = pselect(max_sd + 1, &readfds, nullptr, nullptr, nullptr, &origMask);

        if (activity < 0 && errno != EINTR) {
            perror("pselect error");
            break;
        }

        if (wasSigInt) {
            cout << "\nSIGINT received. Shutting down..." << endl;
            break;
        }

        if (wasSigHup) {
            cout << "\nSIGHUP received and handled! Server continues..." << endl;
            wasSigHup = 0;
        }

        if (activity < 0 && errno == EINTR) {
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket >= 0) {
                if (clients.empty()) {
                    clients.push_back(new_socket);
                    cout << "Client connected (FD: " << new_socket << ")" << endl;
                } else {
                    cout << "Busy. Rejecting new connection." << endl;
                    close(new_socket);
                }
            }
        }

        for (auto it = clients.begin(); it != clients.end(); ) {
            int fd = *it;
            if (FD_ISSET(fd, &readfds)) {
                ssize_t valread = read(fd, buffer, BUFFER_SIZE);
                if (valread > 0) {
                    cout << "Received " << valread << " bytes from client (FD: " << fd << ")" << endl;
                    ++it;
                } else {
                    cout << "Client disconnected (FD: " << fd << ")" << endl;
                    close(fd);
                    it = clients.erase(it);
                }
            } else {
                ++it;
            }
        }
    }

    for (int fd : clients) {
        close(fd);
    }
    close(server_fd);
    
    cout << "Server stopped cleanly." << endl;

    return 0;
}