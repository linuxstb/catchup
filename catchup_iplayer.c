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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <curl/curl.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlschemas.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "rtmpdump/librtmp/rtmp.h"
#include "catchup.h"
#include "catchup_internal.h"
#include "rtmpdump.h"

int catchup_get_info_iplayer(struct catchup_t* cu)
{
  struct MemoryStruct chunk;
  struct MemoryStruct mediaselector_buf;
  char url[128];

  /* Start of code... */

  xmlInitParser();

  snprintf(url,sizeof(url),"http://www.bbc.co.uk/iplayer/ion/episodedetail/episode/%s/format/xml",cu->id);

  fprintf(stderr,"Fetching %s...\n",url);

  xml_fetch(url,&chunk);

  printf("%lu bytes retrieved\n", (long)chunk.size);

  char filename[20];
  sprintf(filename,"%s.xml",cu->id);
  int fd = open(filename,O_CREAT|O_TRUNC|O_RDWR,0666);
  if (fd >= 0) {
    write(fd,chunk.memory,chunk.size);
    close(fd);
  }

  //fprintf(stderr,"%s",chunk.memory);

  xmlDocPtr doc = xmlParseMemory(chunk.memory,chunk.size);
  if (doc == NULL) fprintf(stderr,"XML Parse failure\n");

  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);

  if (xmlXPathRegisterNs(xpathCtx, (xmlChar*)"ion", (xmlChar*)"http://bbc.co.uk/2008/iplayer/ion") < 0) {
    fprintf(stderr,"Unable to register ion namespace\n");
    printf("Status=ERROR\n");
    return -1;
  }

  cu->brandTitle = (char*)myXpathGetValue(xpathCtx, (xmlChar*)"/ion:ion/ion:blocklist/ion:episode_detail/ion:brand_title", NULL);
  cu->episodeTitle = (char*)myXpathGetValue(xpathCtx, (xmlChar*)"/ion:ion/ion:blocklist/ion:episode_detail/ion:hierarchical_title", NULL);
  cu->seriesTitle = (char*)myXpathGetValue(xpathCtx, (xmlChar*)"/ion:ion/ion:blocklist/ion:episode_detail/ion:series_title", NULL);
  cu->programmeNumber = (char*)myXpathGetValue(xpathCtx, (xmlChar*)"/ion:ion/ion:blocklist/ion:episode_detail/ion:position", NULL);

  xmlChar* pageUrl;
  if ((pageUrl = myXpathGetValue(xpathCtx, (xmlChar*)"/ion:ion/ion:blocklist/ion:episode_detail/ion:my_url", NULL)) == NULL) {
    fprintf(stderr,"pageUrl element not found\n");
    return 1;
  }

  xmlChar* complete_title;
  if ((complete_title = myXpathGetValue(xpathCtx, (xmlChar*)"/ion:ion/ion:blocklist/ion:episode_detail/ion:complete_title", NULL)) == NULL) {
    fprintf(stderr,"complete_title element not found\n");
    return 1;
  }
  cu->completeTitle = strdup((char*)complete_title);
  xmlFree(complete_title);

  xmlChar* mediaselector_url;
  if ((mediaselector_url = myXpathGetValue(xpathCtx, (xmlChar*)"/ion:ion/ion:blocklist/ion:episode_detail/ion:my_mediaselector_xml_url", NULL)) == NULL) {
    fprintf(stderr,"my_mediaselector_xml_url element not found\n");
    return 1;
  }

  fprintf(stderr,"Fetching %s...\n",mediaselector_url);
  xml_fetch((char*)mediaselector_url,&mediaselector_buf);
  xmlFree(mediaselector_url);

  printf("%lu bytes retrieved\n", (long)mediaselector_buf.size);
  //fprintf(stderr,"%s",mediaselector_buf.memory);

  xmlDocPtr mediaselector_doc = xmlParseMemory(mediaselector_buf.memory,mediaselector_buf.size);
  if (mediaselector_doc == NULL) fprintf(stderr,"XML Parse failure\n");

  //  /root/node[not(@val <= preceding-sibling::node/@val) and not(@val <=following-sibling::node/@val)]

  xmlNode* root = xmlDocGetRootElement(mediaselector_doc);

  xmlNode* media = NULL;
  int max_bitrate = 0;

  xmlNode* p = root->children;
  while (p != NULL) {
    if (strcmp((char*)p->name,"media")==0) {
      xmlNode* q = p->children;

      if (strcmp((char*)q->name,"connection")==0) {
        xmlChar* protocol = xmlGetProp(q,(xmlChar*)"protocol");
        if ((protocol) && (strcmp((char*)protocol,"rtmp")==0)) {
          xmlChar* bitrate = xmlGetProp(p,(xmlChar*)"bitrate");
          if (bitrate != NULL) {
            int i = atoi((char*)bitrate);
            if (i > max_bitrate) {
              max_bitrate = i;
              media = p;
            }
          }
        }
      }
    }
    p = p->next;
  }

  if (media == NULL) {
    fprintf(stderr,"No rtmp streams found\n");
    return 1;
  }

  fprintf(stderr,"Found rtmp connection with bitrate %d\n",max_bitrate);
  xmlNode* connection = NULL;

  p = media->children;
  xmlChar* application = NULL;
  while (p && connection==NULL) {
    application = xmlGetProp(p,(xmlChar*)"application");
    if (application) {
      if ((strcmp((char*)application,"ondemand")==0) || (strcmp((char*)application,"a1414/e3")==0) || (strcmp((char*)application,"a5999/e1")==0)) {
        connection = p;
      }
    }
    p = p-> next;
  }

  if (connection == NULL) {
    fprintf(stderr,"No ondemand or a1414/e3 connection available\n");
    return 1;
  }

  xmlChar* authString = xmlGetProp(connection,(xmlChar*)"authString");
  if (authString == NULL) {
    fprintf(stderr,"No authString attribute\n");
    return 1;
  }

  xmlChar* identifier = xmlGetProp(connection,(xmlChar*)"identifier");
  if (identifier == NULL) {
    fprintf(stderr,"No identifier attribute\n");
    return 1;
  }

  xmlChar* server = xmlGetProp(connection,(xmlChar*)"server");
  if (server == NULL) {
    fprintf(stderr,"No server attribute\n");
    return 1;
  }

  cu->port = 1935;

  if (strcmp((char*)application,"ondemand")==0) {
    cu->playpath = malloc(strlen((char*)identifier)+1+strlen((char*)authString)+1);
    sprintf(cu->playpath,"%s?%s",identifier,authString);
  } else {
    cu->playpath = (char*)identifier;
  }

  cu->host = strdup((char*)server);
  cu->swfVfy = strdup("http://www.bbc.co.uk/emp/revisions/18269_21576_10player.swf?revision=18269_21576");

  if (strcmp((char*)application,"ondemand")==0) {
    char* tcUrl_fmt = "rtmp://%s:80/ondemand?_fcs_vhost=%s&undefined&%s";
    cu->tcUrl = malloc(strlen(tcUrl_fmt)+strlen((char*)server)*2+strlen((char*)authString));
    sprintf(cu->tcUrl,tcUrl_fmt,server,server,authString);
  } else {
    char* tcUrl_fmt = "rtmp://%s:1935/%s?%s";
    cu->tcUrl = malloc(strlen(tcUrl_fmt)+strlen((char*)server)*2+strlen((char*)authString)+strlen((char*)application));
    sprintf(cu->tcUrl,tcUrl_fmt,server,application,authString);
  }

  if (strcmp((char*)application,"ondemand")==0) {
    char* app_fmt = "ondemand?_fcs_vhost=%s&undefined&%s";
    cu->app = malloc(strlen(app_fmt)+strlen((char*)server)+strlen((char*)authString));
    sprintf(cu->app,app_fmt,server,authString);
  } else {
    char* app_fmt = "%s?%s";
    cu->app = malloc(strlen((char*)application)+1+strlen((char*)authString)+1);
    sprintf(cu->app,app_fmt,application,authString);
  }

  cu->pageUrl = malloc(23+strlen((char*)pageUrl));
  sprintf(cu->pageUrl,"http://www.bbc.co.uk%s",pageUrl);
  //  int timeout = 10; 120 is the default for rtmpdump

  cu->protocol = RTMP_PROTOCOL_RTMP;

  if(chunk.memory)
    free(chunk.memory);

  return 0;
}

