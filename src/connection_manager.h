#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include "config.h"
#include "exception.h"
#include "tcp_socket.h"

#include <vector>
#include <map>

class Connection : public TcpSocket
{
    public:
        Connection(int id, int port, std::string& host)
            : TcpSocket(port, host), id(id)
        { }

        inline int getId(void) const { return id; }

    private:
        int id;

};

class ConnectionManager
{
    public:
        ConnectionManager() {}
        ConnectionManager( std::vector<TcpConfig>& cfgs );
        void init( std::vector<TcpConfig>& cfgs );
        void connect_all( void );
        bool connect( int id );
        void close_all( void );
        void close( int id ); // close particular connection
        const Connection* get( int id );
        void wait_for_connections( void );
        bool exist( int id );

        inline void add( TcpConfig& cfg )
        {
            connections.emplace_back( cfg.number, cfg.port, cfg.host );
        }

        inline const std::vector<Connection>& get_all() { return connections; }
        inline int get_num_of_connection() { return connections.size(); }

    private:
        std::vector<Connection> connections;
        std::map<int, Connection*> conn_map;
};

#endif
