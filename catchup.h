#ifndef _CATCHUP_H
#define _CATCHUP_H

#include "rtmpdump/librtmp/rtmp.h"

/* The supported catch-up services */
#define CATCHUP_IPLAYER  1
#define CATCHUP_ITV      2
#define CATCHUP_4OD      3
#define CATCHUP_DEMAND5  4

struct catchup_t
{
  /* Basic identifying info */
  char* id;
  int provider;

  /* Episode metadata */
  char* completeTitle;    /* The complete title (iPlayer provides this) */
  char* brandTitle;       /* 4oD - "The Inbetweeners" */
  char* episodeTitle;     /* 4oD - "Will's Birthday" */
  char* seriesTitle;      /* 4oD - N/A,  iPlayer - "Series 9" */
  char* programmeNumber;  /* 4oD - "3" */

  /* RTMP parameters */
  int port;
  int protocol;
  char* playpath;
  char* host;
  char* swfVfy;
  char* tcUrl;
  char* app;
  char* pageUrl;
  char* flashVer;
  char* conn;
};


int catchup_get_info(int provider, char* id, struct catchup_t* cu);
int catchup_download_file(char* filename, struct catchup_t* cu);

#endif
