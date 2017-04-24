#include "config.h"
#include "connection_manager.h"
#include "constants.h"
#include "callback_bridge.h"
#include "tcp_server.h"
#include "tcp_socket.h"
#include "utils.h"
#include "registry.h"
#include "jajodia_mutchler.h"

#include <unistd.h>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <string>
#include <deque>
#include <vector>

using namespace std;

#define NUM_OF_UPDATES 2

// function prototypes
void run_server( int port );
void on_client_data( void* data, TcpSocket* c_sock );


// static global variable
static TcpServer *server_ptr;
static bool do_terminate = false;
static string server_file;
static TcpConfig my_conf;
static vector<TcpConfig> other_servers;

// connection manager
ConnectionManager server_connections;

//
static vector<int> VNs;
static vector<int> RUs;
static vector<string> DSs;


int main( int argc, char const *argv[] )
{
    if ( argc < 2 )
    {
        Utils::print_error( "[ERROR] Usage: ./server <client_number>[1-7]" );
        exit( EXIT_FAILURE );
    }

    // Set server file name
    /*
    server_file = "server";
    server_file.append( argv[1] );
    server_file.append( ".out" );
    Registry::instance().add_file( server_file );
    */

    // configure quorum
    Config server_config( SERVER_CONFIG_FILE );
    int server_num = Utils::str_to_int( argv[1] );
    try
    {
        server_config.create();

        if ( server_num > NUM_OF_SERVERS )
        {
            throw Exception( "[ERROR] Undefined client number" );
        }
        for ( int i = 1; i <= NUM_OF_SERVERS; i++ )
        {
            TcpConfig cfg = server_config.getTcpConfig( i );
            if ( i == server_num )
            {
                my_conf = cfg;
                continue;
            }
            other_servers.push_back( cfg );
        }
    }
    catch (...)
    {
        Utils::print_error( "[ERROR] Unable to configure quorum" );
        exit( EXIT_FAILURE );
    }

    // Spawn server thread
    thread server_thread( run_server, my_conf.port );
    server_thread.detach();

    // Setup connection with partition peers
    JajodiaMutchler::instance().init( &server_config, my_conf.number );
    cout << "[INFO] partition initialized" << endl;

    // Execute update

    // Stage 1.
    for ( int i = 0; i < NUM_OF_UPDATES; i++ )
    {
        JajodiaMutchler::instance().execute_update();
    }

    // Stage 2.
    // Do_partition
    if ( server_num == 1 || server_num == 2 ||
         server_num == 3 || server_num == 4 )
    {
        int s[] = { 5, 6, 7, 8 };
        vector<int> sites( s, s + sizeof( s ) / sizeof( int ) );
        JajodiaMutchler::instance().close_connections( sites );
    }

    if ( server_num == 5 || server_num == 6 ||
         server_num == 7 || server_num == 8 )
    {
        int s[] = { 1, 2, 3, 4 };
        vector<int> sites( s, s + sizeof( s ) / sizeof( int ) );
        JajodiaMutchler::instance().close_connections( sites );
    }

    for ( int i = 0; i < NUM_OF_UPDATES; i++ )
    {
        JajodiaMutchler::instance().execute_update();
    }

    // Stage 3.
    // Do_partition
    if ( server_num == 1 )
    {
        int s[] = { 2, 3, 4 };
        vector<int> sites( s, s + sizeof( s ) / sizeof( int ) );
        JajodiaMutchler::instance().close_connections( sites );
    }

    if ( server_num == 2 || server_num == 3 || server_num == 4 )
    {
        int s[] = { 1 };
        vector<int> sites( s, s + sizeof( s ) / sizeof( int ) );
        JajodiaMutchler::instance().close_connections( sites );
    }

    if ( server_num == 5 || server_num == 6 || server_num == 7 )
    {
        int s[] = { 8 };
        vector<int> sites( s, s + sizeof( s ) / sizeof( int ) );
        JajodiaMutchler::instance().close_connections( sites );
    }

    if ( server_num == 8 )
    {
        int s[] = { 5, 6, 7 };
        vector<int> sites( s, s + sizeof( s ) / sizeof( int ) );
        JajodiaMutchler::instance().close_connections( sites );
    }

    for ( int i = 0; i < NUM_OF_UPDATES; i++ )
    {
        JajodiaMutchler::instance().execute_update();
    }

    // stage 4.
    // Do merge
    if ( server_num == 2 || server_num == 3 || server_num == 4 )
    {
        int s[] = { 5, 6, 7 };
        vector<int> sites( s, s + sizeof( s ) / sizeof( int ) );
        JajodiaMutchler::instance().open_connections( sites );
    }

    if ( server_num == 5 || server_num == 6 || server_num == 7 )
    {
        int s[] = { 2, 3, 4 };
        vector<int> sites( s, s + sizeof( s ) / sizeof( int ) );
        JajodiaMutchler::instance().open_connections( sites );
    }

    for ( int i = 0; i < NUM_OF_UPDATES; i++ )
    {
        JajodiaMutchler::instance().execute_update();
    }

    while( !do_terminate )
    {
        usleep( 10000 );
    }

    cout << "[Server] " << my_conf.number << "Buy :)" << endl;
    return 0;
}

void run_server( int port )
{
    // Init callbacks
    SimpleMessage client_data;
    CallbackBridge client_sock_cb;
    client_sock_cb.subscribe( &on_client_data, &client_data );

    // start TCP server
    TcpServer server( port, &client_sock_cb, sizeof( SimpleMessage ) );
    server_ptr = &server;
    try
    {
        cout << "[INFO] Started listening on port " << port << endl;
        cout.flush();
        server.run();
    }
    catch ( Exception ex )
    {
        Utils::print_error( "server_main: "
            + string(ex.what()) );
        exit( EXIT_FAILURE );
    }
}

void on_client_data( void* data, TcpSocket* c_sock )
{
    SimpleMessage s_message = *( ( SimpleMessage* )data );
    SimpleMessage reply_msg;
    reply_msg.msg_t = RESULT;
    ReplyMessage *r_msg = &reply_msg.payload.reply_m;

    switch( s_message.msg_t )
    {
        case JAJODIA:
            JajodiaMutchler::instance().process_message( &s_message );
            break;

        default:
            break;
    }

    c_sock->send( &s_message, sizeof( SimpleMessage ) );
}
