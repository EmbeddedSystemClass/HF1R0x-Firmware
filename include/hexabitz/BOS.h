#ifndef BOS_H
#define BOS_H

#include "helper/helper.h"

#include <stdint.h>
#include <stdbool.h>

#include <vector>
#include <string>

#define	CODE_unknown_message							0
#define	CODE_ping										1
#define	CODE_ping_response								2
#define	CODE_IND_on										3
#define	CODE_IND_off									4
#define	CODE_IND_toggle									5

#define	CODE_hi											10
#define	CODE_hi_response								11
#define	CODE_explore_adj								12
#define	CODE_explore_adj_response						13
#define	CODE_port_dir									14
#define	CODE_baudrate									15
#define	CODE_module_id									16
#define	CODE_topology									17
#define	CODE_broadcast_plan								18
#define	CODE_read_port_dir								19
#define	CODE_read_port_dir_response						20
#define	CODE_exp_eeprom	 								21
#define	CODE_def_array	 								22
#define	CODE_CLI_command 								23
#define	CODE_CLI_response  								24
#define	CODE_update  									25
#define	CODE_update_via_port  							26
#define	CODE_DMA_channel  								27
#define	CODE_DMA_scast_stream  							28

#define	CODE_read_remote  								30
#define	CODE_read_remote_response  						31
#define	CODE_write_remote  								32
#define	CODE_write_remote_response  					33
#define	CODE_write_remote_force							34


#define SEQ(OPERATION) 							\
		OPERATION(		INVALID		,	   -1		)  	\
		OPERATION(		H01R0		,		6		) 	\
		OPERATION(		P01R0		,		6		) 	\
		OPERATION(		H23R0		,		6		) 	\
		OPERATION(		H23R1		,		6		) 	\
		OPERATION(		H07R3		,		6		) 	\
		OPERATION(		H08R6		,		6		) 	\
		OPERATION(		H09R0		,		6		) 	\
		OPERATION(		H1BR6		,		6		) 	\
		OPERATION(		H12R0		,		6		) 	\
		OPERATION(		H13R7		,		6		) 	\
		OPERATION(		H0FR6		,		6		) 	\
		OPERATION(		H1AR2		,		6		) 	\
		OPERATION(		H0AR9		,		6		) 	\
		OPERATION(		H1DR1		,		6		) 	\
		OPERATION(		H1DR5		,		6		) 	\
		OPERATION(		H0BR4		,		6		) 	\
		OPERATION(		H18R0		,		6		) 	\
		OPERATION(		H26R0		,		6		)	\
		OPERATION(		HF1R0		,		2		)



namespace BOS {

static constexpr int MAX_NUM_OF_PORTS = 10;
static constexpr int MAX_NUM_OF_MODULES = 25;

enum module_pn_e {
#define TO_ENUM_PART_NUMBER(p,n)					TO_ENUM(p)

	SEQ(TO_ENUM_PART_NUMBER)
	
#undef TO_ENUM_PART_NUMBER
};


enum class PortDir {
	NORMAL, 
	REVERSED
};


std::string toString(enum module_pn_e pn);
enum module_pn_e toPartNumberEnum(std::string str);
std::vector<std::string> getPartNumberList(void);
int getNumOfPorts(enum module_pn_e partNum);

}



#endif /* BOS_H */