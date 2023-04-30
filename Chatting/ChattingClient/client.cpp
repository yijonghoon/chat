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

int chat_recv() {
    char buf[MAX_SIZE] = { };
    string msg;

    while (1) {
        ZeroMemory(&buf, MAX_SIZE);

        if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
            msg = buf;
            // �г��� : �޽���
            std::stringstream ss(msg);  // ���ڿ��� ��Ʈ��ȭ�ϰ� ss��� ������ ��´�.
            string user;
            ss >> user; // ss�� ��� ��Ʈ���� ����, ���ڿ��� ���� �и��� ������ �Ҵ�
            if (user != my_nick) cout << buf << endl; // ���� ���� �� �ƴ� ��쿡�� ����ϵ���.
            // �г��� : �޽������� ó�� '�г���' �� �޾Ƽ� my_nick�� ���Ѵ�. ������� ���Ѵٸ�
        }
        else {
            cout << "Server Off" << endl;
            return -1;
        }
    }
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
            cout << "�α����� ���ּ���";

            cout << "���̵� �Է����ּ��� :";
            cin >> id;


            cout << "��й�ȣ�� �Է����ּ��� :";
            cin >> password;

            client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); // 
            my_nick = "*�α���*";

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
            cout << "ȸ������ �������Դϴ�." << endl;

            cout << "���̵� �Է����ּ��� :";
            cin >> id;


            cout << "��й�ȣ�� �Է����ּ��� :";
            cin >> password;


            cout << "����� �г��� �Է� >> ";
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
            const char* buffer = text.c_str(); // string���� char* Ÿ������ ��ȯ
            send(client_sock, buffer, strlen(buffer), 0);
        }
        th2.join();
        closesocket(client_sock);
    }

    WSACleanup();
    return 0;
}