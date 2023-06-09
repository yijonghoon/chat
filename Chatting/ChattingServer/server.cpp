#pragma comment(lib, "ws2_32.lib") // 명시적으로 라이브러리를 호출하는 방법 중 하나.

#include <WinSock2.h> //winsock 헤더 사용중
#include <string>
#include <iostream>
#include <thread>
#include <sstream>
#include <vector>
#include "mysql.h"


#define MAX_SIZE 1024
#define MAX_CLIENT 4 // 최대 3명

using std::cout;
using std::cin;
using std::endl;
using std::string;

bool is_duplicate;
bool is_login;


struct SOCKET_INFO {
    SOCKET sck; // ctrl + 클릭 하면 헤더 속 SOCKET 코드를 찾아갈 수 있다, socket은 unsigned int pointer를 갖고 있구나 하고 예측할 수 있다.
    string user; // user : 사람의 이름
};

std::vector<SOCKET_INFO> sck_list; // 서버에 연결된 client 저장할 변수
SOCKET_INFO server_sock; // 서버 소켓의 정보를 저장할 변수
int client_count = 0;  //현재 접속하고 있는 클라이언트의 수 카운트.
std::vector<int> empty_socket;
string nickname;

void server_init(); // 서버용 소켓을 만드는 함수. ~ listen()
void socket_list_init();
void add_client(); // accept 함수 실행되고 있을 예정
void add_client_authentication(SOCKET_INFO new_client);
void send_msg(const char* msg); // send () 실행
void send_history(string user_id, SOCKET_INFO new_client);
void recv_msg(int idx); // recv() 실행
void del_client(int idx); // 클라이언트와의 연결을 끊을 때
void sql_signup(string id, string nickname, string password);
void sql_check_if_id_exists(string new_id);
void sql_connect();
void sql_disconnect();
void sql_login(string id, string pw);
void sql_message_input(string sender, string receiver, string msg);

// MySQL Connector/C++ 초기화
sql::mysql::MySQL_Driver* driver; // 추후 해제하지 않아도 Connector/C++가 자동으로 해제해 줌
sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;
sql::ResultSet* result;

int main() {
    WSADATA wsa;

    // Winsock를 초기화하는 함수. MAKEWORD(2, 2)는 Winsock의 2.2 버전을 사용하겠다는 의미.
    // 실행에 성공하면 0을, 실패하면 그 이외의 값을 반환.
    // 0을 반환했다는 것은 Winsock을 사용할 준비가 되었다는 의미.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa); // 성공하면 0. !0 => 1
    
    // code가 0이 되면 if문을 실행하겠다.
    if (!code) {
        server_init(); // 서버측 소켓 활성화

        std::thread th1[MAX_CLIENT]; 
        // 크기가 MAX_CLIENT인 배열 생성. 배열에 담길 자료형은 std::thread
        // thread = 대기, thread가 끝났다고 하기 전까지 main 함수가 종료되지 않도록 해줌. 메소드 .join
        for (int i = 0; i < MAX_CLIENT; i++) {
            th1[i] = std::thread(add_client); 
            // 클라이언트 받을 수 있는 상태를 만들어 줌, accept
            // 클라이언트 별로 각자 다른 일을 할 수 있게끔 다중스레드를 만들어 줌
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

        closesocket(server_sock.sck); //소켓 종료
    }
    else {
        cout << "프로그램 종료. (Error code : " << code << ")"; // if를 통과 못하면 (초기화 실패), 프로그램 종료
    }

    WSACleanup();

    
    0;
}

void server_init() {
    server_sock.sck = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);  // SOCK_STREAM은 TCP 1, UDP 2, 자유 3 이렇게 코드를 가져온다.
    // 서버 소켓을 특정할 수 있는 int형 숫자를 담음. server+sock의 sck
    // SOCKET_INFO sck, user
    SOCKADDR_IN server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(2304); // ~~~~:7777
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //  127.0.0.1: ~~~~

    bind(server_sock.sck, (sockaddr*)&server_addr, sizeof(server_addr));
    // server_sock.sck, 주소를 할당하고 싶은 socket
    // (sockaddr*)&server_addr의 경우 server_addr의 자료형 SOCKADDR_IN을 sockaddr* 형으로 변환

    listen(server_sock.sck, SOMAXCONN);

    server_sock.user = "서버";

    cout << "서버 On" << endl;
}
void socket_list_init() {
    for (int i = 0; i < MAX_CLIENT; i++)
        empty_socket.push_back(i);
}

void add_client() {
    cout << "add_client" << endl;
    // 클라이언트와 서버와 연결에 성공하면, 새로운 소켓을 하나 생성하게 되는데, 
    // 그 주소를 담을 변수 => addr
    SOCKADDR_IN addr = {};
    int addrsize = sizeof(addr);
     // 메시지 최대 길이 설정

    ZeroMemory(&addr, addrsize); //  addr 메모리를 0x00으로 초기화/addrsize = sizeof(addr), 

    SOCKET_INFO new_client = {};
    // sck, user : 클라이언트의 소켓 정보를 저장
    


    new_client.sck = accept(server_sock.sck, (sockaddr*)&addr, &addrsize);
    // connect()
    
    add_client_authentication(new_client); 
}

void add_client_authentication(SOCKET_INFO new_client) {
    char buf[MAX_SIZE] = { };
    //console cout << "add_client_authentication" << endl;
    //recv(new_client.sck, buf, MAX_SIZE, 0); // 클라이언트 connect(), send()
    // 클라이언트 측에서 바로 user 이름을 담아서 send를 함. recv()로 받기 위해
    //console cout << "buf" << buf << endl;
    if (recv(new_client.sck, buf, MAX_SIZE, 0) <= 0) {
        closesocket(new_client.sck);
         std::thread th(add_client);
        th.join();
        // th1[i] = std::thread(add_client);
        return;
    } 

    std::stringstream userinfo(buf);
    string id, pw;
    userinfo >> new_client.user >> id >> pw;

    //console cout << "user" << new_client.user << endl;
   
    if (new_client.user == "*login*") {
        //console cout << "로그인으로 들어옴!" << endl;
        sql_login(id, pw);
        if (is_login) {
            send(new_client.sck, "Success : 로그인을 성공했습니다.", MAX_SIZE, 0);
            send_history(id, new_client);
            //console cout << "is_login if문 분기 통과 완료" << endl;
            new_client.user = nickname;
            string msg = "[공지] " + new_client.user + " 님이 입장했습니다.";
            cout << msg << endl; // 서버 콘솔에 공지 찍음.


            sck_list.push_back(new_client); // sck list 에 추가함.
            // [ {12345,"jonghoon"}, {43210,"yijonghoon"},{32134,"hoon"} ]

            std::thread th(recv_msg, client_count); //client_count = 현재 클라이언트의 수(입장 전 0)
            // 방금 생성된 client가 앞으로도 계속 메시지를 받을 수 있도록 recv


            client_count++; // 클라이언트 수 늘려줌.
            cout << "[공지] 현재 접속자 수 : " << client_count << "명" << endl;
            send_msg(msg.c_str());
            th.join(); // 얘를 만날때까지 main함수의 종료가 보류됨.
        }
        else {
            send(new_client.sck, "Fail : 아이디 혹은 패스워드가 다릅니다.", MAX_SIZE, 0);
            cout << "Fail : 아이디 혹은 패스워드가 다릅니다." << endl;
            add_client_authentication(new_client);

        }

    }
    // new_client.user = string(buf); // buf를 string형으로 변환해서 user에 저장

    // 회원 가입 part
    else { 
        is_duplicate = false;
        sql_check_if_id_exists(id);

        //console cout << "is_duplicate" << is_duplicate << endl;


        if (!is_duplicate) {
            //console cout << "new" << endl;
            sql_signup(id, new_client.user, pw);
            send(new_client.sck, "Success : 회원가입에 성공했습니다.", MAX_SIZE, 0);
            string msg = "[공지] " + new_client.user + " 님이 입장했습니다.";
            cout << msg << endl; // 서버 콘솔에 공지 찍음.

         
            sck_list.push_back(new_client); // sck list 에 추가함.
            // [ {12345,"jonghoon"}, {43210,"yijonghoon"},{32134,"hoon"} ]

            std::thread th(recv_msg, client_count); //client_count = 현재 클라이언트의 수(입장 전 0)
            // 방금 생성된 client가 앞으로도 계속 메시지를 받을 수 있도록 recv


            client_count++; // 클라이언트 수 늘려줌.
            cout << "[공지] 현재 접속자 수 : " << client_count << "명" << endl;
            send_msg(msg.c_str());
            th.join(); // 얘를 만날때까지 main함수의 종료가 보류됨.

        }
        else {
            cout << "duplicated" << endl;
            send(new_client.sck, "Fail : 중복된 아이디가 있습니다.", MAX_SIZE, 0);
            add_client_authentication(new_client);
        }


    }
    
   
}

void send_msg(const char* msg) {
    for (int i = 0; i < client_count; i++) {
        send(sck_list[i].sck, msg, MAX_SIZE, 0);
    }
    
}


void recv_msg(int idx) {
    char buf[MAX_SIZE] = { };
    string msg = "";

    while (1) { //recv_msg가 한번 실행되면 받을때까지 계속 대기.
        ZeroMemory(&buf, MAX_SIZE); // buf 0으로 초기화
        if (recv(sck_list[idx].sck, buf, MAX_SIZE, 0) > 0) {
            std::stringstream sss(buf);
            string this_id, is_whisper, receiver, message;
            sss >> this_id;
            sss >> is_whisper;
            sss >> receiver;
            
            std::getline(sss, message);
            message.erase(message.begin());
            //console cout << this_id << " " << receiver << " " << message << " " << endl;
         
            msg = sck_list[idx].user + " : " + message;
            cout << msg << endl;
            send_msg(msg.c_str());
            sql_message_input(this_id, receiver, message);
        }
        else {
            msg = "[공지] " + sck_list[idx].user + " 님이 퇴장했습니다.";
            cout << msg << endl;
            send_msg(msg.c_str());
            del_client(idx);

            return;
        }
    }
}


void send_history(string user_id, SOCKET_INFO new_client) {
    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(server, username, dbpassword);
        con->setSchema("project_chat");
        stmt = con->createStatement();
        stmt->execute("set names euckr");
        sql::PreparedStatement* pstmt = con->prepareStatement("SELECT * FROM message WHERE (sender = ? OR receiver = ?) OR (receiver = '*')");
        pstmt->setString(1, user_id);
        pstmt->setString(2, user_id);
    
        sql::ResultSet* res = pstmt->executeQuery();
        while (res->next()) {
            
            string message = res->getString("message");
            string time = res->getString("time");
            string sender = res->getString("sender");
            cout << "[" << time << "] " << sender << ": " << message << endl;
        }
        delete res;
        delete pstmt;
    }
    catch (sql::SQLException& e) {
        cout << "SQL Exception: " << e.what() << endl;
    }
    if (stmt) { delete stmt; stmt = nullptr; }
}

void del_client(int idx) {
    closesocket(sck_list[idx].sck);
    client_count--;

}


void sql_signup(string id, string nickname, string password) {
    //console cout << "00";
    
    sql_connect();
    //console cout << "aa";

    // 데이터베이스 선택
    con->setSchema("project_chat");

    //console cout << "bb";

    // db 한글 저장을 위한 셋팅 
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    //console cout << "cc";
    if (stmt) { delete stmt; stmt = nullptr; }

    //console cout << "signup in";
    pstmt = con->prepareStatement("INSERT INTO user(id, nickname, password) VALUES(?,?,?)"); // INSERT

    pstmt->setString(1, id);
    pstmt->setString(2, nickname); 
    pstmt->setString(3, password);
 
    pstmt->execute(); // 쿼리 실행
    //console cout << "signup success";
    sql_disconnect();

}

void sql_check_if_id_exists(string new_id) {
    sql_connect();
    //console cout << "sql_check_if_id_exists" << endl;
    stmt = con->createStatement();
    //console cout << "sql_check_if_id_exists finish1" << endl;
    result = stmt->executeQuery("SELECT * FROM user WHERE id = '"+new_id +"'");
    delete stmt;
    //console cout << "sql_check_if_id_exists finish2" << endl;
    while (result->next()) {
        cout << "중복검사에 걸림" << endl;
        is_duplicate = true;
    }
    //console cout << "sql_check_if_id_exists finish3" << endl;
    sql_disconnect();

    //console cout << "sql_check_if_id_exists finish4" << endl;
}

void sql_connect() {

    // MySQL 연결
    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(server, username, dbpassword); // MySQL 데이터베이스 연결 객체
    }
    catch (sql::SQLException& e) {
        cout << "Could not connect to server. Error message: " << e.what() << endl;
        exit(1);
    }

    // 데이터베이스 선택
    con->setSchema("project_chat");

    // db 한글 저장을 위한 셋팅 
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    if (stmt) { delete stmt; stmt = nullptr; }
}
void sql_disconnect() {
    // MySQL Connector/C++ 정리
    if (pstmt != nullptr) {
        delete pstmt;
        pstmt = nullptr;
        cout << "disconnect pstmt"<<endl;
    }

    if (con != nullptr) {
        delete con;
        con = nullptr;
        cout << "discon con" << endl;
    }


}

void sql_login(string id, string pw) {
    sql_connect();
    is_login = false;
    pstmt = con->prepareStatement("SELECT * FROM user WHERE id = ? AND password = ?");
    pstmt->setString(1, id);
    pstmt->setString(2, pw);
    result = pstmt->executeQuery();

    while (result->next()) {
        cout << "로그인 성공!" << endl;
        pstmt = con->prepareStatement("SELECT nickname FROM user WHERE id = ? AND password = ?");
        pstmt->setString(1,id);
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

void sql_message_input(string sender, string receiver, string message) {
    sql_connect();
    //cout << sender << receiver << message << endl;
    con->setSchema("project_chat");

    //console cout << "bb";

    // db 한글 저장을 위한 셋팅 
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    //console cout << "cc";
    if (stmt) { delete stmt; stmt = nullptr; }

    //console cout << "signup in";
    pstmt = con->prepareStatement("INSERT INTO message(sender, receiver, message) VALUES(?,?,?);"); // INSERT

    pstmt->setString(1, sender);
    pstmt->setString(2, receiver);
    pstmt->setString(3, message);

    pstmt->execute(); // 쿼리 실행
    //console cout << "signup success";
    sql_disconnect();
}





/*

코드의 구조
server_init 서버측 소켓 활성화
max client = 3

for(MAX_CLIENT){
클라이언트를 받을 수있는 상태
accept,

*/