#pragma comment(lib, "ws2_32.lib")

#include <WinSock2.h> //Winsock 헤더파일 include. WSADATA 들어있음.ㄴ
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

int chat_recv() {
    char buf[MAX_SIZE] = { };
    string msg;

    while (1) {
        ZeroMemory(&buf, MAX_SIZE);

        if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
            msg = buf;
            // 닉네임 : 메시지
            std::stringstream ss(msg);  // 문자열을 스트림화하고 ss라는 변수에 담는다.
            string user;
            ss >> user; // ss에 담고 스트림을 통해, 문자열을 공백 분리해 변수에 할당
            if (user != my_nick) cout << buf << endl; // 내가 보낸 게 아닐 경우에만 출력하도록.
            // 닉네임 : 메시지에서 처음 '닉네임' 만 받아서 my_nick과 비교한다. 통과하지 못한다면
        }
        else {
            cout << "Server Off" << endl;
            return -1;
        }
    }
}

int main() {
    WSADATA wsa;

    // Winsock를 초기화하는 함수. MAKEWORD(2, 2)는 Winsock의 2.2 버전을 사용하겠다는 의미.
    // 실행에 성공하면 0을, 실패하면 그 이외의 값을 반환.
    // 0을 반환했다는 것은 Winsock을 사용할 준비가 되었다는 의미.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa);

    if (!code) {
        
        cout << "이미 아이디가 있으시면 1, 회원가입을 원하시면 2번을 눌러주세요.";
        int sign;
        cin >> sign;
        if (sign == 1) {
            cout << "로그인을 해주세요";

            cout << "아이디를 입력해주세요 :";
            cin >> id;


            cout << "비밀번호를 입력해주세요 :";
            cin >> password;

            client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); // 
            my_nick = "*로그인*";

            SOCKADDR_IN client_addr = {};
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(2304);
            InetPton(AF_INET, TEXT("127.0.0.1"), &client_addr.sin_addr);

            while (1) {
                if (!connect(client_sock, (SOCKADDR*)&client_addr, sizeof(client_addr))) {
                    cout << "Server Connect" << endl;
                    send(client_sock, (my_nick + ' ' + id + ' ' + password).c_str(), (my_nick + ' ' + id + ' ' + password).length(), 0);


                    break;
                }
                cout << "Connecting..." << endl;
            }

        }
        else if (sign == 2) {
            cout << "회원가입 페이지입니다." << endl;

            cout << "아이디를 입력해주세요 :";
            cin >> id;


            cout << "비밀번호를 입력해주세요 :";
            cin >> password;


            cout << "사용할 닉네임 입력 >> ";
            cin >> my_nick;

            client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); // 

            SOCKADDR_IN client_addr = {};
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(2304);
            InetPton(AF_INET, TEXT("127.0.0.1"), &client_addr.sin_addr);

            while (1) {
                if (!connect(client_sock, (SOCKADDR*)&client_addr, sizeof(client_addr))) {
                    cout << "Server Connect" << endl;
                    send(client_sock, (my_nick + ' ' + id + ' ' + password).c_str(), (my_nick + ' ' + id + ' ' + password).length(), 0);
                    

                    break;
                }
                cout << "Connecting..." << endl;
                
                
            }
            main();
            
        }


        std::thread th2(chat_recv);
        

        while (1) {
            string text;
            std::getline(cin, text);
            const char* buffer = text.c_str(); // string형을 char* 타입으로 변환
            send(client_sock, buffer, strlen(buffer), 0);
        }
        th2.join();
        closesocket(client_sock);
    }

    WSACleanup();
    return 0;
}