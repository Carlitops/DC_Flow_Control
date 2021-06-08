/*
 * pdcp_reordering.h
 *
 *  Created on: Jul 22, 2019
 *      Author: carlos
 */

#ifndef PDCP_REORDERING_H_
#define PDCP_REORDERING_H_

#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "hashtable.h"
#include <stdbool.h>
#include "pdcp.h"
#include "UTIL/MEM/mem_block.h"
#include "COMMON/platform_types.h"

#define CLOCKID CLOCK_REALTIME
#define SIG SIGUSR2
#define TIMER_PDCP_REORD 100000000

unsigned int nb_rx_pdus; // total number of received pdus!
timer_t t_reordering; // pdcp t_reordering timer
hash_key_t      key_glob;
pthread_mutex_t pdcp_buffer_mutex;
int timer_ticks;

struct pdcp_buff_elmt{
	unsigned char 	*sdu;
	rb_id_t       	rb_id; 	// useful to OAI
	sdu_size_t 		size;
	pdcp_sn_t 		sn;		// seq nb, we need this as the header has already been removed
	pdcp_hfn_t 		hfn;
	bool 			wifi; 	// received from WiFi or LTE
	bool delivered; 		// indicates that the SDU has already been delivered to upper layer (used in NC)
	struct pdcp_buff_elmt *nc; // only used for NC buffer, and points to the next level of modification
	struct pdcp_buff_elmt *next;
	struct pdcp_buff_elmt *prev;
};

// Buffer structure
struct pdcp_buff_t{
	unsigned int cnt;
	struct pdcp_buff_elmt *head;
	struct pdcp_buff_elmt *tail;
};

struct pdcp_buff_t *pdcp_buffer;
pdcp_sn_t 	first_sn;
pdcp_hfn_t 	first_hfn;

unsigned int cnt_pdcp_buffer;

void pdcp_buffer_init(void);

struct pdcp_buff_t *pdcp_buffer_create(void);

void block_sigusr2(void);

void unblock_sigusr2(void);

boolean_t check_pdcp_buff_SN_in(struct pdcp_buff_t *buffer, pdcp_hfn_t hfn, pdcp_sn_t sn, unsigned int seq_num_size);

void insert_pdcp_buff_elmt(struct pdcp_buff_t *buffer, unsigned char *sdu, sdu_size_t size,
			   	   	   	   pdcp_sn_t sn, pdcp_hfn_t hfn, const rb_id_t rb_id, unsigned int seq_num_size);

struct pdcp_buff_elmt *remove_highcount_pdcp_buff_elmt (struct pdcp_buff_t *buffer, pdcp_hfn_t hfn, pdcp_sn_t sn, unsigned int seq_num_size);

boolean_t check_timer_armed(timer_t _timerid);

void arm_timer(timer_t _timerid);

void disarm_timer (timer_t _timerid);

int sn_minus(pdcp_sn_t sn1, pdcp_hfn_t hfn1, pdcp_sn_t sn2, pdcp_hfn_t hfn2, unsigned int seq_num_size);

void pdcp_timer_handler(int sig, siginfo_t *si, void *uc);

struct pdcp_buff_elmt *remove_lesscount_pdcp_buff_elmt(struct pdcp_buff_t *buffer, pdcp_hfn_t hfn, pdcp_sn_t sn);

struct pdcp_buff_elmt *remove_lowestSN_pdcp_buff_elmt(struct pdcp_buff_t *buffer);

#endif /* OPENAIR2_LAYER2_PDCP_V10_1_0_PDCP_REORDERING_H_ */


