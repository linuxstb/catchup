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
#include "rtmpdump/librtmp/amf.h"
#include "rtmpdump/librtmp/log.h"
#include "catchup.h"
#include "catchup_internal.h"
#include "rtmpdump.h"

static void parse_bootstrap(unsigned char* streamdata, int streamlen)
{
  int istream_pos = 0;
#if 0
  while (read_box) {
    if ($box_data{boxname} eq 'abst') {
        my $boxmsg = substr($box_data{box}, $box_data{boxhdr_len});
        my $ipos = 0;
        $ipos += 1; # version
        $ipos += 3; # Flags
        $ipos += 4; # Bootstrapinfoversion
        $ipos += 1; #Profile+Live+Update+res
        print "Timescale ". unpack("N", substr($boxmsg, $ipos, 4)). "\n";
        $ipos += 4; # Timescale;
        my $mt = unpack("N2", substr($boxmsg, $ipos, 4));
        $mt = unpack("N2", substr($boxmsg, $ipos+4, 4)) + ($mt << 32);
        print "CurrentMediaTime " . $mt . "\n";
        $ipos += 8; # CurrentMediaTime
        $ipos += 8; # TimeCodeOffset
        my $jpos = index($boxmsg, $nulchar, $ipos);
print "$ipos $jpos\n";
        $ipos = $jpos + 1; # Movie ID String.
        my $servercnt = unpack('c', substr($boxmsg, $ipos, 1));
        $ipos += 1;
        for (my $i = 0; $i < $servercnt; $i++) {
            $ipos = index($boxmsg, $nulchar,  $ipos) + 1;
        }
        my $qualitycnt = unpack('c', substr($boxmsg, $ipos, 1));
        $ipos += 1;
        for (my $i = 0; $i < $qualitycnt; $i++) {
            $ipos = index($boxmsg, $nulchar, $ipos) + 1;
        }
        my $k = $ipos;
        $ipos = index($boxmsg, $nulchar, $ipos) + 1; # DrmData
        my $drmdata = substr($boxmsg, $k, $ipos - $k);
print "drm $drmdata\n";
        $ipos = index($boxmsg, $nulchar, $ipos) + 1; # MetaData
        my $segtabcnt = unpack('c', substr($boxmsg, $ipos, 1));
        $ipos += 1;
        for (my $i = 0; $i < $segtabcnt; $i ++) {
            my $npos = $ipos;
            my $segentlen = unpack("N", substr($boxmsg, $npos, 4));
            $npos = $npos+4;
            my $segtype = substr($boxmsg, $npos, 4);
            $npos = $npos+4;
            print "Seg Type $segtype len $segentlen\n";
            my $flags = unpack('N', substr($boxmsg, $npos, 4));
            $npos = $npos+4;
            #  version 0xff000000, flags 0x00ffffff 
            my $qualentcnt = unpack('c', substr($boxmsg, $npos, 1));
            $npos = $npos+1;
            for (my $i = 0; $i < $qualentcnt; $i++) {
                $npos = index($boxmsg, $nulchar, $npos) + 1; 
            }
            my $segrunentcnt = unpack('N', substr($boxmsg, $npos, 4));
print "  Segment Run Count $segrunentcnt\n";
            $npos += 4;
            for (my $i = 0; $i < $segrunentcnt; $i++) {
               my $firstseg = unpack("N", substr($boxmsg, $npos, 4));
               $npos += 4;
               my $fragperseg = unpack("N", substr($boxmsg, $npos, 4));
               $numfrags = $fragperseg;
               $npos += 4;
print "    First Segment $firstseg Frag per Seg $fragperseg\n";
            }
print "  reached - " . ($npos - $ipos)." $segentlen\n";
            $ipos += $segentlen;
        }
        my $frgtabcnt = unpack('c', substr($boxmsg, $ipos, 1));
        $ipos += 1;
        for (my $i = 0; $i < $frgtabcnt; $i++) {
            my $npos = $ipos;
            my $frgentlen = unpack("N", substr($boxmsg, $npos, 4));
            $npos = $npos+4;
            my $frgtype = substr($boxmsg, $npos, 4);
            $npos = $npos+4;
            print "Frag Type $frgtype len $frgentlen\n";
            my $flags = unpack('N', substr($boxmsg, $npos, 4));
            # version 0xff000000 flags 0x00ffffff
            $npos = $npos+4;
            my $timescale = unpack('N', substr($boxmsg, $npos, 4));
print "  TimeScale ".$timescale."\n";
            $npos = $npos+4;
            my $qualentcnt = unpack('c', substr($boxmsg, $npos, 1));
            $npos = $npos + 1;
            for (my $i = 0; $i < $qualentcnt; $i++) {
                $npos = index($boxmsg, $nulchar, $npos) + 1;
            }
            my $fragruncnt = unpack('N', substr($boxmsg, $npos, 4));
            $npos = $npos + 4;
print "  Frag Run Count $fragruncnt\n";
            for (my $i = 0; $i < $fragruncnt; $i++) {
                my $firstfrag = unpack('N', substr($boxmsg, $npos, 4));
                $npos = $npos + 4;
                my $timestamp = unpack('N', substr($boxmsg, $npos, 4));
                $npos = $npos + 4;
                $timestamp = unpack('N', substr($boxmsg, $npos, 4)) + ($timestamp << 32);
                $npos = $npos + 4;
                my $fragduration = unpack('N', substr($boxmsg, $npos, 4));
                $npos = $npos + 4;
                if ($fragduration == 0) {
                    my $discon = unpack('c', substr($boxmsg, $npos, 1));
                    $npos = $npos + 1;
                    if ($discon == 0 ) {
print "    frag - end of presentation\n";
                    }
                }
                else {
print "    first frag $firstfrag  Frag timestamp $timestamp Frag duration $fragduration\n";
                }
            }
print "  reached - " . ($npos - $ipos ) ." $frgentlen\n";
            $ipos += $frgentlen;
        }
        print "Reached ". ($box_data{boxhdr_len} + $ipos) ."\n";
    } 
}
#endif
}
static void hds_download(struct catchup_t* cu, char* streamUri)
{
  struct MemoryStruct chunk;
  unsigned char *p, *q;
  base64_decodestate state_in;
  int fd;

  fprintf(stderr,"Fetching %s\n",streamUri);

  xml_fetch(streamUri,&chunk);

  char filename[20];
  sprintf(filename,"%s.f4m",cu->id);
  fd = open(filename,O_CREAT|O_TRUNC|O_RDWR,0666);
  if (fd >= 0) {
    write(fd,chunk.memory,chunk.size);
    close(fd);
  }

  xmlDocPtr doc = xmlParseMemory(chunk.memory,chunk.size);
  if (doc == NULL) fprintf(stderr,"XML Parse failure\n");

  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);

  if (xmlXPathRegisterNs(xpathCtx, (xmlChar*)"f4m", (xmlChar*)"http://ns.adobe.com/f4m/1.0") < 0) {
    fprintf(stderr,"Unable to register ion namespace\n");
    printf("Status=ERROR\n");
    return -1;
  }

  xmlChar* bootstrapInfo_b64;
  if ((bootstrapInfo_b64 = myXpathGetValue(xpathCtx, (xmlChar*)"/f4m:manifest/f4m:bootstrapInfo", NULL)) == NULL) {
    fprintf(stderr,"bootstrapInfo element not found\n");
    return 1;
  }

  p = bootstrapInfo_b64;
  while (*p < 33) p++;
  q = p+1;
  while (*q >=33) q++; *q=0;
  fprintf(stderr,"bootstrapInfo_b64=%s\n",p);

  base64_init_decodestate(&state_in);

  unsigned char bootstrapInfo[1024];

  int len = base64_decode_block((signed char*)p,strlen((char*)p),(signed char*)bootstrapInfo,&state_in);

  fprintf(stderr,"len=%d\n",len);
  int i;
  for (i=0;i<len;i++) { fprintf(stderr,"%02x ",bootstrapInfo[i]); }

  parse_bootstrap(bootstrapInfo,len);

  xmlChar* drmAdditionalHeader_b64;
  if ((drmAdditionalHeader_b64 = myXpathGetValue(xpathCtx, (xmlChar*)"/f4m:manifest/f4m:drmAdditionalHeader", NULL)) == NULL) {
    fprintf(stderr,"drmAdditionalHeader element not found\n");
    return 1;
  }

  p = drmAdditionalHeader_b64;
  while (*p < 33) p++;
  q = p+1;
  while (*q >=33) q++; *q=0;
  fprintf(stderr,"drmAdditionalHeader_b64=%s\n",p);
  fprintf(stderr,"len=%d\n",strlen(drmAdditionalHeader_b64));

  base64_init_decodestate(&state_in);

  unsigned char* drmAdditionalHeader = malloc(strlen(drmAdditionalHeader_b64));  // binary version will be smaller.

  len = base64_decode_block((signed char*)p,strlen((char*)p),(signed char*)drmAdditionalHeader,&state_in);

  fprintf(stderr,"len=%d\n",len);

  sprintf(filename,"%s.amf",cu->id);
  fd = open(filename,O_CREAT|O_TRUNC|O_RDWR,0666);
  if (fd >= 0) {
    write(fd,drmAdditionalHeader,len);
    close(fd);
  }

  AMFObject obj;
  int nRes = AMF_Decode(&obj, drmAdditionalHeader+4,len-4, TRUE);
  if (nRes < 0)
    {
      fprintf(stderr, "%s, error decoding invoke packet", __FUNCTION__);
      return 0;
    }

  int old_debug = RTMP_debuglevel;
  RTMP_debuglevel = RTMP_LOGDEBUG;
  AMF_Dump(&obj);
  RTMP_debuglevel = old_debug;

  return 1;
}

int catchup_get_info_4oD(struct catchup_t* cu)
{
  struct MemoryStruct chunk;
  char url[128];

  /* Start of code... */

  xmlInitParser();

  snprintf(url,sizeof(url),"http://ais.channel4.com/asset/%s",cu->id);

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

  xmlChar* streamUri;
  if ((streamUri = myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/uriData/streamUri", NULL)) == NULL) {
    fprintf(stderr,"streamUri element not found\n");
    return 1;
  }

  cu->brandTitle = (char*)myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/brandTitle", NULL);
  cu->episodeTitle = (char*)myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/episodeTitle", NULL);
  cu->programmeNumber = (char*)myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/programmeNumber", NULL);

  char* ii;

  char protocol[1024];
  strcpy(protocol,(char*)streamUri);
  ii = index(protocol,':');
  *ii = 0;

  if (strcmp(protocol,"http")==0) {
    hds_download(cu,(char*)streamUri);
    return 1;
  } else if (strcmp(protocol,"rtmpe")!=0) {
    fprintf(stderr,"Unsupported protocol \"%s\", aborting.\n",protocol);
    return 1;
  }
  cu->protocol = RTMP_PROTOCOL_RTMPE;

  xmlChar* assetId;
  if ((assetId = myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/assetId", NULL)) == NULL) {
    fprintf(stderr,"assetId element not found\n");
    return 1;
  }

  xmlChar* token;
  if ((token = myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/uriData/token", NULL)) == NULL) {
    fprintf(stderr,"token element not found\n");
    return 1;
  }


  base64_decodestate state_in;

  base64_init_decodestate(&state_in);

  char token_decrypted[1024];

  int len = base64_decode_block((signed char*)token,strlen((char*)token),(signed char*)token_decrypted,&state_in);

  static BLOWFISH_CTX ctx;

  Blowfish_Init(&ctx,(unsigned char*)"STINGMIMI",9);
  if (len % 8) {
    fprintf(stderr,"token length is not a multiple of 8, aborting\n");
    return 1;
  }

  int i;
  for (i=0;i<len;i+=8) {
    Blowfish_Decrypt(&ctx, (unsigned char*)(token_decrypted + i));
  }

  char* s = token_decrypted;
  while (*s) {
    if (*s < 32) { *s = 0; }
    s++;
  }
  //fprintf(stderr,"len=%d\n",len);

  char auth[1024]; // = malloc(strlen(e)+strlen(token))+50;

  xmlChar* cdn;
  if ((cdn = myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/uriData/cdn", NULL)) == NULL) {
    fprintf(stderr,"cdn element not found\n");
    return 1;
  }

  if (strcmp((char*)cdn,"ll")==0) {
    xmlChar* e;
    if ((e = myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/uriData/e", NULL)) == NULL) {
      fprintf(stderr,"e element not found\n");
      return 1;
    }
    sprintf(auth,"e=%s&h=%s",e,token_decrypted);  
  } else {
    xmlChar* fingerprint;
    if ((fingerprint = myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/uriData/fingerprint", NULL)) == NULL) {
      fprintf(stderr,"fingerprint element not found\n");
      return 1;
    }

    xmlChar* slist;
    if ((slist = myXpathGetValue(xpathCtx, (xmlChar*)"/Channel4ServiceResult/assetInfo/uriData/slist", NULL)) == NULL) {
      fprintf(stderr,"slist element not found\n");
      return 1;
    }

    sprintf(auth,"auth=%s&aifp=%s&slist=%s",token_decrypted,fingerprint,slist);
    //fprintf(stderr,"HERE: auth=%s\n",auth);
  }

  ii = strstr((char*)streamUri,"mp4:");
  cu->tcUrl = malloc(ii-(char*)streamUri);
  memcpy(cu->tcUrl,streamUri,(ii-(char*)streamUri)-1);
  cu->tcUrl[(ii-(char*)streamUri)-1]=0;

  //char* playpath="";
  //fprintf(stderr,"ii=%s\n",ii);
  cu->playpath = malloc(strlen(ii)+1+strlen(auth)+1);
  sprintf(cu->playpath,"%s?%s",ii,auth);

  cu->port = 1935;

  cu->host = strdup(cu->tcUrl+8);  // strip the "rtmpe://"
  ii = index(cu->host,'/');
  cu->app = strdup(ii+1);
  *ii = 0;  // Truncate string


  //fprintf(stderr,"hostname=%s\n",hostname);
  //fprintf(stderr,"app=%s\n",app);

  cu->swfVfy = strdup("http://www.channel4.com/static/programmes/asset/flash/swf/4odplayer-11.31.swf");

  // e.g. streamUri=rtmpe://ll.securestream.channel4.com/a4174/e1/mp4:xcuassets/CH4_08_02_900_52556010001001_002.mp4

  //fprintf(stderr,"streamUri=%s\n",streamUri);
  //fprintf(stderr,"tcUrl=%s\n",tcUrl);

  //fprintf(stderr,"assetId=%s\n",assetId);
  //fprintf(stderr,"token=%s\n",token);
  //fprintf(stderr,"cdn=%s\n",cdn);
  //fprintf(stderr,"e=%s\n",e);
  //fprintf(stderr,"auth=%s\n",auth);

#if 0
  Working from rtmpdump command-line:
  DEBUG: Protocol : RTMPE
  DEBUG: Hostname : ll.securestream.channel4.com
  DEBUG: Port     : 1935
  DEBUG: Playpath : mp4:xcuassets/CH4_08_02_16_48848001001001_002.mp4?e=1347098331&h=ab17f96b8189b9cf9597ccd3ba5628fb
  DEBUG: tcUrl    : rtmpe://ll.securestream.channel4.com:1935/a4174/e1
  DEBUG: swfUrl   : http://www.channel4.com/static/programmes/asset/flash/swf/4odplayer-11.31.swf
  DEBUG: app      : a4174/e1
  DEBUG: flashVer : WIN 11,0,1,152
  DEBUG: live     : no
#endif

  cu->flashVer = strdup("WIN 11,0,1,152");
  cu->conn = strdup("Z:");

  if(chunk.memory)
    free(chunk.memory);

  return 0;
}

