/*
Copyright (C) 2012 Dave Chapman <dave@dchapman.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "catchup.h"

static void id_usage()
{
    fprintf(stderr,"\nThe following episode IDs are supported:\n\n");
    fprintf(stderr,"  BBC iPlayer - 8 character ID (visible in URL)\n");
    fprintf(stderr,"  ITV Player  - 6 digit ID (visible in URL)\n");
    fprintf(stderr,"  4oD         - 7 digit ID (visible in URL)\n");
    fprintf(stderr,"  demand5     - 11 character ID (search for ref: in page source)\n");
    fprintf(stderr,"\n");
}

int main(int argc, char* argv[])
{
  int provider;
  struct catchup_t cu;
  int res;

  if (argc != 2) {
    fprintf(stderr,"USAGE: catchup episodeid\n");
    id_usage();
    exit(1);
  }

  if (strlen(argv[1]) == 8) {
    fprintf(stderr,"Catchup mode - iPlayer\n");
    provider = CATCHUP_IPLAYER;
  } else if (strlen(argv[1]) == 6) {
    fprintf(stderr,"Catchup mode - ITV\n");
    provider = CATCHUP_ITV;
  } else if (strlen(argv[1]) == 7) {
    fprintf(stderr,"Catchup mode - 4oD\n");
    provider = CATCHUP_4OD;
  } else if (strlen(argv[1]) == 11) {
    fprintf(stderr,"Catchup mode - demand5\n");
    provider = CATCHUP_DEMAND5;
  } else {
    fprintf(stderr,"Unrecognised episode ID format.\n");
    id_usage();
    exit(1);
  }

  res = catchup_get_info(provider,argv[1],&cu);

  if (res) {
    fprintf(stderr,"Fatal error.\n");
    return 1;
  }

  fprintf(stderr,"\n");
  fprintf(stderr,"completeTitle:   %s\n",cu.completeTitle);
  fprintf(stderr,"brandTitle:      %s\n",cu.brandTitle);
  fprintf(stderr,"seriesTitle:     %s\n",cu.seriesTitle);
  fprintf(stderr,"programmeNumber: %s\n",cu.programmeNumber);
  fprintf(stderr,"episodeTitle:    %s\n",cu.episodeTitle);
  fprintf(stderr,"\n");

  char* outfile;

  if ((cu.brandTitle && cu.brandTitle[0]!=0) && (cu.seriesTitle) && (cu.programmeNumber) && (cu.episodeTitle)) {
    outfile = malloc(strlen(cu.brandTitle)+3+strlen(cu.seriesTitle)+3+5+3+strlen(cu.episodeTitle)+3+strlen(cu.id)+5);
    sprintf(outfile,"%s - %s - %02d - %s - %s.flv",cu.brandTitle,cu.seriesTitle,atoi(cu.programmeNumber),cu.episodeTitle,cu.id);
  } else if ((cu.brandTitle==NULL || cu.brandTitle[0]==0) && (cu.seriesTitle) && (cu.programmeNumber) && (cu.episodeTitle)) {
    outfile = malloc(strlen(cu.seriesTitle)+3+5+3+strlen(cu.episodeTitle)+3+strlen(cu.id)+5);
    sprintf(outfile,"%s - %02d - %s - %s.flv",cu.seriesTitle,atoi(cu.programmeNumber),cu.episodeTitle,cu.id);
  } else if ((cu.brandTitle) && (cu.programmeNumber) && (cu.episodeTitle)) {
    outfile = malloc(strlen(cu.brandTitle)+3+5+3+strlen(cu.episodeTitle)+3+strlen(cu.id)+5);
    sprintf(outfile,"%s - %02d - %s - %s.flv",cu.brandTitle,atoi(cu.programmeNumber),cu.episodeTitle,cu.id);
  } else {
    /* TODO: Build output filename based on metadata retrieved in cu */
    outfile = malloc(32);
    sprintf(outfile,"%s.flv",cu.id);
  }

  /* Sanitise filename */
  char* s = outfile;
  while (*s != 0) {
    if ((*s==':') || (*s=='<') || (*s=='>') || (*s=='/'))
      *s = '_';

    s++;
  }

#if 1
  /* Display the rtmpdump command-line to download this file */
  fprintf(stderr,"rtmpdump \\\n");
  switch(cu.protocol) {
    case RTMP_PROTOCOL_RTMP: fprintf(stderr,"--protocol rtmp \\\n"); break;
    case RTMP_PROTOCOL_RTMPE: fprintf(stderr,"--protocol rtmpe \\\n"); break;
  }
  fprintf(stderr,"--port     %d \\\n",cu.port);
  if (cu.playpath) fprintf(stderr,"--playpath \"%s\" \\\n",cu.playpath);
  if (cu.host)     fprintf(stderr,"--host     \"%s\" \\\n",cu.host);
  if (cu.swfVfy)   fprintf(stderr,"--swfVfy   \"%s\" \\\n",cu.swfVfy);
  if (cu.tcUrl)    fprintf(stderr,"--tcUrl    \"%s\" \\\n",cu.tcUrl);
  if (cu.app)      fprintf(stderr,"--app      \"%s\" \\\n",cu.app);
  if (cu.pageUrl)  fprintf(stderr,"--pageUrl  \"%s\" \\\n",cu.pageUrl);
  if (cu.flashVer) fprintf(stderr,"--flashVer \"%s\" \\\n",cu.flashVer);
  if (cu.conn)     fprintf(stderr,"--conn     \"%s\" \\\n",cu.conn);
  fprintf(stderr,"--outfile  \"%s\"\n",outfile);
#endif

  fprintf(stderr,"Downloading \"%s\"\n",outfile);

  res = catchup_download_file(outfile,&cu);

  fprintf(stderr,"\n\nDownloading completed.\n");

  return 0;
}
