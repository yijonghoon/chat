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
            cout << "�������� ������ ���������ϴ�." << endl;
            break;
        }
    }
    return 0;
}
int main() {
    WSADATA wsa;
    // Winsock�� �ʱ�ȭ�ϴ� �Լ�. MAKEWORD(2, 2)�� Winsock�� 2.2 ������ ����ϰڴٴ� �ǹ�.
    // ���࿡ �����ϸ� 0��, �����ϸ� �� �̿��� ���� ��ȯ.
    // 0�� ��ȯ�ߴٴ� ���� Winsock�� ����� �غ� �Ǿ��ٴ� �ǹ�.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (!code) {
        cout << "�̹� ���̵� �����ø� 1, ȸ�������� ���Ͻø� 2���� �����ּ���.";
        int sign;
        cin >> sign;
        if (sign == 1) {
            my_nick = "*login*";
            cout << "�α����� ���ּ���" << endl;;
            cout << "���̵� �Է����ּ��� : ";
            cin >> id;
            cout << "��й�ȣ�� �Է����ּ��� : ";
            cin >> password;

        }
        else if (sign == 2) {
            cout << "ȸ������ �������Դϴ�." << endl;
            cout << "���̵� �Է����ּ��� :";
            cin >> id;
            cout << "��й�ȣ�� �Է����ּ��� :";
            cin >> password;
            cout << "����� �г��� �Է� >> ";
            cin >> my_nick;

        }


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


        isServerConnected = true;
        std::thread th2(chat_recv);
        while (1) {
            string text;
            std::getline(cin, text);
            const char* buffer = text.c_str(); // string���� char* Ÿ������ ��ȯ
            send(client_sock, buffer, strlen(buffer), 0);
        }
        isServerConnected = false;
        th2.join();
        closesocket(client_sock);
    }
    WSACleanup();
    return 0;
}