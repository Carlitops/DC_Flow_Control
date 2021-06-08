/*
 * pdpc_reordering.c
 *
 *  Created on: Jul 22, 2019
 *      Author: carlos
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "pdcp_reordering.h"
#include "pdcp_sequence_manager.h"
#include "rlc.h"

//Prototyping
struct pdcp_buff_t *pdcp_buffer_create(void);

void pdcp_buffer_init(void);

void unblock_sigusr2(void);

void block_sigusr2(void);

boolean_t check_pdcp_buff_SN_in(struct pdcp_buff_t *buffer, pdcp_hfn_t hfn, pdcp_sn_t sn, unsigned int seq_num_size);

void insert_pdcp_buff_elmt(struct pdcp_buff_t *buffer, unsigned char *sdu, sdu_size_t size,
			   	   	   	   pdcp_sn_t sn,
						   pdcp_hfn_t hfn,
						   const rb_id_t rb_id,
						   unsigned int seq_num_size);

struct pdcp_buff_elmt *remove_highcount_pdcp_buff_elmt (struct pdcp_buff_t *buffer, pdcp_hfn_t hfn, pdcp_sn_t sn, unsigned int seq_num_size);

boolean_t check_timer_armed (timer_t _timerid);

void disarm_timer (timer_t _timerid);

void arm_timer(timer_t _timerid);

int sn_minus(pdcp_sn_t sn1, pdcp_hfn_t hfn1, pdcp_sn_t sn2, pdcp_hfn_t hfn2, unsigned int seq_num_size);

void pdcp_timer_handler(int sig, siginfo_t *si, void *uc);

struct pdcp_buff_elmt *remove_lesscount_pdcp_buff_elmt(struct pdcp_buff_t *buffer, pdcp_hfn_t hfn, pdcp_sn_t sn);

struct pdcp_buff_elmt *remove_lowestSN_pdcp_buff_elmt(struct pdcp_buff_t *buffer);

////////////////////////////////////////

struct pdcp_buff_t *pdcp_buffer_create()
{
	struct pdcp_buff_t *new_buffer;
	new_buffer = malloc(sizeof(struct pdcp_buff_t));
	new_buffer->head = NULL;
	new_buffer->tail = NULL;
	new_buffer->cnt = 0;
	printf("Buffer created\n");
	return new_buffer;
}

// Init PDCP buffer
void pdcp_buffer_init(){
	pdcp_buffer = pdcp_buffer_create();
// Timer

	nb_rx_pdus  = 0;
	struct sigevent sev;
   	long long freq_nanosecs;
   	sigset_t mask;
   	struct sigaction sa;

   	sa.sa_flags = SA_SIGINFO;
   	sa.sa_sigaction = pdcp_timer_handler;
   	sigemptyset(&sa.sa_mask);
   	sigaction(SIG, &sa, NULL);
   	sev.sigev_notify = SIGEV_SIGNAL;
   	sev.sigev_signo = SIG;
   	sev.sigev_value.sival_ptr = &t_reordering;
   	timer_create(CLOCKID, &sev, &t_reordering);

// Mutex
	/*pthread_mutex_init(&pdcp_buffer_mutex,NULL);
	key_glob            = HASHTABLE_NOT_A_KEY_VALUE;
	timer_ticks = 0;
	LOG_D(PDCP, "PDCP Buffer initialized\n");*/
}



void unblock_sigusr2()
{
		sigset_t signal_set;
		sigemptyset(&signal_set);

		sigaddset(&signal_set, SIGUSR2);
		sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
}

void block_sigusr2()
{
	// FIXME : Trying to blocks signals
		sigset_t signal_set;
		sigemptyset(&signal_set);

		sigaddset(&signal_set, SIGUSR2);
		sigprocmask(SIG_BLOCK, &signal_set, NULL);
	/****/
}

boolean_t check_pdcp_buff_SN_in(struct pdcp_buff_t *buffer, pdcp_hfn_t hfn, pdcp_sn_t sn, unsigned int seq_num_size){
	printf("we are in check_pdcp_buff_SN_in \n");

	struct pdcp_buff_elmt *ptr;

	if (buffer->cnt == 0)
		return false;

	// The head is higher than sn
	if ((buffer->head->hfn > hfn) || ( (buffer->head->hfn == hfn) && (buffer->head->sn > sn)))
		return false;

	// The tail is lower than sn
	if ((buffer->tail->hfn < hfn) || ((buffer->tail->hfn == hfn) && (buffer->tail->sn < sn)))
		return false;

	// Check if it is better to browse from the end !
	// FIXME: Again this is not always the case, but this may save some time !
	// We just check if where there is (maybe) less PDUs to cross

	// Browse from the end !
	if (sn_minus(buffer->tail->sn, buffer->tail->hfn, sn, hfn, seq_num_size) <= sn_minus(sn, hfn,
			buffer->head->sn, buffer->head->hfn, seq_num_size)){
		ptr = buffer->tail;
		while (ptr != NULL){
			if ( (ptr->sn == sn) && (ptr->hfn == hfn) )
			    return true;

			ptr = ptr->prev;
		}
	} else {
		ptr = buffer->head;
		while (ptr != NULL){
			if ( (ptr->sn == sn) && (ptr->hfn == hfn) )
			    return true;

			ptr = ptr->next;
		}
	}
	return false;
}

void insert_pdcp_buff_elmt(struct pdcp_buff_t *buffer, unsigned char *sdu, sdu_size_t size,
			   	   	   	   pdcp_sn_t sn, pdcp_hfn_t hfn, const rb_id_t rb_id, unsigned int seq_num_size){
	printf("we are in insert_pdcp_buff_elmt\n");

	struct pdcp_buff_elmt *new_elmt = NULL;

	// Create the new elmt
	new_elmt = malloc(sizeof(struct pdcp_buff_elmt));//creo falta el cast

	if (new_elmt == NULL){
		perror("[PDCP-BUFF] error allocating new space\n");
		return;
	}
	new_elmt->sdu = malloc(size);//creo falta el cast

	if (new_elmt->sdu == NULL){
		perror("[PDCP-BUFF] error allocating new_elmt->sdu\n");
		free(new_elmt);
		return;
	}

	memcpy(new_elmt->sdu, sdu, size);
	memcpy(&new_elmt->rb_id, &rb_id, sizeof(rb_id_t));
	memcpy(&new_elmt->size, &size, sizeof(sdu_size_t ));
	memcpy(&new_elmt->sn, &sn, sizeof(pdcp_sn_t));
	memcpy(&new_elmt->hfn, &hfn, sizeof(pdcp_hfn_t));
	new_elmt->nc = NULL;
	new_elmt->next = NULL;
	new_elmt->prev = NULL;

	if (buffer->cnt == 0) { //buffer is empty
		new_elmt->next = NULL;
		new_elmt->prev = NULL;
		buffer->head = new_elmt;
		buffer->tail = new_elmt;
	}
	else // place it at the correct place
	{
		if ( ((buffer->head->hfn == hfn) && (buffer->head->sn > sn)) ||  ((buffer->head->hfn > hfn))){
			// Insert in the beginning (new head)

			new_elmt->next = buffer->head;
			buffer->head->prev = new_elmt;
			buffer->head = new_elmt;

		}else if (((buffer->tail->hfn == hfn) && (buffer->tail->sn < sn)) || ((buffer->tail->hfn < hfn))){
			// Insert directly in the tail
			new_elmt->prev = buffer->tail;
			buffer->tail->next = new_elmt;
			buffer->tail = new_elmt;

		} else // In the middle, we need to browse the list
			{
			struct pdcp_buff_elmt *ptr;

			// Check if it is better to start from the end or the beginning !
			// FIXME: this is non alwayse the case, but we can save some time with this !

			// Browse starting from the tail
			if ( sn_minus(sn, hfn, buffer->tail->sn, buffer->tail->hfn, seq_num_size) <= sn_minus(buffer->head->sn, buffer->head->hfn, sn, hfn, seq_num_size) ){
				ptr = buffer->tail; // points to the tail

				while (ptr->prev != NULL){
					if (ptr->sn == sn && ptr->hfn == hfn) // This happens only if this is an NC PDU, so it corresponds to a more
						break;
					if (ptr->prev->hfn < hfn)
						break;
					if ( ptr->sn > sn && ptr->prev->sn < sn)
						break;

					ptr = ptr->prev;
				}

				if (ptr->sn == sn && ptr->hfn == hfn) // the pdu is a modificaiton, browse on the nc side (see figure in the beginning of this file)
				{
					while (ptr->nc != NULL)
						ptr = ptr->nc;

					// we are at the correct place, this is actually always the end
					ptr->nc = new_elmt;
				} else {
					new_elmt->next 		= ptr;

					if (ptr->prev != NULL){
						new_elmt->prev 		= ptr->prev;
						ptr->prev->next 	= new_elmt;
					} else {
						new_elmt->prev 		= NULL;
					}
					ptr->prev 		= new_elmt;
				}
			} else // Maybe better to start from head
				{
				ptr 	= buffer->head; // points to the tail

				while (ptr->next != NULL)
				{
					if (ptr->sn == sn && ptr->hfn == hfn) // This happens only if this is an NC PDU, so it corresponds to a more
						break;
					if ( ptr->next->hfn > hfn )
						break;
					if ( ptr->sn < sn && ptr->next->sn > sn) // elmt has to be inserted in the begining
						break;

					ptr = ptr->next;
				}

				if (ptr->sn == sn && ptr->hfn == hfn) // the pdu is an nc modificaiton, browse on the nc side (see figure in the beginning of this file)
				{
					while (ptr->nc != NULL)
						ptr = ptr->nc;

					// we are at the correct place, this is actually always the end
					ptr->nc = new_elmt;
				}
				else
				{
					new_elmt->prev 		= ptr;

					if (ptr->next != NULL)
					{
						new_elmt->next 		= ptr->next;
						ptr->next->prev 	= new_elmt;
					}
					else
					{
						new_elmt->next 		= NULL;
					}

					ptr->next 		= new_elmt;
				}

			}
		}
	}

	buffer->cnt++;
	printf("cnt: %d\n",buffer->cnt);
}

struct pdcp_buff_elmt *remove_highcount_pdcp_buff_elmt (struct pdcp_buff_t *buffer, pdcp_hfn_t hfn, pdcp_sn_t sn, unsigned int seq_num_size){
/*
	if (buffer->cnt == 0){
		return NULL;
	}
	struct pdcp_buff_elmt *ptr = buffer->head;

	// Only one elemt
	if (buffer->cnt == 1){
	    if (((ptr->sn == sn) && (ptr->hfn == hfn)) // The SN indicated in the args
	      || ((ptr->sn == sn + 1) && (ptr->hfn == hfn)) // Same hfn, and sn+1
	      || ((ptr->sn == 0) && (sn == pdcp_calculate_max_seq_num_for_given_size(seq_num_size) - 1) && (ptr->hfn + 1 == hfn))) {// the next hfn

	    	buffer->head = NULL;
	    	buffer->tail = NULL;
	    	buffer->cnt = 0;
	    	return ptr;
	    } else
	    	return NULL;
	}

	// FIXME: What's bellow has not been adapted to the new buffer structure !!

	while (ptr != NULL)
	{
		if ( ( (ptr->sn == sn) && (ptr->hfn == hfn) ) // The SN indicated in the args
		  || ( (ptr->sn == sn + 1) && (ptr->hfn == hfn) ) // Same hfn, and sn+1
		  || ( (ptr->sn == 0) && (sn == pdcp_calculate_max_seq_num_for_given_size(seq_num_size) - 1) && (ptr->hfn == hfn) ) // the next hfn
		   )
			break;
		ptr = ptr->next;
	}
	if (ptr == NULL) {
		return NULL;
	}

	if (ptr->prev == NULL){ // We are at the begining

		buffer->head->next->prev = NULL;
		buffer->head = buffer->head->next;
		buffer->cnt--;
		return ptr;
	}

	if (ptr->next == NULL) // Last elmt
	{
		buffer->tail->prev->next = NULL;
		buffer->tail = buffer->tail->prev;
		buffer->cnt--;
		return ptr;
	}

	ptr->prev->next = ptr->next;
	ptr->next->prev = ptr->prev;
	buffer->cnt--;
	return ptr;
*/

	if (buffer == NULL)
		return NULL;

	struct pdcp_buff_elmt *ptr = buffer;

	pdcp_t *pdcp_p = NULL;
        hashtable_rc_t  h_rc;

        h_rc = hashtable_get(pdcp_coll_p, key_glob, (void**)&pdcp_p); // get the pdcp entity

	//printf("highcoung removing %d\n", sn);

        if (h_rc != HASH_TABLE_OK) {
		printf("[PDCP][Timer] Error getting pdcp entity\n");
		return NULL;
        }

	// Only one elemt, and this is the right one
	if ( (ptr->next == NULL) && (
	     ( (ptr->sn == sn) && (ptr->hfn == hfn) ) // The SN indicated in the args
	  || ( (ptr->sn == sn + 1) && (ptr->hfn == hfn) ) // Same hfn, and sn+1
	  || ( (ptr->sn == 0) && (sn == pdcp_calculate_max_seq_num_for_given_size(pdcp_p->seq_num_size)) && (ptr->hfn == hfn) ) // the next hfn
           ))
	{
		//printf("here\n");
		buffer = NULL;
		return ptr;
	}

	while (ptr != NULL)
	{
		if ( ( (ptr->sn == sn) && (ptr->hfn == hfn) ) // The SN indicated in the args
		  || ( (ptr->sn == sn + 1) && (ptr->hfn == hfn) ) // Same hfn, and sn+1
		  || ( (ptr->sn == 0) && (sn == pdcp_calculate_max_seq_num_for_given_size(pdcp_p->seq_num_size)) && (ptr->hfn == hfn) ) // the next hfn
		   )
			break;
		ptr = ptr->next;
	}
	if (ptr == NULL)
	{
		return NULL;
	}

	if (ptr->prev == NULL) // We are at the begining
	{
		buffer->head->prev = NULL;
		buffer = buffer->head->next;

		return ptr;
	}

	if (ptr->next == NULL) // Last elmt
	{
		ptr->prev->next = NULL;
		return ptr;
	}

	ptr->prev->next = ptr->next;
	ptr->next->prev = ptr->prev;
	return ptr;
}


boolean_t check_timer_armed(timer_t _timerid){
	struct itimerspec its;
	timer_gettime(_timerid, &its);
	if ((its.it_value.tv_sec == 0) && (its.it_value.tv_nsec == 0)){
		return FALSE;
	}

	return TRUE;
}

void disarm_timer (timer_t _timerid){
	struct itimerspec its;

	its.it_value.tv_sec 	= 0;
    its.it_value.tv_nsec 	= 0;
   	its.it_interval.tv_sec 	= 0;
    its.it_interval.tv_nsec = 0;
	timer_settime(_timerid, 0, &its, NULL);
}

void arm_timer(timer_t _timerid){
	struct itimerspec its;

	its.it_value.tv_sec 	= 0;
    its.it_value.tv_nsec 	= TIMER_PDCP_REORD;
   	its.it_interval.tv_sec 	= its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

	timer_settime(_timerid, 0, &its, NULL);
}

int sn_minus(pdcp_sn_t sn1, pdcp_hfn_t hfn1, pdcp_sn_t sn2, pdcp_hfn_t hfn2, unsigned int seq_num_size){
	/*Compute sn2 - sn1 : compute the nb of PDU there is between sn2 and sn1*/
	if (hfn1 == hfn2)
		return ((int)sn2 - (int)sn1);

	// hfn2 > hfn1
	pdcp_sn_t max_sn = pdcp_calculate_max_seq_num_for_given_size(seq_num_size);

	return ((int)sn2 + ((int)max_sn - (int)sn1 + 1));
}

void pdcp_timer_handler(int sig, siginfo_t *si, void *uc){

    disarm_timer(t_reordering);

    struct pdcp_buff_elmt *elmt_ret;
    pdcp_t *pdcp_p = NULL;
    hashtable_rc_t  h_rc;
    mem_block_t *new_sdu_p       = NULL;

    /*if (pthread_mutex_lock(&pdcp_buffer_mutex) != 0) {
		      printf("[PDCP][Timer] error locking mutex \n");
		      return;
	        }*/

    list_t *sdu_list_p = &pdcp_sdu_list;
    h_rc = hashtable_get(pdcp_coll_p, key_glob, (void**)&pdcp_p); // get the pdcp entity

    if (h_rc != HASH_TABLE_OK) {
	printf("[PDCP][Timer] Error getting pdcp entity\n");
	/*if (pthread_mutex_unlock(&pdcp_buffer_mutex) != 0) {
		      printf("[PDCP][UE] error unlocking mutex for UE \n");
		      return;
		    }*/
	return;
  }

    timer_ticks++;
    pdcp_sn_t last_sn_ret = pdcp_p->reordering_sn;
    pdcp_hfn_t last_hfn_ret = pdcp_p->reordering_hfn;

    if (pdcp_buffer != NULL) // Not empty
    {
	while ( (elmt_ret = remove_lesscount_pdcp_buff_elmt(pdcp_buffer, pdcp_p->reordering_hfn, pdcp_p->reordering_sn)) != NULL){

		new_sdu_p = get_free_mem_block(elmt_ret->size + sizeof (pdcp_data_ind_header_t), __func__);

		if (new_sdu_p) {

		      memset(new_sdu_p->data, 0, sizeof (pdcp_data_ind_header_t));
		      ((pdcp_data_ind_header_t *) new_sdu_p->data)->data_size = elmt_ret->size;
		      ((pdcp_data_ind_header_t *) new_sdu_p->data)->rb_id = elmt_ret->rb_id;

		      memcpy(&new_sdu_p->data[sizeof (pdcp_data_ind_header_t)], elmt_ret->sdu, elmt_ret->size);

		      list_add_tail_eurecom (new_sdu_p, sdu_list_p);

			pdcp_p->last_submitted_pdcp_rx_sn = elmt_ret->sn;
        	pdcp_p->last_hfn = elmt_ret->hfn;
		}
		else
			printf("Error allocating new buffer in timer handler\n");

		//free(elmt_ret);
		elmt_ret = NULL;
	}
	// In UM mode, PDUs are not retransmitted. We need to deliver stored PDUs after 2 timers
	// this is not in the spec, we're only trying
	if (pdcp_p->rlc_mode == RLC_MODE_UM && timer_ticks == 2){

		if ( (elmt_ret = remove_lowestSN_pdcp_buff_elmt(pdcp_buffer)) != NULL){

			new_sdu_p = get_free_mem_block(elmt_ret->size + sizeof (pdcp_data_ind_header_t), __func__);

			if (new_sdu_p) {

			   memset(new_sdu_p->data, 0, sizeof (pdcp_data_ind_header_t));
			   ((pdcp_data_ind_header_t *) new_sdu_p->data)->data_size = elmt_ret->size;
			   ((pdcp_data_ind_header_t *) new_sdu_p->data)->rb_id = elmt_ret->rb_id;

			   memcpy(&new_sdu_p->data[sizeof (pdcp_data_ind_header_t)], elmt_ret->sdu, elmt_ret->size);
			   list_add_tail_eurecom (new_sdu_p, sdu_list_p);
			   last_sn_ret = elmt_ret->sn;
			   last_hfn_ret = elmt_ret->hfn;
			   pdcp_p->last_submitted_pdcp_rx_sn = elmt_ret->sn;
			   pdcp_p->last_hfn = elmt_ret->hfn;
			}
			else
				printf("Error allocating new buffer in timer handler\n");
			//free(elmt_ret);
			elmt_ret = NULL;
		} // End remove_lowestelemt
	} //End of RLC_UM

	while ( (elmt_ret = remove_highcount_pdcp_buff_elmt(pdcp_buffer, last_hfn_ret ,last_sn_ret, pdcp_p->seq_num_size)) != NULL)	{
		new_sdu_p = get_free_mem_block(elmt_ret->size + sizeof (pdcp_data_ind_header_t), __func__);

		if (new_sdu_p) {

		   memset(new_sdu_p->data, 0, sizeof (pdcp_data_ind_header_t));
		   ((pdcp_data_ind_header_t *) new_sdu_p->data)->data_size = elmt_ret->size;
		   ((pdcp_data_ind_header_t *) new_sdu_p->data)->rb_id = elmt_ret->rb_id;

		   memcpy(&new_sdu_p->data[sizeof (pdcp_data_ind_header_t)], elmt_ret->sdu, elmt_ret->size);

		   list_add_tail_eurecom (new_sdu_p, sdu_list_p);
		   last_sn_ret = elmt_ret->sn;
		   last_hfn_ret = elmt_ret->hfn;
		   pdcp_p->last_submitted_pdcp_rx_sn = elmt_ret->sn;
		   pdcp_p->last_hfn = elmt_ret->hfn;
		}
		else
			printf("Error allocating new buffer in timer handler\n");
		//free(elmt_ret);
		elmt_ret = NULL;
	}

	if (pdcp_buffer != NULL){
		arm_timer(t_reordering);
		pdcp_p->reordering_sn = pdcp_p->next_pdcp_tx_sn;
		pdcp_p->reordering_hfn = pdcp_p->rx_hfn;
	}
   }

    if (timer_ticks == 2)
    	timer_ticks = 0;

    /*if (pthread_mutex_unlock(&pdcp_buffer_mutex) != 0) {
		      printf("[UE] error unlocking mutex for UE \n" );
		      return;
    }*/

    //printf("out of handler\n");
}

struct pdcp_buff_elmt *remove_lesscount_pdcp_buff_elmt(struct pdcp_buff_t *buffer, pdcp_hfn_t hfn, pdcp_sn_t sn){
	/* Remove the elemt with the lowest COUNT lower than COUNT in arg*/

	struct pdcp_buff_elmt *ptr = buffer->head;

	if (buffer->cnt == 0)
		return NULL;

	if ((ptr->next == NULL) &&  (((ptr->sn < sn) && (ptr->hfn <= hfn)) || (ptr->hfn < hfn) )) // Only one elemt and corresponds
	{
		buffer->head = NULL;
		buffer->tail = NULL;
		buffer->cnt = 0;
		return ptr;
	}

	while (ptr != NULL)	{
		if (((ptr->sn < sn) && (ptr->hfn <= hfn)) || (ptr->hfn < hfn) )
			break;
		ptr = ptr->next;
	}

	if (ptr == NULL)
		return NULL;

	if (ptr->prev == NULL) // We are at the begining
	{
		buffer->head->next->prev = NULL;
		buffer->head = buffer->head->next;
		buffer->cnt--;

		return ptr;
	}

	// We should never get to this case as the list is ordered
	ptr->prev->next = ptr->next;
	ptr->next->prev = ptr->prev;
	return ptr;

}

struct pdcp_buff_elmt *remove_lowestSN_pdcp_buff_elmt(struct pdcp_buff_t *buffer){
	/* Remove the elemt with the lowest SN */
	if ( buffer->cnt == 0)
		return NULL;

	struct pdcp_buff_elmt *ptr = buffer->head;

	if (buffer->cnt == 1) // Only one elemt
	{
		buffer->head = NULL;
		buffer->tail = NULL;
	} else {
		// New head
		buffer->head->next->prev = NULL;
		buffer->head = buffer->head->next;
	}
	buffer->cnt--;
	return ptr;
}

