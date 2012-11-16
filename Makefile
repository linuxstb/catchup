LIBRTMP_OBJS = rtmpdump/librtmp/amf.o rtmpdump/librtmp/hashswf.o rtmpdump/librtmp/log.o rtmpdump/librtmp/parseurl.o rtmpdump/librtmp/rtmp.o
CFLAGS_RTMP = -DCRYPTO -Ilibrtmp/rtmpdump

all: catchup ion

catchup.o: catchup.c catchup.h
	gcc -W -Wall -I/usr/include/libxml2 -c -o catchup.o catchup.c

catchup_utils.o: catchup_utils.c catchup_internal.h catchup.h
	gcc -W -Wall -I/usr/include/libxml2 -c -o catchup_utils.o catchup_utils.c

catchup_iplayer.o: catchup_iplayer.c catchup_internal.h catchup.h
	gcc -W -Wall -I/usr/include/libxml2 -c -o catchup_iplayer.o catchup_iplayer.c

catchup_itv.o: catchup_itv.c catchup_internal.h catchup.h
	gcc -W -Wall -I/usr/include/libxml2 -c -o catchup_itv.o catchup_itv.c

catchup_4oD.o: catchup_4oD.c catchup_internal.h catchup.h
	gcc -W -Wall -I/usr/include/libxml2 -c -o catchup_4oD.o catchup_4oD.c

catchup_demand5.o: catchup_demand5.c catchup_internal.h catchup.h
	gcc -W -Wall -I/usr/include/libxml2 -c -o catchup_demand5.o catchup_demand5.c

catchup: main.o catchup.o rtmpdump.o catchup_utils.o catchup_iplayer.o catchup_itv.o catchup_4oD.o catchup_demand5.o $(LIBRTMP_OBJS)
	gcc -o catchup main.o catchup.o rtmpdump.o catchup_utils.o catchup_iplayer.o catchup_itv.o catchup_4oD.o catchup_demand5.o $(LIBRTMP_OBJS) -lcurl -lxml2

rtmpdump.o: rtmpdump.c
	gcc -W -Wall $(CFLAGS_RTMP) -c -o rtmpdump.o rtmpdump.c

main.o: main.c catchup.h
	gcc -W -Wall -c -o main.o main.c

ion: ion.o catchup_utils.o
	gcc -o ion ion.o catchup_utils.o -lcurl -lxml2

ion.o: ion.c
	gcc -W -Wall -I/usr/include/libxml2 -c -o ion.o ion.c

# Local copy of librtmp
rtmpdump/librtmp/amf.o: rtmpdump/librtmp/amf.c
	$(CC) $(CFLAGS_RTMP) -c $< -o $@

rtmpdump/librtmp/hashswf.o: rtmpdump/librtmp/hashswf.c
	$(CC) $(CFLAGS_RTMP) -c $< -o $@

rtmpdump/librtmp/log.o: rtmpdump/librtmp/log.c
	$(CC) $(CFLAGS_RTMP) -c $< -o $@

rtmpdump/librtmp/parseurl.o: rtmpdump/librtmp/parseurl.c
	$(CC) $(CFLAGS_RTMP) -c $< -o $@

rtmpdump/librtmp/rtmp.o: rtmpdump/librtmp/rtmp.c
	$(CC) $(CFLAGS_RTMP) -c $< -o $@

clean:
	rm -f catchup main.o catchup.o rtmpdump.o catchup_utils.o catchup_iplayer.o catchup_itv.o catchup_4oD.o catchup_demand5.o $(LIBRTMP_OBJS) *~
