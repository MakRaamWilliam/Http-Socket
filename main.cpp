#include <iostream>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
//#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>
#include "ctime"
#include <bits/stdc++.h>

#pragma comment(lib, "Ws_.lib")

#define  MAXPENDING 10
#define  MAXDATABUF 50000
#define  TIMEOUT  1000
using namespace std;

string Handle_Request(string request);
string Handle_GET(string file_path);
string Handle_POST(string request, string file_path);
void* HandleTCPClient(void* params);

struct structSock{
    int socket;
};
int clinetnum;
vector<string> splitwithdel(string command, char del){

    stringstream commandstr(command);
    string s;
    vector<string> lines;
    while(getline(commandstr, s, del))
        lines.push_back(s);

    return lines;
}

string getdata(string command){
    size_t pos = 0;
    string del="\r\n\r\n";
    while ((pos = command.find(del)) != std::string::npos) {
        command.erase(0, pos + del.length());
    }
    return command ;
}

// from https://stackoverflow.com/questions/15660203/inet-pton-identifier-not-found/15660299
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    struct sockaddr_storage ss;
    unsigned long s = size;

    ZeroMemory(&ss, sizeof(ss));
    ss.ss_family = af;

    switch(af) {
        case AF_INET:
            ((struct sockaddr_in *)&ss)->sin_addr = *(struct in_addr *)src;
            break;
        case AF_INET6:
            ((struct sockaddr_in6 *)&ss)->sin6_addr = *(struct in6_addr *)src;
            break;
        default:
            return NULL;
    }
    /* cannot direclty use &size because of strict aliasing rules */
    return (WSAAddressToString((struct sockaddr *)&ss, sizeof(ss), NULL, dst, &s) == 0)?
           dst : NULL;
}

int main(int argc, char const *argv[])
{

    int servPort = 5000;
    if(argc >= 2)
        servPort = atoi(argv[1]);

    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    if (wsResult != 0)
    {
        cerr << "Can't start Winsock, Err #" << wsResult << endl;
        return 0;
    }

    int servSock;
    servSock = socket(AF_INET, SOCK_STREAM, 0);

    if (servSock == INVALID_SOCKET)
    {
        perror("In socket  ");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort );

    if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){
        perror("Error in  bind");
        exit(EXIT_FAILURE);
    }

    if (listen(servSock, MAXPENDING) < 0){
        perror("Error in listen");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        printf("\n    Waiting for new connection on port %d \n\n",servPort);
        struct sockaddr_in clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);
        int clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
        if (clntSock < 0){
            perror("Error in accept");
            exit(EXIT_FAILURE);
        }
        char clntName[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,sizeof(clntName)) != NULL)
            printf("Handling client %s/%d\n", clntName, ntohs(clntAddr.sin_port));
        clinetnum++;
        structSock *paramSock = new structSock();
        pthread_t pthread;
        paramSock->socket = clntSock;
        pthread_create(&pthread, NULL, &HandleTCPClient, (void *) paramSock);
    }
    return 0;
}

void* HandleTCPClient(void* params){
    struct structSock *p = (structSock *) params;
    int clntSock = p->socket;
    char data [MAXDATABUF] ;
    unsigned long nwtimeout = 5 * 1000 / clinetnum;
    unsigned long strtime = std::clock();
    while(true){
        if((std::clock() - strtime) > nwtimeout)
            break;
        ZeroMemory(data, MAXDATABUF);
        long recBytes = recv(clntSock , data, MAXDATABUF, 0);
        if (recBytes <= 0  ) {
            cout<<"Client disconnected\n";
            break;
        }
        string datastr = string(data,recBytes);
        cout << ">>>>>>>>>>>>> Received Request "<<recBytes << " Bytes\n";
        cout << string (data,recBytes) << "\n";
        string response = Handle_Request(datastr);

        long sendBytes = send(clntSock, response.c_str(), response.size() + 1, 0);
        cout << ">>>>>>>>>>>>> Sent Response "<<sendBytes<<" Bytes\n" << response << "\n";
        close(clntSock);
    }
    cout << "--------Close Connection \n";
    close(clntSock);
}

string Handle_Request(string request){
    vector<string> tokens = splitwithdel(request, ' ');

    if(tokens.size() < 2){
        return "HTTP/1.1 404 Not Found\r\n";
    }
    if( tokens[0] == "GET"){
        return Handle_GET(tokens[1]);
    } else if( tokens[0] == "POST"){
        return Handle_POST(request, tokens[1]);
    } else {
        return "HTTP/1.1 404 Not Found\r\n";
    }
}

string Handle_GET(string file_path){
    string mypath = "C:\\Users\\makrm\\CLionProjects\\server\\data";
    file_path = mypath + file_path;
    if(splitwithdel(file_path,'.').size() < 2)
        return "HTTP/1.1 404 Not Found\r\n";
    string response ="";
    string type;
    type = splitwithdel(file_path, '.')[1];
    if (type == "txt") {
        type = "Content-Type: text/plain\n";
    } else if (type == "html") {
        type = "Content-Type: text/html\n";
    } else {
        if (type == "jpg") type = "jpeg";
        type = "Content-Type: image/" + type + "\n";
    }

    ifstream file(file_path,std::ios::binary);
    if(file.good()){
        string datafile;
        string temp = "";
        while (getline (file, temp)) {
            datafile += temp + "\n";
        }
        response = "HTTP/1.1 200 OK\n";
        response += "Status Code: 200\n";
        response += type + "Content-Length: "+ to_string(datafile.size());
        response += "\r\n\r\n" + datafile;
    } else {
        response = "HTTP/1.1 404 Not Found\r\n";
    }
    file.close();
    return response;
}

string Handle_POST(string request, string file_path){
    string mypath = "C:\\Users\\makrm\\CLionProjects\\server\\posts\\";
    file_path = mypath + splitwithdel(file_path, '/').back();
    ofstream file(file_path, std::ios::binary);
    string  data = getdata(request);
    file<< data.substr(0,data.size()-1);
    file.close();
    return "HTTP/1.1 200 OK\r\n";
}
