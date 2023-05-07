#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h> //Winsock ������� include. WSADATA �������.��
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#define MAX_SIZE 1024
using std::cout;
using std::cin;
using std::endl;
using std::string;
SOCKET client_sock;
string my_nick;
string id, password;
bool isServerConnected = false;
bool is_login;

int chat_recv() {
    char buf[MAX_SIZE] = {};
    string msg;
    while (isServerConnected) {
        ZeroMemory(&buf, MAX_SIZE);
        if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
            msg = buf;
            std::stringstream ss(msg);
            string user;
            ss >> user;
            if (user != my_nick)
                cout << buf << endl;
        }
        else {
            cout << "서버와의 연결이 끊어졌습니다." << endl;
            break;
        }
    }
    return 0;
}

int result_recv() {
    cout << "result_recv active" << endl;
    char buf[MAX_SIZE] = {};
    string msg;
    while (isServerConnected) {
        cout << "result_recv active2" << endl;
        ZeroMemory(&buf, MAX_SIZE);
        cout << "result_recv active2.5" << endl;
        if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
            cout << "result_recv active3" << endl;
            msg = buf;
            std::stringstream ss(msg);
            string result;
            ss >> result;
            if (result == "Success") {
            is_login = true;
        }
            cout << buf << endl;
            break;
        }
        else {
            cout << "서버와의 연결이 끊어졌습니다." << endl;
            break;
        }
    }
    return 0;
}


int main() {
    WSADATA wsa;
    int code = WSAStartup(MAKEWORD(2, 2), &wsa);
    client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN client_addr = {};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(2304);
    InetPton(AF_INET, TEXT("127.0.0.1"), &client_addr.sin_addr);

    while (1) {
        if (!connect(client_sock, (SOCKADDR*)&client_addr, sizeof(client_addr))) {

            break;
        }
        cout << "Connecting..." << endl;
    }
    isServerConnected = true;

    if (!code) {
        cout << "이미 아이디가 있으시면 1, 회원가입을 원하시면 2번을 눌러주세요.";
        int sign;
        cin >> sign;
        if (sign == 1) {
            my_nick = "*login*";
            cout << "로그인을 해주세요" << endl;

            while (true) {
                cout << "아이디를 입력해주세요 : ";
                cin >> id;
                cout << "비밀번호를 입력해주세요 : ";
                cin >> password;

                send(client_sock, (my_nick + ' ' + id + ' ' + password).c_str(), (my_nick + ' ' + id + ' ' + password).length(), 0);
                result_recv();

                if (is_login) break;
            }
        }




        else if (sign == 2) {
            cout << "회원가입 페이지입니다." << endl;

            while (true) {


                cout << "아이디를 입력해주세요 :";
                cin >> id;
                cout << "비밀번호를 입력해주세요 :";
                cin >> password;
                cout << "사용할 닉네임 입력 : ";
                cin >> my_nick;

                // send(client_sock, (my_nick + ' ' + id + ' ' + password).c_str(), (my_nick + ' ' + id + ' ' + password).length(), 0);

                send(client_sock, (my_nick + ' ' + id + ' ' + password).c_str(), (my_nick + ' ' + id + ' ' + password).length(), 0);
                result_recv();

                if (is_login) break;
            }
    }


        std::thread th2(chat_recv);
        while (1) {
            string text, temp, receiver;
            const char* buffer;
            std::getline(cin, text);
            std::stringstream text_inspect(text);
            
            int word_count = 0;
            bool is_whisper = false;
            while (text_inspect >> temp) {
                if (temp == "to:" or temp == "TO:" or temp == "to" or temp == "TO") {
                    is_whisper = true;
                    text_inspect >> receiver;
                    text = "";
                    std::getline(text_inspect, text);
                }
                else {
                    break;
                    
                }
            }
            if (is_whisper) {
                buffer = (id + " Y " + receiver + text).c_str();
            }
            else {
                buffer = (id + " N " + "*" + " " + text).c_str();
            }

            
            send(client_sock, buffer, strlen(buffer), 0);
        }
        isServerConnected = false;
        th2.join();
        closesocket(client_sock);
    }
    WSACleanup();
    return 0;
}