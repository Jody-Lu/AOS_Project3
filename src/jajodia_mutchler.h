#ifndef JAJODIA_MUTCHLER_H
#define JAJODIA_MUTCHLER_H

#include "callback_bridge.h"
#include "config.h"
#include "connection_manager.h"

#include <vector>
#include <tuple>
#include <utility>

class JajodiaMutchler
{
public:
	static JajodiaMutchler& instance( void );

	void init( Config *c, int id );
	inline void close()
    {
        connections.close_all();
    }

    void execute_update( void );
    void broadcast_all( JAJODIA_MSG_TYPE type );
    void process_message( SimpleMessage* message );
    void close_connections( std::vector<int>& sites );

private:
	JajodiaMutchler() {}
	~JajodiaMutchler() {}

	void process_vote_request( JajodiaMessage* mm );
	void process_abort( JajodiaMessage* mm );
	void process_commit( JajodiaMessage* mm );
	void process_dryrun( JajodiaMessage* mm );
	void process_lock_request( void );
	void process_release_lock( void );
	void execute_dryrun( void );
	bool is_distinguished();
	void do_update( JajodiaMessage *mm );
	void catch_up();

	void send( int id, SimpleMessage& message );

	static JajodiaMutchler instance_;

	ConnectionManager connections;

	Config* config;
    int my_id;
    int my_token_holder;
    bool updating;

    // Dryrun variables
    bool dryrun;
    int min_site_id;
    int num_site_available;

    // Data structure for Jajodia-Mutchler algorithm
	int VN;     // version number
	int RU;     // replicas updated
	int DS;     // distinguished site

	int M;      // Max version number (most current)

	// Collect VN, RU, DS
	// pair<id, tuple<VN, RU, DS> >
	std::vector< std::pair< int, std::tuple<int, int, int> > > responses;
};

#endif