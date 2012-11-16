/*
 * This is a heavily modified version of the rtmpdump.c included with rtmpdump.
 * The following (C) applies:
 * 
 *  RTMPDump
 *  Copyright (C) 2009 Andrej Stepanchuk
 *  Copyright (C) 2009 Howard Chu
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTMPDump; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#define _FILE_OFFSET_BITS	64

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#include <signal.h>		// to catch Ctrl-C
#include <getopt.h>

#include "rtmpdump/librtmp/rtmp.h"
#include "rtmpdump/librtmp/log.h"

#define	SET_BINMODE(f)

#define RD_SUCCESS		0
#define RD_FAILED		1
#define RD_INCOMPLETE		2
#define RD_NO_CONNECT		3

#define DEF_TIMEOUT	10	/* seconds */
#define DEF_BUFTIME	(10 * 60 * 60 * 1000)	/* 10 hours default */
#define DEF_SKIPFRM	0

#ifdef _DEBUG
uint32_t debugTS = 0;
int pnum = 0;

FILE *netstackdump = 0;
FILE *netstackdump_read = 0;
#endif

uint32_t nIgnoredFlvFrameCounter = 0;
uint32_t nIgnoredFrameCounter = 0;
#define MAX_IGNORED_FRAMES	50

FILE *file = 0;

static const AVal av_conn = AVC("conn");

void
sigIntHandler(int sig)
{
  RTMP_ctrlC = TRUE;
  RTMP_LogPrintf("Caught signal: %d, cleaning up, just a second...\n", sig);
  // ignore all these signals now and let the connection close
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
}

int
Download(RTMP * rtmp,		// connected RTMP object
	 FILE * file, uint32_t dSeek, uint32_t dStopOffset, double duration, int bResume, char *metaHeader, uint32_t nMetaHeaderSize, char *initialFrame, int initialFrameType, uint32_t nInitialFrameSize, int nSkipKeyFrames, int bLiveStream, int bHashes, int bOverrideBufferTime, uint32_t bufferTime, double *percent)	// percentage downloaded [out]
{
  int32_t now, lastUpdate;
  int bufferSize = 64 * 1024;
  char *buffer;
  int nRead = 0;
  off_t size = ftello(file);
  unsigned long lastPercent = 0;

  rtmp->m_read.timestamp = dSeek;

  *percent = 0.0;

  if (rtmp->m_read.timestamp)
    {
      RTMP_Log(RTMP_LOGDEBUG, "Continuing at TS: %d ms\n", rtmp->m_read.timestamp);
    }

  if (bLiveStream)
    {
      RTMP_LogPrintf("Starting Live Stream\n");
    }
  else
    {
      // print initial status
      // Workaround to exit with 0 if the file is fully (> 99.9%) downloaded
      if (duration > 0)
	{
	  if ((double) rtmp->m_read.timestamp >= (double) duration * 999.0)
	    {
	      RTMP_LogPrintf("Already Completed at: %.3f sec Duration=%.3f sec\n",
			(double) rtmp->m_read.timestamp / 1000.0,
			(double) duration / 1000.0);
	      return RD_SUCCESS;
	    }
	  else
	    {
	      *percent = ((double) rtmp->m_read.timestamp) / (duration * 1000.0) * 100.0;
	      *percent = ((double) (int) (*percent * 10.0)) / 10.0;
	      RTMP_LogPrintf("%s download at: %.3f kB / %.3f sec (%.1f%%)\n",
			bResume ? "Resuming" : "Starting",
			(double) size / 1024.0, (double) rtmp->m_read.timestamp / 1000.0,
			*percent);
	    }
	}
      else
	{
	  RTMP_LogPrintf("%s download at: %.3f kB\n",
		    bResume ? "Resuming" : "Starting",
		    (double) size / 1024.0);
	}
    }

  if (dStopOffset > 0)
    RTMP_LogPrintf("For duration: %.3f sec\n", (double) (dStopOffset - dSeek) / 1000.0);

  if (bResume && nInitialFrameSize > 0)
    rtmp->m_read.flags |= RTMP_READ_RESUME;
  rtmp->m_read.initialFrameType = initialFrameType;
  rtmp->m_read.nResumeTS = dSeek;
  rtmp->m_read.metaHeader = metaHeader;
  rtmp->m_read.initialFrame = initialFrame;
  rtmp->m_read.nMetaHeaderSize = nMetaHeaderSize;
  rtmp->m_read.nInitialFrameSize = nInitialFrameSize;

  buffer = (char *) malloc(bufferSize);

  now = RTMP_GetTime();
  lastUpdate = now - 1000;
  do
    {
      nRead = RTMP_Read(rtmp, buffer, bufferSize);
      //RTMP_LogPrintf("nRead: %d\n", nRead);
      if (nRead > 0)
	{
	  if (fwrite(buffer, sizeof(unsigned char), nRead, file) !=
	      (size_t) nRead)
	    {
	      RTMP_Log(RTMP_LOGERROR, "%s: Failed writing, exiting!", __FUNCTION__);
	      free(buffer);
	      return RD_FAILED;
	    }
	  size += nRead;

	  //RTMP_LogPrintf("write %dbytes (%.1f kB)\n", nRead, nRead/1024.0);
	  if (duration <= 0)	// if duration unknown try to get it from the stream (onMetaData)
	    duration = RTMP_GetDuration(rtmp);

	  if (duration > 0)
	    {
	      // make sure we claim to have enough buffer time!
	      if (!bOverrideBufferTime && bufferTime < (duration * 1000.0))
		{
		  bufferTime = (uint32_t) (duration * 1000.0) + 5000;	// extra 5sec to make sure we've got enough

		  RTMP_Log(RTMP_LOGDEBUG,
		      "Detected that buffer time is less than duration, resetting to: %dms",
		      bufferTime);
		  RTMP_SetBufferMS(rtmp, bufferTime);
		  RTMP_UpdateBufferMS(rtmp);
		}
	      *percent = ((double) rtmp->m_read.timestamp) / (duration * 1000.0) * 100.0;
	      *percent = ((double) (int) (*percent * 10.0)) / 10.0;
	      if (bHashes)
		{
		  if (lastPercent + 1 <= *percent)
		    {
		      RTMP_LogStatus("#");
		      lastPercent = (unsigned long) *percent;
		    }
		}
	      else
		{
		  now = RTMP_GetTime();
		  if (abs(now - lastUpdate) > 200)
		    {
		      fprintf(stderr,"\rDAVE: %.3f kB / %.2f sec (%.1f%%)",
				(double) size / 1024.0,
				(double) (rtmp->m_read.timestamp) / 1000.0, *percent);
		      lastUpdate = now;
		    }
		}
	    }
	  else
	    {
	      now = RTMP_GetTime();
	      if (abs(now - lastUpdate) > 200)
		{
		  if (bHashes)
		    RTMP_LogStatus("#");
		  else
		    fprintf(stderr,"\r%.3f kB / %.2f sec", (double) size / 1024.0,
			      (double) (rtmp->m_read.timestamp) / 1000.0);
		  lastUpdate = now;
		}
	    }
	}
#ifdef _DEBUG
      else
	{
	  RTMP_Log(RTMP_LOGDEBUG, "zero read!");
	}
#endif

    }
  while (!RTMP_ctrlC && nRead > -1 && RTMP_IsConnected(rtmp) && !RTMP_IsTimedout(rtmp));
  free(buffer);
  if (nRead < 0)
    nRead = rtmp->m_read.status;

  /* Final status update */
  if (!bHashes)
    {
      if (duration > 0)
	{
	  *percent = ((double) rtmp->m_read.timestamp) / (duration * 1000.0) * 100.0;
	  *percent = ((double) (int) (*percent * 10.0)) / 10.0;
	  RTMP_LogStatus("\r%.3f kB / %.2f sec (%.1f%%)",
	    (double) size / 1024.0,
	    (double) (rtmp->m_read.timestamp) / 1000.0, *percent);
	}
      else
	{
	  RTMP_LogStatus("\r%.3f kB / %.2f sec", (double) size / 1024.0,
	    (double) (rtmp->m_read.timestamp) / 1000.0);
	}
    }

  RTMP_Log(RTMP_LOGDEBUG, "RTMP_Read returned: %d", nRead);

  if (bResume && nRead == -2)
    {
      RTMP_LogPrintf("Couldn't resume FLV file, try --skip %d\n\n",
		nSkipKeyFrames + 1);
      return RD_FAILED;
    }

  if (nRead == -3)
    return RD_SUCCESS;

  if ((duration > 0 && *percent < 99.9) || RTMP_ctrlC || nRead < 0
      || RTMP_IsTimedout(rtmp))
    {
      return RD_INCOMPLETE;
    }

  return RD_SUCCESS;
}

static void STR2AVAL(AVal* av,char* str)
{
  av->av_val = str; 
  av->av_len = strlen(av->av_val);
}

int
do_rtmp(int port,int protocol,char* playpath_arg,char* host,char* swfVfy_arg,char* tcUrl_arg,char* app_arg,char* pageUrl_arg,char* flashVer_arg,char* conn,char* outfile)
{
  int nStatus = RD_SUCCESS;
  double percent = 0;
  double duration = 0.0;

  int nSkipKeyFrames = DEF_SKIPFRM;	// skip this number of keyframes when resuming

  int bOverrideBufferTime = FALSE;	// if the user specifies a buffer time override this is true
  int bResume = FALSE;		// true in resume mode
  uint32_t dSeek = 0;		// seek position in resume mode, 0 otherwise
  uint32_t bufferTime = DEF_BUFTIME;


  // meta header and initial frame for the resume mode (they are read from the file and compared with
  // the stream we are trying to continue
  char *metaHeader = 0;
  uint32_t nMetaHeaderSize = 0;

  // video keyframe for matching
  char *initialFrame = 0;
  uint32_t nInitialFrameSize = 0;
  int initialFrameType = 0;	// tye: audio or video

  AVal hostname = { 0, 0 };
  AVal playpath = { 0, 0 };
  AVal subscribepath = { 0, 0 };
  AVal usherToken = { 0, 0 }; //Justin.tv auth token
  int retries = 0;
  int bLiveStream = FALSE;	// is it a live stream? then we can't seek/resume
  int bHashes = FALSE;		// display byte counters not hashes by default

  long int timeout = DEF_TIMEOUT;	// timeout connection after 120 seconds
  uint32_t dStopOffset = 0;
  RTMP rtmp;

  AVal swfUrl = { 0, 0 };
  AVal tcUrl = { 0, 0 };
  AVal pageUrl = { 0, 0 };
  AVal app = { 0, 0 };
  AVal auth = { 0, 0 };
  AVal swfHash = { 0, 0 };
  uint32_t swfSize = 0;
  AVal flashVer = { 0, 0 };
  AVal sockshost = { 0, 0 };

#ifdef CRYPTO
  int swfAge = 30;	/* 30 days for SWF cache by default */
  int swfVfy = 0;
  unsigned char hash[RTMP_SWF_HASHLEN];
#endif

  char *flvFile = 0;

  signal(SIGINT, sigIntHandler);
  signal(SIGTERM, sigIntHandler);
  signal(SIGHUP, sigIntHandler);
  signal(SIGPIPE, sigIntHandler);
  signal(SIGQUIT, sigIntHandler);

  RTMP_debuglevel = RTMP_LOGINFO;

  RTMP_Init(&rtmp);

  STR2AVAL(&swfUrl,swfVfy_arg);
  swfVfy = 1;

  if (host) { STR2AVAL(&hostname, host); }
  if (playpath_arg) { STR2AVAL(&playpath, playpath_arg); }
  if (tcUrl_arg) { STR2AVAL(&tcUrl, tcUrl_arg); }
  if (pageUrl_arg) { STR2AVAL(&pageUrl, pageUrl_arg); }
  if (app_arg) { STR2AVAL(&app, app_arg); }
  if (flashVer_arg) { STR2AVAL(&flashVer, flashVer_arg); }

  if (conn) {
    AVal av;
    STR2AVAL(&av, conn);
    if (!RTMP_SetOpt(&rtmp, &av_conn, &av))
    {
      RTMP_Log(RTMP_LOGERROR, "Invalid AMF parameter: %s", conn);
      return RD_FAILED;
    }
  }

  flvFile = outfile;

  file = fopen(flvFile, "w+b");
  if (file == 0)
    {
      RTMP_LogPrintf("Failed to open file! %s\n", flvFile);
      return RD_FAILED;
    }

  if (port == 0)
    {
      if (protocol & RTMP_FEATURE_SSL)
	port = 443;
      else if (protocol & RTMP_FEATURE_HTTP)
	port = 80;
      else
	port = 1935;
    }

#ifdef CRYPTO
  if (swfVfy)
    {
      if (RTMP_HashSWF(swfUrl.av_val, &swfSize, hash, swfAge) == 0)
        {
          swfHash.av_val = (char *)hash;
          swfHash.av_len = RTMP_SWF_HASHLEN;
        }
    }

  if (swfHash.av_len == 0 && swfSize > 0)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "Ignoring SWF size, supply also the hash with --swfhash");
      swfSize = 0;
    }

  if (swfHash.av_len != 0 && swfSize == 0)
    {
      RTMP_Log(RTMP_LOGWARNING,
	  "Ignoring SWF hash, supply also the swf size  with --swfsize");
      swfHash.av_len = 0;
      swfHash.av_val = NULL;
    }
#endif

  RTMP_SetupStream(&rtmp, protocol, &hostname, port, &sockshost, &playpath,
		   &tcUrl, &swfUrl, &pageUrl, &app, &auth, &swfHash, swfSize,
		   &flashVer, &subscribepath, &usherToken, dSeek, dStopOffset, bLiveStream, timeout);

  /* Try to keep the stream moving if it pauses on us */
  if (!bLiveStream && !(protocol & RTMP_FEATURE_HTTP))
    rtmp.Link.lFlags |= RTMP_LF_BUFX;

  int first = 1;

  while (!RTMP_ctrlC)
    {
      RTMP_Log(RTMP_LOGDEBUG, "Setting buffer time to: %dms", bufferTime);
      RTMP_SetBufferMS(&rtmp, bufferTime);

      if (first)
	{
	  first = 0;
	  RTMP_LogPrintf("Connecting ...\n");

	  if (!RTMP_Connect(&rtmp, NULL))
	    {
	      nStatus = RD_NO_CONNECT;
	      break;
	    }

	  RTMP_Log(RTMP_LOGINFO, "Connected...");

	  if (!RTMP_ConnectStream(&rtmp, dSeek))
	    {
	      nStatus = RD_FAILED;
	      break;
	    }
	}
      else
	{
	  nInitialFrameSize = 0;

          if (retries)
            {
	      RTMP_Log(RTMP_LOGERROR, "Failed to resume the stream\n\n");
	      if (!RTMP_IsTimedout(&rtmp))
	        nStatus = RD_FAILED;
	      else
	        nStatus = RD_INCOMPLETE;
	      break;
            }
	  RTMP_Log(RTMP_LOGINFO, "Connection timed out, trying to resume.\n\n");
          /* Did we already try pausing, and it still didn't work? */
          if (rtmp.m_pausing == 3)
            {
              /* Only one try at reconnecting... */
              retries = 1;
              dSeek = rtmp.m_pauseStamp;
              if (dStopOffset > 0)
                {
                  if (dStopOffset <= dSeek)
                    {
                      RTMP_LogPrintf("Already Completed\n");
		      nStatus = RD_SUCCESS;
		      break;
                    }
                }
              if (!RTMP_ReconnectStream(&rtmp, dSeek))
                {
	          RTMP_Log(RTMP_LOGERROR, "Failed to resume the stream\n\n");
	          if (!RTMP_IsTimedout(&rtmp))
		    nStatus = RD_FAILED;
	          else
		    nStatus = RD_INCOMPLETE;
	          break;
                }
            }
	  else if (!RTMP_ToggleStream(&rtmp))
	    {
	      RTMP_Log(RTMP_LOGERROR, "Failed to resume the stream\n\n");
	      if (!RTMP_IsTimedout(&rtmp))
		nStatus = RD_FAILED;
	      else
		nStatus = RD_INCOMPLETE;
	      break;
	    }
	  bResume = TRUE;
	}

      nStatus = Download(&rtmp, file, dSeek, dStopOffset, duration, bResume,
			 metaHeader, nMetaHeaderSize, initialFrame,
			 initialFrameType, nInitialFrameSize,
			 nSkipKeyFrames, bLiveStream, bHashes,
			 bOverrideBufferTime, bufferTime, &percent);
      free(initialFrame);
      initialFrame = NULL;

      /* If we succeeded, we're done.
       */
      if (nStatus != RD_INCOMPLETE || !RTMP_IsTimedout(&rtmp) || bLiveStream)
	break;
    }

  if (nStatus == RD_SUCCESS)
    {
      RTMP_LogPrintf("Download complete\n");
    }
  else if (nStatus == RD_INCOMPLETE)
    {
      RTMP_LogPrintf
	("Download may be incomplete (downloaded about %.2f%%), try resuming\n",
	 percent);
    }

  RTMP_Log(RTMP_LOGDEBUG, "Closing connection.\n");
  RTMP_Close(&rtmp);

  if (file != 0)
    fclose(file);

  return nStatus;
}
