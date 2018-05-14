#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "common.h"
#include "db_core.h"
//#include "db_content.h"

#include "dap_client.h"
#include "dap_http_client.h"

#include "stream.h"
#include "stream_ch.h"
#include "stream_ch_pkt.h"
#include "stream_ch_proc.h"

#include "media.h"
#include "media_mcod.h"
#include "ch_mcod.h"

#define LOG_TAG "ch_mcod"


struct stream_ch;

void ch_mcod_new(stream_ch_t * ch, void * arg);
void ch_mcod_delete(stream_ch_t * ch, void * arg);
void ch_mcod_packet_in(stream_ch_t * ch, void * arg);
void ch_mcod_packet_out(stream_ch_t * ch, void * arg);


/**
 * @brief ch_mcod_state_str
 * @param st
 * @return
 */
const char* ch_mcod_state_str(ch_mcod_state_t st)
{
    switch(st){
        case CH_MCOD_STATE_START: return "START";break;
        case CH_MCOD_STATE_PLAYING: return "PLAYING";break;
        case CH_MCOD_STATE_PAUSED: return "PAUSED";break;
        case CH_MCOD_STATE_END: return "END"; break;
        default: return "UNDEFINED";
    }

}

/**
 * @brief ch_mcod_init
 * @return
 */
int ch_mcod_init()
{
    if(media_init()!=0)
        return -1;

    stream_ch_proc_add('g',ch_mcod_new,ch_mcod_delete,
                       ch_mcod_packet_in,ch_mcod_packet_out);
    log_it(NOTICE,"Multimedia Content-On-Dement (MCOD)  initialized");
    return 0;
}

void ch_mcod_deinit()
{
    log_it(NOTICE,"Multimedia Content-On-Dement (MCOD) module deinit");
}

/**
 * @brief ch_mcod_new
 * @param ch
 * @param arg
 */
void ch_mcod_new(stream_ch_t * ch, void * arg)
{
    (void) arg;

    /* ERASE */
    /*
    ch->internal = calloc(1,sizeof(ch_mcod_t));
    ch_mcod_t * mcod= (ch_mcod_t*) ch->internal;



    mcod->fname=db_get_content_by_id(ch->stream->session->media_id);
    mcod->ch=ch;
    mcod->client_buf_time=CH_MCOD_SECOND*CH_MCOD_BUF_TIME;

    if(mcod->fname){
        log_it(INFO,"Requested file %s",mcod->fname);
        mcod->stream_size=db_get_content_length_by_id(ch->stream->session->media_id);
        mcod->media_type = db_get_content_type_by_id(ch->stream->session->media_id);

        mcod->mm=media_open(mcod,mcod->media_type,mcod->fname);
        if(mcod->mm == NULL){
            ch_mcod_set_state(mcod,CH_MCOD_STATE_END);
        }else{
            ch_mcod_set_state(mcod,CH_MCOD_STATE_START);
            log_it(INFO,"Everything initialized well");
        }
    }else{
        log_it(ERROR,"Haven't found file with db_id = %u",ch->stream->session->media_id);
        ch->stream->conn_http->reply_status_code=404;
        strcpy(ch->stream->conn_http->reply_reason_phrase,"Not found");
    } */
}


/**
 * @brief stream_ch_gst_delete
 * @param ch
 * @param arg
 */
void ch_mcod_delete(stream_ch_t * ch, void * arg)
{
    (void) arg;
    ch_mcod_t * mcod=CH_MCOD(ch);
    media_close(mcod->mm);
    if(mcod->fname)
        free(mcod->fname);
    //free(sg);
}

/**
 * @brief ch_mcod_packet_in
 * @param ch
 * @param arg
 */
void ch_mcod_packet_in(stream_ch_t * ch, void * arg)
{
    stream_ch_pkt_t * pkt = (stream_ch_pkt_t *) arg;
    ch_mcod_t * mcod = CH_MCOD(ch);
    media_t * mm = mcod->mm;

//    log_it(DEBUG, "Stream channel packet read");

    if(pkt){
        switch( pkt->hdr.type ){
              case 'c':{
                  uint64_t n_pos=0;
                  uint32_t packets_to_play=0;
                  char buf_cmd[4096];
                  size_t buf_cmd_size = (pkt->hdr.size>(sizeof(buf_cmd) -1 ))?
                                        (sizeof(buf_cmd)-1): pkt->hdr.size;
                  memcpy(buf_cmd,pkt->data,buf_cmd_size);
                  buf_cmd[buf_cmd_size]='\0';
                  log_it(INFO,"SA packet with command '%s'",buf_cmd );

                 if(strcmp("pause",buf_cmd)==0){
                     ch_mcod_set_state(mcod ,CH_MCOD_STATE_PAUSED);
                 }else if(strcmp("play",buf_cmd)==0){
                     ch_mcod_set_state(mcod ,CH_MCOD_STATE_PLAYING);
                 }else if(sscanf(buf_cmd, "status pos=%llu",& mcod->client_position )>=1){
                    // int64_t position=media_last_position(mm); //Unused Variable
                    int64_t position_last_pulled = media_position_last_pulled(mm);
                    log_it(DEBUG,"client status update: base_sync->last_pulled = %'.3lf , mcod->client_position = %'.3lf"
                             " "
                             ,
                              MEDIA_NS_TO_S(position_last_pulled),
                              MEDIA_NS_TO_S(mcod->client_position)
                                //ch_mcod_state_str(mcod->state)
                            );
                    //ch_mcod_check_and_step(mcod);
                    media_check_and_play(mcod->mm);
                    stream_ch_set_ready_to_write(ch,true);
                 }else if( sscanf(buf_cmd,"seek %llu",&n_pos )==1){
                     if(n_pos>0){
                          //media_inactivate(mcod->mm);
                          mcod->client_position=n_pos;
                          media_rewind(mm,n_pos);
                          stream_ch_set_ready_to_write(ch,true);
                      }
                  }else{
                      log_it(WARNING, "Unknown mcod command %s", (char*) pkt->data);
                  }
              }break;
              default:{
                  log_it(WARNING, "Unknown income packet id %c type %c size %u",(char) pkt->hdr.id,(char) pkt->hdr.type, pkt->hdr.size);
              }
        }
    }else
        log_it(ERROR, "Packet read error, arg is NULL");
}

void ch_mcod_check_and_step(ch_mcod_t * mcod)
{
//    if( media_last_position(mcod->mm)<(mcod->client_position+mcod->client_buf_time) ){

    return;

    if(media_is_active(mcod->mm)){
        if(!media_out_step(mcod->mm)){
            media_inactivate(mcod->mm);
    	    stream_ch_pkt_write_f(mcod->ch, 'i',"mcod_media_active=false");
	}
    }else if( media_last_position(mcod->mm)<(mcod->client_position+mcod->client_buf_time) ){
        stream_ch_pkt_write_f(mcod->ch, 'i',"mcod_media_active=true");
        media_activate(mcod->mm);
        if(!media_out_step(mcod->mm)){
            media_inactivate(mcod->mm);
            stream_ch_pkt_write_f(mcod->ch, 'i',"mcod_media_active=false");
	}
    }
}

/**
 * @brief ch_mcod_packet_out
 * @param ch
 * @param arg
 */
void ch_mcod_packet_out(stream_ch_t * ch, void * arg)
{
    (void) arg;
    char buf_meta[256]; // output
    const size_t buf_in_size_max=1024;
    char buf_in[buf_in_size_max]; // Input buffer
    size_t buf_in_size=0;
    int ret;
    ch_mcod_t * mcod=CH_MCOD(ch);
    media_t * mm=mcod->mm;

   // log_it(DEBUG, "ch_mcod_packet_out: !! begins  with state '%s'",ch_mcod_state_str(mcod->state));
    switch(mcod->state){
        case CH_MCOD_STATE_START:{
            log_it(INFO,"START state");
            stream_ch_pkt_write_f(ch, 'i',"gst_state=start");
            ch_mcod_set_state(mcod,CH_MCOD_STATE_PLAYING);

        }break;
        case CH_MCOD_STATE_PLAYING:{
            if(! mcod->is_duration_sent ){
                int64_t duration=media_duration(mm);
                if(duration>0){
                    log_it (DEBUG,"duration = %llf seconds", ((long double) duration)/ 1000000000.0);
                    stream_ch_pkt_write_f(ch,'i',"duration=%lld",duration);
                    mcod->is_duration_sent=true;
                }
            }
            //stream_ch_pkt_write_f(ch, 'i',"mcod_state=playing");
            media_out_pull(mm);
            media_check_and_play(mm);

            //media_out_step(mm);
            //
            //stream_ch_set_ready_to_write(ch,true);
        }break;
        case CH_MCOD_STATE_PAUSED:{
            log_it(NOTICE,"PAUSED state: nothing to stream");
            stream_ch_pkt_write_f(ch, 'i',"mcod_state=paused");
            stream_ch_set_ready_to_write(ch,false);
        }break;
        case CH_MCOD_STATE_END:{
            log_it(NOTICE,"GStreamer pipeline streaming ends");
            stream_ch_pkt_write_f(ch,'i',"mcod_state=end");
            media_close(mm);
        }break;
        default:{
            log_it(ERROR, "What the hell we faced as gstreamer's channel state with the number %d?!",mcod->state);
        }
    }
   // log_it(DEBUG, "stream_gst_packet_out: !! end with state '%s' and ch->ready_to_write='%s'",stream_gst_state_str(sg->state)
   //        , sg->ch->ready_to_write?"true":"false" );
}


/**
 * @brief ch_mcod_set_state
 * @param mcod
 * @param st
 */
void ch_mcod_set_state(ch_mcod_t * mcod, ch_mcod_state_t st)
{
    if(mcod->state!=st){
        mcod->state=st;
        switch(st){
            case CH_MCOD_STATE_START:{
                stream_ch_pkt_write_f(mcod->ch, 'i',"mcod_state=start");
                stream_ch_set_ready_to_write(mcod->ch,true);
            }break;
            case CH_MCOD_STATE_PLAYING:{
                log_it(NOTICE,"PLAYING state");
                stream_ch_pkt_write_f(mcod->ch, 'i',"mcod_state=playing");
                //stream_ch_set_ready_to_write(mcod->ch,true);
                //media_activate
                media_activate(mcod->mm);
                //media_outs_push(mcod->mm);
            }break;
            case CH_MCOD_STATE_PAUSED:{
                log_it(NOTICE,"PAUSED state");

                //stream_ch_set_ready_to_write(mcod->ch,false);
                media_inactivate(mcod->mm);
                int ret=0;
                //while(ret==0)
                //    ret=media_out_pull(mcod->mm);
                stream_ch_pkt_write_f(mcod->ch, 'i',"mcod_state=paused");
            }break;
            case CH_MCOD_STATE_END:{
                mcod->ch->stream->conn->signal_close=true;
            }break;
            default:{
                log_it(ERROR,"Unknown mcod state %d",st);
            }
        }
    }
}
