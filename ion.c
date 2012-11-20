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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "catchup_internal.h"

/*
   Date is ISO format - e.g. 2012-10-20
   Channel is, e.g. bbc_one_london
   Partner is:
     s0000001 - BBC
     s0000003 - ITV
     s0000004 - Channel 4
     s0000005 - Five
     s0000011 - S4C
     s0000013 - Commmunity Channel
*/

#define ION_PARTNER_BBC                1
#define ION_PARTNER_ITV                3
#define ION_PARTNER_CHANNEL4           4
#define ION_PARTNER_CHANNEL5           5
#define ION_PARTNER_S4C               11
#define ION_PARTNER_COMMUNITY_CHANNEL 13


struct ion_channels_t
{
  char* channel_id;
  int partner_id;
};

static struct ion_channels_t ion_channels[] = 
{ 
  { "bbc_one_cambridge",         ION_PARTNER_BBC },
  { "bbc_one_channel_islands",   ION_PARTNER_BBC },
  { "bbc_one_east",              ION_PARTNER_BBC },
  { "bbc_one_east_midlands",     ION_PARTNER_BBC },
  { "bbc_one_east_yorkshire",    ION_PARTNER_BBC },
  { "bbc_one_london",            ION_PARTNER_BBC },
  { "bbc_one_north_east",        ION_PARTNER_BBC },
  { "bbc_one_north_west",        ION_PARTNER_BBC },
  { "bbc_one_northern_ireland",  ION_PARTNER_BBC },
  { "bbc_one_oxford",            ION_PARTNER_BBC },
  { "bbc_one_scotland",          ION_PARTNER_BBC },
  { "bbc_one_south",             ION_PARTNER_BBC },
  { "bbc_one_south_east",        ION_PARTNER_BBC },
  { "bbc_one_south_west",        ION_PARTNER_BBC },
  { "bbc_one_wales",             ION_PARTNER_BBC },
  { "bbc_one_west",              ION_PARTNER_BBC },
  { "bbc_one_west_midlands",     ION_PARTNER_BBC },
  { "bbc_one_yorks",             ION_PARTNER_BBC },
  { "bbc_two_england",           ION_PARTNER_BBC },
  { "bbc_two_northern_ireland",  ION_PARTNER_BBC },
  { "bbc_two_scotland",          ION_PARTNER_BBC },
  { "bbc_two_wales",             ION_PARTNER_BBC },
  { "bbc_three",                 ION_PARTNER_BBC },
  { "bbc_four",                  ION_PARTNER_BBC },
  { "bbc_news24",                ION_PARTNER_BBC },
  { "bbc_hd",                    ION_PARTNER_BBC },
  { "cbeebies",                  ION_PARTNER_BBC },
  { "cbbc",                      ION_PARTNER_BBC },
  { "bbc_parliament",            ION_PARTNER_BBC },
  { "bbc_alba",                  ION_PARTNER_BBC },

  { "itv1",                      ION_PARTNER_ITV },
  { "itv2",                      ION_PARTNER_ITV },
  { "itv3",                      ION_PARTNER_ITV },
  { "itv4",                      ION_PARTNER_ITV },

  { "channel4",                  ION_PARTNER_CHANNEL4 },
  { "e4",                        ION_PARTNER_CHANNEL4 },
  { "more4",                     ION_PARTNER_CHANNEL4 },
  { "film4",                     ION_PARTNER_CHANNEL4 },
  { "4music",                    ION_PARTNER_CHANNEL4 },

  { "channel_five",              ION_PARTNER_CHANNEL5 },
  { "fiver",                     ION_PARTNER_CHANNEL5 },
  { "five_usa",                  ION_PARTNER_CHANNEL5 },

  { "s4c",                       ION_PARTNER_S4C },

  { "community_channel",         ION_PARTNER_COMMUNITY_CHANNEL },
  { NULL, 0}
};

int get_channel_id(char* channel)
{
  int i = 0;

  while (ion_channels[i].channel_id) {
    if (strcmp(ion_channels[i].channel_id, channel)==0)
      return i;

    i++;
  }

  return -1;
}

#if 0
xmlNodePtr node = NULL;
xmlNsPtr saml;
xmlNsPtr samlp;
char prefixeduuid[64];
char refuuid[64];
char timestamp[64];
uuid_t uu;
time_t now;
struct tm* tm;

/* TODO: Check uniqueness in output directory */
uuid_generate(uu);
uuid_unparse(uu, uuid);
prefixeduuid[0]='_';
uuid_unparse(uu, prefixeduuid+1);
refuuid[0]='#';
refuuid[1]='_';
uuid_unparse(uu, refuuid+2);

/* Create timestamp */
now = time(NULL);
tm = gmtime(&now);
sprintf(timestamp,"%04d-%02d-%02dT%02d:%02d:%02dZ",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);

/* Create and set namespaces */
samlp = xmlNewNs(root_node,"urn:oasis:names:tc:SAML:2.0:protocol", "samlp");
saml = xmlNewNs(root_node,"urn:oasis:names:tc:SAML:2.0:assertion", "saml");

xmlSetNs(root_node, saml);
xmlSetNs(root_node, samlp);

/* Add attributes to root node */
xmlNewProp(root_node, BAD_CAST "ID", BAD_CAST prefixeduuid);
#endif

xmlChar* xmlNodeGetContentReEscape(xmlNode* p)
{
  char* s = (char*)xmlNodeGetContent(p);

  /* Count ampersands */
  int i=0;
  int len = strlen(s);
  int ampcount = 0;
  for (i=0;i<len;i++) {
    if (s[i]=='&') ampcount++;
  }

  if (ampcount==0)
    return (xmlChar*)s;

  char* ss = malloc(len + ampcount*4 + 1);
  int j = 0;
  for (i=0;i<=len;i++) {
    if (s[i] == '&') {
      ss[j++] = '&';
      ss[j++] = 'a';
      ss[j++] = 'm';
      ss[j++] = 'p';
      ss[j++] = ';';
    } else {
      ss[j++] = s[i];
    }
  }

  xmlFree(s);
  return (xmlChar*)ss;
}

xmlDocPtr ion_get(int channel_id, char* date)
{
  struct MemoryStruct chunk;
  char url[128];
  xmlDocPtr pyepg_doc = NULL;       /* document pointer */
  xmlNodePtr pyepg_root = NULL;

  fprintf(stderr,"Fetching ION schedule - partner=%d, channel=%s, date=%s\n",ion_channels[channel_id].partner_id,ion_channels[channel_id].channel_id,date);

  xmlInitParser();

  if (ion_channels[channel_id].partner_id == ION_PARTNER_BBC) {
    snprintf(url,sizeof(url),"http://www.bbc.co.uk/iplayer/ion/schedule/date/%s/service/%s/format/xml",date,ion_channels[channel_id].channel_id);
  } else {
    snprintf(url,sizeof(url),"http://www.bbc.co.uk/iplayer/ion/partner/schedule/partner/s%07d/date/%s/service/%s/format/xml",ion_channels[channel_id].partner_id,date,ion_channels[channel_id].channel_id);
  }

  fprintf(stderr,"Fetching %s...\n",url);
  xml_fetch(url,&chunk);
  printf("%lu bytes retrieved\n", (long)chunk.size);

  //fprintf(stderr,"%s",chunk.memory);

  xmlDocPtr doc = xmlParseMemory(chunk.memory,chunk.size);
  if (doc == NULL) {
    fprintf(stderr,"XML Parse failure\n");
    return NULL;
  }

  xmlNode* root = xmlDocGetRootElement(doc);

  /* Create output document and root node */
  pyepg_doc = xmlNewDoc(BAD_CAST "1.0");
  pyepg_root = xmlNewNode(NULL, BAD_CAST "epg");
  xmlDocSetRootElement(pyepg_doc, pyepg_root);
  xmlNode* channel_node = xmlNewChild(pyepg_root, NULL, BAD_CAST "channel", NULL);
  xmlNewProp(channel_node, BAD_CAST "id", BAD_CAST ion_channels[channel_id].channel_id);
  xmlNode* brands_node = xmlNewChild(pyepg_root, NULL, BAD_CAST "brands", NULL);
  xmlNode* seasons_node = xmlNewChild(pyepg_root, NULL, BAD_CAST "seasons", NULL);
  xmlNode* episodes_node = xmlNewChild(pyepg_root, NULL, BAD_CAST "episodes", NULL);
  xmlNode* ondemands_node = xmlNewChild(pyepg_root, NULL, BAD_CAST "ondemands", NULL);
  xmlNode* schedule_node = xmlNewChild(pyepg_root, NULL, BAD_CAST "schedule", NULL);
  xmlNewProp(schedule_node, BAD_CAST "channel", BAD_CAST ion_channels[channel_id].channel_id);

  xmlNode* p = root->children->next->children->next;
  while (p) {
    if (strcmp((char*)p->name,"broadcast")==0) {
      char* brand_id = NULL;
      char* brand_title = NULL;
      char* availability = NULL;
      char* available_until = NULL;
      char* start_time_iso = NULL;
      char* end_time_iso = NULL;
      char* complete_title = NULL;
      char* episode_id = NULL;
      char* broadcast_id = NULL;
      char* title = NULL;
      char* synopsis = NULL;
      char* series_id = NULL;
      char* series_title = NULL;
      char* play_version_id = NULL;
      char* short_synopsis = NULL;
      char* myurl = NULL;
      char* my_image_base_url = NULL;
      char* my_image_template_url = NULL;
      char* episode_image = NULL;
      char* version_id = NULL;

      xmlNode* q = p->children;
      while (q) {
        if (strcmp((char*)q->name,"episode")==0) {
          xmlNode* r = q->children;
          while (r) {
            if (strcmp((char*)r->name,"brand_id")==0)  brand_id = (char*)xmlNodeGetContent(r);
            else if (strcmp((char*)r->name,"brand_title")==0)  brand_title = (char*)xmlNodeGetContentReEscape(r);
            else if (strcmp((char*)r->name,"availability")==0)  availability = (char*)xmlNodeGetContent(r);
            else if (strcmp((char*)r->name,"available_until")==0)  available_until = (char*)xmlNodeGetContent(r);
            else if (strcmp((char*)r->name,"id")==0)  episode_id = (char*)xmlNodeGetContent(r);
            else if (strcmp((char*)r->name,"title")==0)  title = (char*)xmlNodeGetContentReEscape(r);
            else if (strcmp((char*)r->name,"synopsis")==0)  synopsis = (char*)xmlNodeGetContentReEscape(r);
            else if (strcmp((char*)r->name,"series_id")==0)  series_id = (char*)xmlNodeGetContent(r);
            else if (strcmp((char*)r->name,"series_title")==0)  series_title = (char*)xmlNodeGetContentReEscape(r);
            else if (strcmp((char*)r->name,"play_version_id")==0)  play_version_id = (char*)xmlNodeGetContent(r);
            else if (strcmp((char*)r->name,"short_synopsis")==0)  short_synopsis = (char*)xmlNodeGetContent(r);
            else if (strcmp((char*)r->name,"my_image_base_url")==0)  my_image_base_url = (char*)xmlNodeGetContent(r);
            else if (strcmp((char*)r->name,"my_image_template_url")==0)  my_image_template_url = (char*)xmlNodeGetContent(r);
            r = r->next;
          }
        }
        else if (strcmp((char*)q->name,"start_time_iso")==0)  start_time_iso = (char*)xmlNodeGetContent(q);
        else if (strcmp((char*)q->name,"end_time_iso")==0)  end_time_iso = (char*)xmlNodeGetContent(q);
        else if (strcmp((char*)q->name,"id")==0)  broadcast_id = (char*)xmlNodeGetContent(q);
        else if (strcmp((char*)q->name,"complete_title")==0)  complete_title = (char*)xmlNodeGetContentReEscape(q);
        else if (strcmp((char*)q->name,"version_id")==0)  version_id = (char*)xmlNodeGetContent(q);
        else if (strcmp((char*)q->name,"myurl")==0)  myurl = (char*)xmlNodeGetContent(q);

        q = q->next;
      }

      printf("**** Broadcast\n");
      printf("broadcast_id=%s\n",broadcast_id);
      printf("start_time_iso=%s\n",start_time_iso);
      printf("end_time_iso=%s\n",end_time_iso);
      printf("complete_title=%s\n",complete_title);

      printf("brand_id=%s\n",brand_id);
      printf("brand_title=%s\n",brand_title);
      printf("series_id=%s\n",series_id);
      printf("series_title=%s\n",series_title);
      printf("episode_id=%s\n",episode_id);
      printf("title=%s\n",title);

      printf("short_synopsis=%s\n",short_synopsis);
      printf("synopsis=%s\n",synopsis);

      printf("availability=%s\n",availability);
      printf("available_until=%s\n",available_until);
      printf("play_version_id=%s\n",play_version_id);
      printf("version_id=%s\n",version_id);
      printf("myurl=%s\n",myurl);
      printf("my_image_base_url=%s\n",my_image_base_url);
      printf("my_image_template_url=%s\n",my_image_template_url);

      if (ion_channels[channel_id].partner_id == ION_PARTNER_BBC) {
        episode_image=malloc(strlen(my_image_base_url)+strlen(episode_id)+13);
        sprintf(episode_image,"%s%s_832_468.jpg",my_image_base_url,episode_id);
      }
      printf("episode_image=%s\n",episode_image);
      /* TODO: Free all strings */

      /* Add broadcast node */
      xmlNode* broadcast_node = xmlNewChild(schedule_node, NULL, BAD_CAST "broadcast", NULL);
      xmlNewProp(broadcast_node, BAD_CAST "id", BAD_CAST broadcast_id);
      xmlNewProp(broadcast_node, BAD_CAST "start", BAD_CAST start_time_iso);
      xmlNewProp(broadcast_node, BAD_CAST "end", BAD_CAST end_time_iso);
      xmlNewProp(broadcast_node, BAD_CAST "episode", BAD_CAST episode_id);

      /* Add episode node */
      xmlNode* episode_node = xmlNewChild(episodes_node, NULL, BAD_CAST "episode", NULL);
      xmlNewProp(episode_node, BAD_CAST "id", BAD_CAST episode_id);
      xmlNewChild(episode_node, NULL, BAD_CAST "complete_title", BAD_CAST complete_title);
      xmlNewChild(episode_node, NULL, BAD_CAST "episode_title", BAD_CAST title);
      xmlNewChild(episode_node, NULL, BAD_CAST "summary", BAD_CAST short_synopsis);
      xmlNewChild(episode_node, NULL, BAD_CAST "description", BAD_CAST synopsis);

      /* Add ondemand node */
      if ((availability != NULL) && (availability[0] != 0) && (strcmp(availability,"NEVER")!=0)) {
        xmlNode* ondemand_node = xmlNewChild(ondemands_node, NULL, BAD_CAST "ondemand", NULL);
        xmlNewProp(ondemand_node, BAD_CAST "id", BAD_CAST version_id);
        xmlNewProp(ondemand_node, BAD_CAST "episode", BAD_CAST episode_id);
        xmlNewChild(ondemand_node, NULL, BAD_CAST "availability", BAD_CAST availability);
        xmlNewChild(ondemand_node, NULL, BAD_CAST "available_until", BAD_CAST available_until);
        switch (ion_channels[channel_id].partner_id)
	{
          case ION_PARTNER_BBC:
            xmlNewChild(ondemand_node, NULL, BAD_CAST "provider", BAD_CAST "bbc.co.uk");
            xmlNewChild(ondemand_node, NULL, BAD_CAST "media_id", BAD_CAST version_id);
            break;

          case ION_PARTNER_ITV:
            xmlNewChild(ondemand_node, NULL, BAD_CAST "provider", BAD_CAST "itv.com");
            xmlNewChild(ondemand_node, NULL, BAD_CAST "media_id", BAD_CAST myurl+43);
            break;

          case ION_PARTNER_CHANNEL4:
            xmlNewChild(ondemand_node, NULL, BAD_CAST "provider", BAD_CAST "channel4.com");
            xmlNewChild(ondemand_node, NULL, BAD_CAST "media_id", BAD_CAST myurl+strlen(myurl)-7);
            break;

          case ION_PARTNER_CHANNEL5:
            xmlNewChild(ondemand_node, NULL, BAD_CAST "provider", BAD_CAST "channel5.com");
            xmlNewChild(ondemand_node, NULL, BAD_CAST "media_id", BAD_CAST myurl+25);
            break;
        }
      }

      /* Add season node, if applicable */
      if ((series_id != NULL) && (series_id[0] != 0)) {
        xmlNode* season_node = xmlNewChild(seasons_node, NULL, BAD_CAST "season", NULL);
        xmlNewProp(season_node, BAD_CAST "id", BAD_CAST series_id);
        xmlNewProp(episode_node, BAD_CAST "series", BAD_CAST series_id);
        if ((series_title != NULL) && (series_title[0] != 0)) {
          xmlNewChild(season_node, NULL, BAD_CAST "title", BAD_CAST series_title);
        }
      }

      /* Add brand node, if applicable */
      if ((brand_id != NULL) && (brand_id[0] != 0)) {
        xmlNode* brand_node = xmlNewChild(brands_node, NULL, BAD_CAST "brand", NULL);
        xmlNewProp(brand_node, BAD_CAST "id", BAD_CAST brand_id);
        xmlNewProp(episode_node, BAD_CAST "brand", BAD_CAST brand_id);
        if ((brand_title != NULL) && (brand_title[0] != 0)) {
          xmlNewChild(brand_node, NULL, BAD_CAST "title", BAD_CAST brand_title);
        }
      }
    }
    p = p->next;
  }

  return pyepg_doc;;
}


int main(int argc, char* argv[])
{
  int channel_id;
  int i;
  xmlDocPtr pyepg_doc;

  if (argc != 3) {
    fprintf(stderr,"Usage: ion channel date\n");
    fprintf(stderr,"\nAvailable channels:\n\n");
    for(i=0;ion_channels[i].channel_id;i++) {
      fprintf(stderr,"  %s\n",ion_channels[i].channel_id);
    }
    return 1;
  }

  channel_id = get_channel_id(argv[1]);
  if (channel_id < 0) {
    fprintf(stderr,"Unknown channel %s\n",argv[1]);
    return 2;
  }

  pyepg_doc = ion_get(channel_id,argv[2]);

  if (pyepg_doc != NULL) {

    xmlSaveFormatFileEnc("test.xml", pyepg_doc, "UTF-8", 1);

    /* Free pyepg_document */
    if(pyepg_doc != NULL) {
      xmlFreeDoc(pyepg_doc); 
    }
  }

  return 0;
}
