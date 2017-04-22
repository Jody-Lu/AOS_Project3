#ifndef TCP_SOCKET
#define TCP_SOCKET

#include "constants.h"
#include "exception.h"

#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_FILE_NAME_LEN 128
#define MAX_WRITE_LEN 512
#define MAX_DATA_LEN 1024
#define HOST_NAME_LEN 50

// command input can be a string or integer
union CommandInput
{
    char data[MAX_WRITE_LEN];
    int number;
};

// data struct carried by the socket buffer
struct SockData
{
    MSG_TYPE msg_t;
    CMD_TYPE cmd_t;
    char filename[MAX_FILE_NAME_LEN];
    CommandInput input;
};

struct ReplyMessage
{
    int server_num;
    bool result;
    int val;
    char message[MAX_WRITE_LEN];
};

struct WriteMessage
{
    int id;
    int seq_num;
    int key;
    int val;
    int commit_keys[3];
    char host_name[HOST_NAME_LEN];

    std::string to_string( std::string type );
};

struct MaekawaMessage
{
    MAEKAWA_MSG_TYPE maekawa_t;
    int id;
};

struct JajodiaMessage
{
    JAJODIA_MSG_TYPE jajodia_t;
    int id;
    int VN;
    int RU;
    int DS;
};

union SimpleMessagePayload
{
    MaekawaMessage maekawa_m;
    JajodiaMessage jajodia_m;
    WriteMessage write_m;
    ReplyMessage reply_m;
};

struct SimpleMessage
{
    SIMPLE_MSG_TYPE msg_t;
    SimpleMessagePayload payload;
};

class TcpSocket
{
    public:
        TcpSocket() {}  //default constructor
        TcpSocket(int port, std::string host = "localhost");
        ~TcpSocket() throw (Exception);

        // socket API wrappers
        bool connect(void) throw (Exception);
        void bind(void) throw (Exception);
        void listen(int req_q_len) throw (Exception);
        void accept(TcpSocket& client_socket) throw (Exception);
        void send(void* data, int size) const throw (Exception);
        void send(std::string) const throw (Exception);
        int receive(void* data, int size) const throw (Exception);
        void close(void) throw (Exception);

        inline bool is_active() const { return sock_fd > -1; }
        inline int get_sock_fd() const { return sock_fd; }
        inline int get_port() const { return port; }
        inline std::string get_host() const { return host; }

    private:
        std::string host;
        int port;
        int sock_fd;
        struct hostent* host_entry;
        struct sockaddr_in host_addr;

        void init(bool ifIpAddrAny = false) throw (Exception);
};

#endif
