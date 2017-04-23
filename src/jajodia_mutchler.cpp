#include "jajodia_mutchler.h"

#include "constants.h"
#include "utils.h"

#include <unistd.h>
#include <time.h>
#include <iostream>

#define TIME_WAIT_RESPONSE 10000

// Declare static instance
JajodiaMutchler JajodiaMutchler::instance_;

JajodiaMutchler& JajodiaMutchler::instance( void )
{
	return instance_;
}

// Initialize JajodiaMutchler
void JajodiaMutchler::init( Config* c, int id )
{
	// Initialize variables
	config = c;
	my_id = id;
	VN = 1;
	RU = 8;
	DS = 1;

	// Inititialize connection with configs
	connections.init( c->get_all() );

	// Connect to all other partition peers
	connections.connect_all();
}

// Execute update operation
void JajodiaMutchler::execute_update( void )
{
	// Get #connections
	int num_connections = connections.get_num_of_connection();

	// Issue LOCK_REQUEST
	process_lock_request();

	// Perform tentative update

	// push this node's VN, RU and DS into collections
	responses.push_back( std::make_pair( my_id, std::make_tuple( VN, RU, DS ) ) );

	// Send VOTE_REQUEST
	std::cout << "[INFO] Broadcast VOTE_REQUEST" << std::endl;
	broadcast_all( VOTE_REQUEST );

	// Wait until collect all responses
	// std::cout << num_connections << std::endl;
	while( responses.size() != num_connections )
	{
		usleep( TIME_WAIT_RESPONSE );
	}

	std::cout << "[INFO] Execute Is_Distinguished routine" << std::endl;

	if ( !is_distinguished() )
	{	// S does not belongs to distinguished partition

		// 1. Issue a RELEASE_LOCK to its local lock manager
		process_release_lock();

		// 2. Send ABORT to all the participants ( broadcast )
		broadcast_all( ABORT );
	}
	else
	{
		// TODO:
		// S does not belongs to distinguished partition
		// 1. Modify this node's VN, RU, DS
		// 2. Send COMMIT message
		// 3. Issue RELEASE_LOCK
	}

	// clear responses
	responses.clear();
}

// Broadcast message to all other peers in the same partition
void JajodiaMutchler::broadcast_all( JAJODIA_MSG_TYPE type )
{
	const std::vector<Connection>& conns = connections.get_all();

	SimpleMessage message;
	message.msg_t = JAJODIA;
	JajodiaMessage *mm = &message.payload.jajodia_m;

	for ( auto& conn : conns )
	{
		if ( conn.getId() == my_id )
		{
			continue;
		}
		mm->jajodia_t = type;
		mm->id = my_id;

		conn.send( &message, sizeof( SimpleMessage ) );
		conn.receive( &message, sizeof( SimpleMessage ) );

		// If broadcasting message is VOTE_REQUEST,
		// then collect VNi, RUi and DSi
		if ( type == VOTE_REQUEST )
		{
			JajodiaMessage *r = &message.payload.jajodia_m;
			std::cout << "[DEBUG] From ID: " << r->id << std::endl;
			std::cout << "[DEBUG] " << r->VN << " " << r->RU << " " << r->DS << std::endl;
			responses.push_back( std::make_pair( r->id, std::make_tuple( r->VN, r->RU, r->DS ) ) );
		}
	}
}

void JajodiaMutchler::process_release_lock( void )
{
	updating = false;
}

void JajodiaMutchler::process_abort( JajodiaMessage* mm )
{
	process_release_lock();
}

// process coming message according to its message type
void JajodiaMutchler::process_message( SimpleMessage *message )
{
	connections.wait_for_connections();

	JajodiaMessage* mm = &message->payload.jajodia_m;

	switch( mm->jajodia_t )
	{
		case VOTE_REQUEST:
			process_vote_request( mm );
			break;

		case ABORT:
			process_abort( mm );
			break;

		case COMMIT:
			process_commit( mm );
			break;

		default:
			break;
	}
}

void JajodiaMutchler::process_commit( JajodiaMessage *mm )
{
	// TODO
	// 1. Set new VN, RU, DS ( call do_update)
	// 2. Issue RELEASE_LOCK
}

// Procees the coming VOTE_REQUEST
void JajodiaMutchler::process_vote_request( JajodiaMessage *mm )
{
	std::cout << "[INFO] Receiving VOTE_REQUEST" << std::endl;

	// Issue LOCK_REQUEST to local lock manager
	process_lock_request( );

	// Update mm's information
	// It will automatically send back at server.cpp : 153
	mm->id = my_id;
	mm->VN = VN;
	mm->RU = RU;
	mm->DS = DS;
}

// Procees local lock operation
void JajodiaMutchler::process_lock_request( void )
{
	// Lock this process
	updating = true;
}

// Check the whether the parririon is distinguished
bool JajodiaMutchler::is_distinguished()
{
	int N;
	M = std::get<0>( responses[0].second ); // Max version number
	std::vector<int> I; // Sites with version number which is equal to M


	// Calculate M
	for ( int i = 0; i < responses.size(); i++ )
	{
		int VNi = std::get<0>( responses[i].second );
		if ( VNi > M )
		{
			M = VNi;
		}
	}

	std::cout << "[DEBUG] Current version: " << M << std::endl;

	// Get set I
	for ( int i = 0; i < responses.size(); i++ )
	{
		int VNi = std::get<0>( responses[i].second );
		if ( VNi == M )
		{
			std::cout << "[DEBUG] Set I: " << responses[i].first << std::endl;
			I.push_back( responses[i].first );
			N = std::get<1>( responses[i].second );
		}
	}

	// case (1)
	if ( I.size() > N / 2 )
	{
		return true;
	}

	// case (2)
	if ( I.size() == N / 2 )
	{
		// TODO
	}

	// case (3)
	if ( I.size() < N / 2 )
	{
		return false;
	}

	return true;
}

void JajodiaMutchler::do_update()
{
	// TODO
	// 1. update VN, RU, DS
}

// Send message to particular peer in the same partition
void JajodiaMutchler::send( int id, SimpleMessage &message )
{
	message.msg_t = JAJODIA;
	JajodiaMessage *mm = &message.payload.jajodia_m;
	mm->id = my_id;

	const Connection *conn = connections.get( id );
	conn->send( &message, sizeof( SimpleMessage ) );
}
























