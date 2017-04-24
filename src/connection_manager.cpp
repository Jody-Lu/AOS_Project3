#include "connection_manager.h"

#define CONN_WAIT_TIMEOUT 10000 // 10 milliseconds

#include <unistd.h>
#include <iostream>


ConnectionManager::ConnectionManager(std::vector<TcpConfig>& cfgs)
{
    for (auto& cfg: cfgs) {
       connections.emplace_back(cfg.number, cfg.port, cfg.host);
    }
}

void ConnectionManager::init(std::vector<TcpConfig>& cfgs)
{
    for ( auto& cfg: cfgs )
    {
       connections.emplace_back(cfg.number, cfg.port, cfg.host);
    }
}

// blocking function; waits till all the connections are successfull
void ConnectionManager::connect_all(void)
{
    for (auto& conn: connections) {
        while (!conn.connect()) {
            usleep(CONN_WAIT_TIMEOUT);
        }
    }

    for (auto& conn: connections) {
        conn_map[conn.getId()] = &conn;
    }
}

bool ConnectionManager::connect(int id)
{
    for (auto& conn: connections) {
        if (conn.getId() == id) {
            return conn.connect();
        }
    }
    return false;
}

void ConnectionManager::close_all(void)
{
    for (auto& conn: connections) {
        conn_map.erase( conn.getId() );
        if (conn.is_active())
            conn.close();
    }
}

void ConnectionManager::close( int id )
{
    for ( auto it = connections.begin(); it != connections.end(); it++ )
    {
        if ( it->getId() == id )
        {
            if ( it->is_active() )
            {
                it->close();
            }
            connections.erase( it );
            conn_map.erase( id );
            break;
        }
    }
}

bool ConnectionManager::exist( int id )
{
    return conn_map.find( id ) != conn_map.end();
}

const Connection* ConnectionManager::get(int id)
{
    for (auto& conn: connections) {
        if (conn.getId() == id)
            return &conn;
    }
    return NULL;
}

void ConnectionManager::wait_for_connections(void)
{
    for (auto& conn: connections) {
        while (!conn.is_active()) {
            usleep(CONN_WAIT_TIMEOUT);
        }
    }
}

void ConnectionManager::add_connection(int id, int port, std::string& host)
{
    connections.emplace_back( id, port, host );
}
