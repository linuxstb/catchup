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

static char* soap_template = 
"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
"  <SOAP-ENV:Body>\n"
"    <tem:GetPlaylist xmlns:tem=\"http://tempuri.org/\" xmlns:itv=\"http://schemas.datacontract.org/2004/07/Itv.BB.Mercury.Common.Types\" xmlns:com=\"http://schemas.itv.com/2009/05/Common\">\n"
"    <tem:request>\n"
"      <itv:RequestGuid>FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF</itv:RequestGuid>\n"
"      <itv:Vodcrid>\n"
"        <com:Id>%s</com:Id>\n"
"        <com:Partition>itv.com</com:Partition>\n"
"      </itv:Vodcrid>\n"
"    </tem:request>\n"
"      <tem:userInfo>\n"
"        <itv:GeoLocationToken>\n"
"          <itv:Token/>\n"
"        </itv:GeoLocationToken>\n"
"        <itv:RevenueScienceValue>scc=true; svisit=1; sc4=Other</itv:RevenueScienceValue>\n"
"      </tem:userInfo>\n"
"      <tem:siteInfo>\n"
"        <itv:Area>ITVPLAYER.VIDEO</itv:Area>\n"
"        <itv:Platform>DotCom</itv:Platform>\n"
"        <itv:Site>ItvCom</itv:Site>\n"
"      </tem:siteInfo>\n"
"    </tem:GetPlaylist>\n"
"  </SOAP-ENV:Body>\n"
"</SOAP-ENV:Envelope>\n";

int catchup_get_info_itv(struct catchup_t* cu)
{
  struct MemoryStruct postdata;
  struct MemoryStruct result;
  struct curl_slist *slist=NULL; 
  int res;

  /* Start of code... */

  postdata.memory = malloc(strlen(soap_template)+10);
  sprintf(postdata.memory,soap_template,cu->id);
  postdata.size = strlen(postdata.memory);

  slist = curl_slist_append(slist,  "Referer: http://www.itv.com/mercury/Mercury_VideoPlayer.swf?v=1.6.479/[[DYNAMIC]]/2");
  slist = curl_slist_append(slist,  "Content-type: text/xml; charset=\"UTF-8\"");
  slist = curl_slist_append(slist,  "SOAPAction: http://tempuri.org/PlaylistService/GetPlaylist");

  res = do_post("http://mercury.itv.com/PlaylistService.svc",
                slist,&postdata,&result);

  curl_slist_free_all(slist); /* free the list again */ 

  if (res > 0) {
    fprintf(stderr,"Error sending SOAP request\n");
    return 1;
  }

  xmlInitParser();

  printf("%lu bytes retrieved\n", (long)result.size);

  char filename[20];
  sprintf(filename,"%s.xml",cu->id);
  int fd = open(filename,O_CREAT|O_TRUNC|O_RDWR,0666);
  if (fd >= 0) {
    write(fd,result.memory,result.size);
    close(fd);
  }

  xmlDocPtr doc = xmlParseMemory(result.memory,result.size);
  if (doc == NULL) fprintf(stderr,"XML Parse failure\n");

  xmlNode* root = xmlDocGetRootElement(doc);
  char* base = NULL;
  char mp4[1024];

  xmlNode* p = root->children->children->children->children->children;
  while (p) {
    if (strcmp((char*)p->name,"ProgrammeTitle")==0) {
      cu->brandTitle = (char*)xmlNodeGetContent(p);
    } else if (strcmp((char*)p->name,"EpisodeTitle")==0) {
      cu->episodeTitle = (char*)xmlNodeGetContent(p);
    } else if (strcmp((char*)p->name,"EpisodeNumber")==0) {
      cu->programmeNumber = (char*)xmlNodeGetContent(p);
    } else if (strcmp((char*)p->name,"VideoEntries")==0) {
      xmlNode* q;
      int max_bitrate = 0;

      q = p->children;
      while (q && (strcmp((char*)q->name,"Video"))) q=q->next;

      if (q==NULL)
        return 1;  /* No Video element */

      q = q->children;
      while (q && (strcmp((char*)q->name,"MediaFiles"))) q=q->next;

      base = (char*)xmlGetProp(q,(xmlChar*)"base");
      if (base==NULL)
        return 1;

      q = q->children;
      while (q) {
        if (strcmp((char*)q->name,"MediaFile")==0) {
	  int bitrate = atoi((char*)xmlGetProp(q,(xmlChar*)"bitrate"));

          if (bitrate > max_bitrate) {
            xmlNode* r = q->children;
            while (r && (strcmp((char*)r->name,"URL"))) r=r->next;
            if (r) {
              max_bitrate = bitrate;
              char* s = (char*)xmlNodeGetContent(r);
              strcpy(mp4,s);
              free(s);
            }
            
          }
        }
        q = q->next;
      }
    }
    p = p -> next;
  }

  cu->playpath = strdup(mp4);
  cu->protocol = RTMP_PROTOCOL_RTMPE; 
  cu->port = 1935;
  cu->swfVfy = strdup("http://www.itv.com/mercury/Mercury_VideoPlayer.swf");
  cu->tcUrl = strdup(base);

  char* s = index(base,'/') + 2;
  char* t = index(s,':');
  if (t==NULL) {
    t = index(s,'/');
  }
  *t = 0;
  cu->host = strdup(s);

  return 0;
}

