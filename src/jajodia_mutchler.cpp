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

	// dryrun variable
	dryrun = false;
	min_site_id = my_id;
	num_site_available = 0;

	// Inititialize connection with configs
	connections.init( c->get_all() );

	// Connect to all other partition peers
	connections.connect_all();
}

void JajodiaMutchler::execute_dryrun( void )
{
	num_site_available = 0;
	min_site_id = my_id;

	int sleep_mil = ( rand() % 95 ) + 90;
	usleep( sleep_mil * 1000 );

	broadcast_all( DRYRUN );
}

// Execute update operation
void JajodiaMutchler::execute_update( void )
{
	// launch dry run

	execute_dryrun();
	std::cout << "[INFO] Execute Dryrun" << std::endl;

	while ( dryrun )
	{
		usleep( TIME_WAIT_RESPONSE );
	}


	// Get #connections
	int num_connections = num_site_available + 1;

	std::cout << "Minimum site: " << min_site_id << std::endl;
	std::cout << "Number of reachable sites: " << num_site_available << std::endl;

	if ( my_id != min_site_id )
	{
		return;
	}

	// Issue LOCK_REQUEST
	process_lock_request();

	// Perform tentative update
	// TODO

	// Push this node's VN, RU and DS into collections
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
		// Done - Genya
		// S does belong to distinguished partition
		// 1. Modify this node's VN, RU, DS
		VN = M + 1;
		RU = responses.size();
		int newDS = std::get<2>( responses[0].second ); // update to Min DS in responses
        for ( int i = 1; i < responses.size(); i++ )
        {
            int tempDS = std::get<2>( responses[i].second );
            if ( newDS > tempDS )
            {
                newDS = tempDS;
            }
        }
        DS = newDS;

        // 2. Send COMMIT message
        broadcast_all( COMMIT );

		// 3. Issue RELEASE_LOCK
        process_release_lock();
	}

	// clear responses
	responses.clear();
}

// Broadcast message to all other peers in the same partition
void JajodiaMutchler::broadcast_all( JAJODIA_MSG_TYPE type )
{
	if ( type == DRYRUN )
	{
		dryrun = true;
	}

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

		// added - Genya
		mm->VN = VN;
        mm->RU = RU;
        mm->DS = DS;

		// catch exception here
		try
		{
			conn.send( &message, sizeof( SimpleMessage ) );
		}
		catch ( Exception ex )
		{
			std::cout << "[DEBUG] Server " << conn.getId()
					  << " is not vailable" << std::endl;
			continue;
		}

		conn.receive( &message, sizeof( SimpleMessage ) );

		// If broadcasting message is VOTE_REQUEST,
		// then collect VNi, RUi and DSi
		if ( type == VOTE_REQUEST )
		{
			JajodiaMessage *r = &message.payload.jajodia_m;
			//std::cout << "[DEBUG] From ID: " << r->id << std::endl;
			//std::cout << "[DEBUG] " << r->VN << " " << r->RU << " " << r->DS << std::endl;
			responses.push_back( std::make_pair( r->id, std::make_tuple( r->VN, r->RU, r->DS ) ) );
		}
		else if ( type == DRYRUN )
		{
			JajodiaMessage *r = &message.payload.jajodia_m;
			if ( r->id < min_site_id )
			{
				min_site_id = r->id;
			}
			num_site_available++;
		}
	}

	if ( type == DRYRUN )
	{
		dryrun = false;
	}
}

void JajodiaMutchler::process_release_lock( void )
{
	updating = false;
	std::cout << "[INFO] after release VN = "
			  << VN << " RU = " << RU
			  << " DS = " << DS << std::endl; // added - Genya
}

void JajodiaMutchler::process_abort( JajodiaMessage* mm )
{
	process_release_lock();
}

void JajodiaMutchler::process_dryrun( JajodiaMessage* mm )
{
	mm->id = my_id;
}

// process coming message according to its message type
void JajodiaMutchler::process_message( SimpleMessage *message )
{
	//connections.wait_for_connections();

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

		case DRYRUN:
			process_dryrun( mm );
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

	// Done - Genya
	// 1. Set new VN, RU, DS (call do_update)
	do_update( mm );
	// 2. Issue RELEASE_LOCK
	process_release_lock( );
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
			//std::cout << "[DEBUG] Set I: " << responses[i].first << std::endl;
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
		// Done - Genya
		bool hasDS = false;
        for ( int i = 0; i < responses.size(); i++ )
        {
            if ( responses[i].first == DS )
            {
                hasDS = true;
            }
        }
        return hasDS;
	}

	// case (3)
	if ( I.size() < N / 2 )
	{
		return false;
	}

	return true;
}

void JajodiaMutchler::do_update( JajodiaMessage *mm )
{
	// Done - Genya
	// 1. update VN, RU, DS
    VN = mm->VN;
    RU = mm->RU;
    DS = mm->DS;
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

void JajodiaMutchler::close_connections( std::vector<int>& sites )
{
	for ( int i = 0; i < sites.size(); i++ )
	{
		connections.close( sites[i] );
	}
}























