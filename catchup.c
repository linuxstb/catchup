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

int catchup_get_info(int provider, char* id, struct catchup_t* cu)
{
  memset(cu,0,sizeof(struct catchup_t));
  cu->provider = provider;
  cu->id = strdup(id);

  switch(provider) {
    case CATCHUP_IPLAYER:
      return catchup_get_info_iplayer(cu);

    case CATCHUP_ITV:
      return catchup_get_info_itv(cu);

    case CATCHUP_4OD:
      return catchup_get_info_4oD(cu);

    case CATCHUP_DEMAND5:
      return catchup_get_info_demand5(cu);
  }

  return 1;
}

int catchup_download_file(char* outfile, struct catchup_t* cu)
{
  return do_rtmp(cu->port,cu->protocol,cu->playpath,cu->host,cu->swfVfy,cu->tcUrl,cu->app,cu->pageUrl,cu->flashVer,cu->conn,outfile);
}
