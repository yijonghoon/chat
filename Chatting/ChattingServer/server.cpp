#pragma comment(lib, "ws2_32.lib") // ��������� ���̺귯���� ȣ���ϴ� ��� �� �ϳ�.

#include <WinSock2.h> //winsock ��� �����
#include <string>
#include <iostream>
#include <thread>
#include <sstream>
#include <vector>
#include "mysql.h"


#define MAX_SIZE 1024
#define MAX_CLIENT 4 // �ִ� 3��

using std::cout;
using std::cin;
using std::endl;
using std::string;

bool is_duplicate;
bool is_login;


struct SOCKET_INFO {
    SOCKET sck; // ctrl + Ŭ�� �ϸ� ��� �� SOCKET �ڵ带 ã�ư� �� �ִ�, socket�� unsigned int pointer�� ���� �ֱ��� �ϰ� ������ �� �ִ�.
    string user; // user : ����� �̸�
};

std::vector<SOCKET_INFO> sck_list; // ������ ����� client ������ ����
SOCKET_INFO server_sock; // ���� ������ ������ ������ ����
int client_count = 0;  //���� �����ϰ� �ִ� Ŭ���̾�Ʈ�� �� ī��Ʈ.
string nickname;

void server_init(); // ������ ������ ����� �Լ�. ~ listen()
void add_client(); // accept �Լ� ����ǰ� ���� ����
void add_client_retry(SOCKET_INFO new_client, char* buf);
void send_msg(const char* msg); // send () ����
void recv_msg(int idx); // recv() ����
void del_client(int idx); // Ŭ���̾�Ʈ���� ������ ���� ��
void sql_signup(string id, string nickname, string password);
void sql_is_id_duplicate(string new_id);
void check_id_duplicate(string new_id);
void sql_connect();
void sql_disconnect();
void sql_login(string id, string pw);

// MySQL Connector/C++ �ʱ�ȭ
sql::mysql::MySQL_Driver* driver; // ���� �������� �ʾƵ� Connector/C++�� �ڵ����� ������ ��
sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;
sql::ResultSet* result;

int main() {
    WSADATA wsa;

    // Winsock�� �ʱ�ȭ�ϴ� �Լ�. MAKEWORD(2, 2)�� Winsock�� 2.2 ������ ����ϰڴٴ� �ǹ�.
    // ���࿡ �����ϸ� 0��, �����ϸ� �� �̿��� ���� ��ȯ.
    // 0�� ��ȯ�ߴٴ� ���� Winsock�� ����� �غ� �Ǿ��ٴ� �ǹ�.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa); // �����ϸ� 0. !0 => 1
    
    // code�� 0�� �Ǹ� if���� �����ϰڴ�.
    if (!code) {
        server_init(); // ������ ���� Ȱ��ȭ

        std::thread th1[MAX_CLIENT]; 
        // ũ�Ⱑ MAX_CLIENT�� �迭 ����. �迭�� ��� �ڷ����� std::thread
        // thread = ���, thread�� �����ٰ� �ϱ� ������ main �Լ��� ������� �ʵ��� ����. �޼ҵ� .join
        for (int i = 0; i < MAX_CLIENT; i++) {
            th1[i] = std::thread(add_client); 
            // Ŭ���̾�Ʈ ���� �� �ִ� ���¸� ����� ��, accept
            // Ŭ���̾�Ʈ ���� ���� �ٸ� ���� �� �� �ְԲ� ���߽����带 ����� ��
        }


        while (1) {
            string text, msg = "";

            std::getline(cin, text);
            const char* buf = text.c_str();
            msg = server_sock.user + " : " + buf;
            send_msg(msg.c_str());
        }

        for (int i = 0; i < MAX_CLIENT; i++) {
            th1[i].join();
        }

        closesocket(server_sock.sck); //���� ����
    }
    else {
        cout << "���α׷� ����. (Error code : " << code << ")"; // if�� ��� ���ϸ� (�ʱ�ȭ ����), ���α׷� ����
    }

    WSACleanup();

    return 0;
}

void server_init() {
    server_sock.sck = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);  // SOCK_STREAM�� TCP 1, UDP 2, ���� 3 �̷��� �ڵ带 �����´�.
    // ���� ������ Ư���� �� �ִ� int�� ���ڸ� ����. server+sock�� sck
    // SOCKET_INFO sck, user
    SOCKADDR_IN server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2304); // ~~~~:7777
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //  127.0.0.1: ~~~~

    bind(server_sock.sck, (sockaddr*)&server_addr, sizeof(server_addr));
    // server_sock.sck, �ּҸ� �Ҵ��ϰ� ���� socket
    // (sockaddr*)&server_addr�� ��� server_addr�� �ڷ��� SOCKADDR_IN�� sockaddr* ������ ��ȯ

    listen(server_sock.sck, SOMAXCONN);

    server_sock.user = "����";

    cout << "���� On" << endl;
}

void add_client() {
    cout << "add_client" << endl;
    // Ŭ���̾�Ʈ�� ������ ���ῡ �����ϸ�, ���ο� ������ �ϳ� �����ϰ� �Ǵµ�, 
    // �� �ּҸ� ���� ���� => addr
    SOCKADDR_IN addr = {};
    int addrsize = sizeof(addr);
    char buf[MAX_SIZE] = { }; // �޽��� �ִ� ���� ����

    ZeroMemory(&addr, addrsize); //  addr �޸𸮸� 0x00���� �ʱ�ȭ/addrsize = sizeof(addr), 

    SOCKET_INFO new_client = {};
    // sck, user : Ŭ���̾�Ʈ�� ���� ������ ����



    new_client.sck = accept(server_sock.sck, (sockaddr*)&addr, &addrsize);
    // connect()
    recv(new_client.sck, buf, MAX_SIZE, 0); // Ŭ���̾�Ʈ connect(), send()
    // Ŭ���̾�Ʈ ������ �ٷ� user �̸��� ��Ƽ� send�� ��. recv()�� �ޱ� ����
    cout << "buf" << buf << endl;
    std::stringstream userinfo(buf);
    string id, pw;
    userinfo >> new_client.user >> id >> pw;

    cout << "user" << new_client.user << endl;

    if (new_client.user == "*login*") {
        cout << "�α������� ����!" << endl;
        sql_login(id, pw);
        if (is_login) {
            cout << "is_login if�� �б� ��� �Ϸ�" << endl;
            new_client.user = nickname;
            string msg = "[����] " + new_client.user + " ���� �����߽��ϴ�.";
            cout << msg << endl; // ���� �ֿܼ� ���� ����.


            sck_list.push_back(new_client); // sck list �� �߰���.
            // [ {12345,"jonghoon"}, {43210,"yijonghoon"},{32134,"hoon"} ]

            std::thread th(recv_msg, client_count); //client_count = ���� Ŭ���̾�Ʈ�� ��(���� �� 0)
            // ��� ������ client�� �����ε� ��� �޽����� ���� �� �ֵ��� recv


            client_count++; // Ŭ���̾�Ʈ �� �÷���.
            cout << "[����] ���� ������ �� : " << client_count << "��" << endl;
            send_msg(msg.c_str());
            th.join(); // �긦 ���������� main�Լ��� ���ᰡ ������.
        }
        /*else {
            send(new_client.sck, "���̵� Ȥ�� �н����尡 �ٸ��ϴ�.", MAX_SIZE, 0);
        }*/

    }
    // new_client.user = string(buf); // buf�� string������ ��ȯ�ؼ� user�� ����

    // ȸ�� ���� part
    else { 
        is_duplicate = false;
        sql_is_id_duplicate(id);



        if (!is_duplicate) {
            sql_signup(id, new_client.user, pw);
            send(new_client.sck, "Success : ȸ�����Կ� �����߽��ϴ�.", MAX_SIZE, 0);
            string msg = "[����] " + new_client.user + " ���� �����߽��ϴ�.";
            cout << msg << endl; // ���� �ֿܼ� ���� ����.


            sck_list.push_back(new_client); // sck list �� �߰���.
            // [ {12345,"jonghoon"}, {43210,"yijonghoon"},{32134,"hoon"} ]

            std::thread th(recv_msg, client_count); //client_count = ���� Ŭ���̾�Ʈ�� ��(���� �� 0)
            // ��� ������ client�� �����ε� ��� �޽����� ���� �� �ֵ��� recv


            client_count++; // Ŭ���̾�Ʈ �� �÷���.
            cout << "[����] ���� ������ �� : " << client_count << "��" << endl;
            send_msg(msg.c_str());
            th.join(); // �긦 ���������� main�Լ��� ���ᰡ ������.

        }
        else {
            send(new_client.sck, "Fail : �ߺ��� ���̵� �ֽ��ϴ�.", MAX_SIZE, 0);
            add_client_retry(new_client,buf);
        }


    }
    
   
}
void add_client_retry(SOCKET_INFO new_client, char * buf) {
    cout << "add_client_retry" << endl;
    recv(new_client.sck, buf, MAX_SIZE, 0); // Ŭ���̾�Ʈ connect(), send()
    // Ŭ���̾�Ʈ ������ �ٷ� user �̸��� ��Ƽ� send�� ��. recv()�� �ޱ� ����
    cout << "buf" << buf << endl;
    std::stringstream userinfo(buf);
    string id, pw;
    userinfo >> new_client.user >> id >> pw;

    cout << "user" << new_client.user << endl;

    if (new_client.user == "*login*") {
        cout << "�α������� ����!" << endl;
        sql_login(id, pw);
        if (is_login) {
            cout << "is_login if�� �б� ��� �Ϸ�" << endl;
            new_client.user = nickname;
            string msg = "[����] " + new_client.user + " ���� �����߽��ϴ�.";
            cout << msg << endl; // ���� �ֿܼ� ���� ����.


            sck_list.push_back(new_client); // sck list �� �߰���.
            // [ {12345,"jonghoon"}, {43210,"yijonghoon"},{32134,"hoon"} ]

            std::thread th(recv_msg, client_count); //client_count = ���� Ŭ���̾�Ʈ�� ��(���� �� 0)
            // ��� ������ client�� �����ε� ��� �޽����� ���� �� �ֵ��� recv


            client_count++; // Ŭ���̾�Ʈ �� �÷���.
            cout << "[����] ���� ������ �� : " << client_count << "��" << endl;
            send_msg(msg.c_str());
            th.join(); // �긦 ���������� main�Լ��� ���ᰡ ������.
        }
        /*else {
            send(new_client.sck, "���̵� Ȥ�� �н����尡 �ٸ��ϴ�.", MAX_SIZE, 0);
        }*/

    }
    // new_client.user = string(buf); // buf�� string������ ��ȯ�ؼ� user�� ����

    // ȸ�� ���� part
    else { 
        is_duplicate = false;
        sql_is_id_duplicate(id);

        cout << "is_duplicate" << is_duplicate << endl;


        if (!is_duplicate) {
            cout << "new" << endl;
            sql_signup(id, new_client.user, pw);
            send(new_client.sck, "Success : ȸ�����Կ� �����߽��ϴ�.", MAX_SIZE, 0);
            string msg = "[����] " + new_client.user + " ���� �����߽��ϴ�.";
            cout << msg << endl; // ���� �ֿܼ� ���� ����.


            sck_list.push_back(new_client); // sck list �� �߰���.
            // [ {12345,"jonghoon"}, {43210,"yijonghoon"},{32134,"hoon"} ]

            std::thread th(recv_msg, client_count); //client_count = ���� Ŭ���̾�Ʈ�� ��(���� �� 0)
            // ��� ������ client�� �����ε� ��� �޽����� ���� �� �ֵ��� recv


            client_count++; // Ŭ���̾�Ʈ �� �÷���.
            cout << "[����] ���� ������ �� : " << client_count << "��" << endl;
            send_msg(msg.c_str());
            th.join(); // �긦 ���������� main�Լ��� ���ᰡ ������.

        }
        else {
            cout << "duplicated" << endl;
            send(new_client.sck, "Fail : �ߺ��� ���̵� �ֽ��ϴ�.", MAX_SIZE, 0);
            add_client_retry(new_client, buf);
        }


    }
    
   
}

void send_msg(const char* msg) {
    for (int i = 0; i < client_count; i++) {
        send(sck_list[i].sck, msg, MAX_SIZE, 0);
    }
    
}
void send_msg(SOCKET sck,string msg) {
   // send(sck_list[i].sck, msg, MAX_SIZE, 0);
}

void recv_msg(int idx) {
    char buf[MAX_SIZE] = { };
    string msg = "";

    while (1) { //recv_msg�� �ѹ� ����Ǹ� ���������� ��� ���.
        ZeroMemory(&buf, MAX_SIZE); // buf 0���� �ʱ�ȭ
        if (recv(sck_list[idx].sck, buf, MAX_SIZE, 0) > 0) {
            msg = sck_list[idx].user + " : " + buf;
            cout << msg << endl;
            send_msg(msg.c_str());
        }
        else {
            msg = "[����] " + sck_list[idx].user + " ���� �����߽��ϴ�.";
            cout << msg << endl;
            send_msg(msg.c_str());
            del_client(idx);
            return;
        }
    }
}


void del_client(int idx) {
    closesocket(sck_list[idx].sck);
    client_count--;
}


void sql_signup(string id, string nickname, string password) {


    sql_connect();

    // �����ͺ��̽� ����
    con->setSchema("project_chat");

    // db �ѱ� ������ ���� ���� 
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    if (stmt) { delete stmt; stmt = nullptr; }


    pstmt = con->prepareStatement("INSERT INTO user(id, nickname, password) VALUES(?,?,?)"); // INSERT

    pstmt->setString(1, id);
    pstmt->setString(2, nickname); 
    pstmt->setString(3, password);
 
    pstmt->execute(); // ���� ����

    sql_disconnect();

}

void sql_is_id_duplicate(string new_id) {
    sql_connect();
    cout << "sql_is_id_duplicate" << endl;
    stmt = con->createStatement();

    result = stmt->executeQuery("SELECT * FROM user WHERE id = '"+new_id +"'");
    delete stmt;

    while (result->next()) {
        cout << "�ߺ��˻翡 �ɸ�" << endl;
        is_duplicate = true;
    }

    sql_disconnect();
}

void sql_connect() {

    // MySQL ����
    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(server, username, dbpassword); // MySQL �����ͺ��̽� ���� ��ü
    }
    catch (sql::SQLException& e) {
        cout << "Could not connect to server. Error message: " << e.what() << endl;
        exit(1);
    }

    // �����ͺ��̽� ����
    con->setSchema("project_chat");

    // db �ѱ� ������ ���� ���� 
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    if (stmt) { delete stmt; stmt = nullptr; }
}
void sql_disconnect() {
    // MySQL Connector/C++ ����
    delete pstmt;
    delete con;


}

void sql_login(string id, string pw) {
    sql_connect();
    is_login = false;
    pstmt = con->prepareStatement("SELECT * FROM user WHERE id = ? AND password = ?");
    pstmt->setString(1, id);
    pstmt->setString(2, pw);
    result = pstmt->executeQuery();

    while (result->next()) {
        cout << "�α��� ����!" << endl;
        pstmt = con->prepareStatement("SELECT nickname FROM user WHERE id = ? AND password = ?");
        pstmt->setString(1, id);
        pstmt->setString(2, pw);
        result = pstmt->executeQuery();
        while (result->next()) {
            nickname = result->getString(1);
            cout << nickname << endl;
            is_login = true;
        }



    }

    sql_disconnect();
}





/*

�ڵ��� ����
server_init ������ ���� Ȱ��ȭ
max client = 3

for(MAX_CLIENT){
Ŭ���̾�Ʈ�� ���� ���ִ� ����
accept,

*/