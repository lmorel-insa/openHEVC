//
//  main.c
//  libavHEVC
//
//  Created by Mickaël Raulet on 11/10/12.
//
//
#include "openHevcWrapper.h"
#include "getopt.h"
#include <libavformat/avformat.h>

#ifdef MEMORY_SAMPLING_ENABLE
#include <numa.h>
#include "numap.h"
#define MAX_THREAD_NB 12
#endif

//#define TIME2

#ifdef TIME2
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
//#include <ctime>
#endif
#define FRAME_CONCEALMENT   0



// Sampling parameters
int mmap_pages_count = 8192; // must be power of two


// #ifdef MEMORY_SAMPLING_ENABLE
void dump_mem_bdw_samples(void) {

	FILE *f;
	int i, j;
	f = fopen("mem_bdw_samples", "w");
	for (i = 0; i < nb_mem_bdw_samples; i++) {
		for(j = 0; j < mem_bdw_measure.nb_nodes; j++) {
			fprintf(f, "%d %" PRIu64 " %" PRIu64 "\n", j, mem_bdw_samples[j][i].read_bdw, mem_bdw_samples[j][i].write_bdw);
		}
	}
	fclose(f);
}






/* Returns the amount of milliseconds elapsed since the UNIX epoch. Works on both
 * windows and linux. */

static unsigned long int GetTimeMs64()
{
#ifdef WIN32
    /* Windows */
    FILETIME ft;
    LARGE_INTEGER li;
    
    /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
     * to a LARGE_INTEGER structure. */
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    
    uint64_t ret = li.QuadPart;
    ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
    ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */
    
    return ret;
#else
    /* Linux */
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    
    unsigned long int ret = tv.tv_usec;
    /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
    //ret /= 1000;
    
    /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
    ret += (tv.tv_sec * 1000000);
    
    return ret;
#endif
}
#endif

typedef struct OpenHevcWrapperContext {
    AVCodec *codec;
    AVCodecContext *c;
    AVFrame *picture;
    AVPacket avpkt;
    AVCodecParserContext *parser;
} OpenHevcWrapperContext;

int find_start_code (unsigned char *Buf, int zeros_in_startcode)
{
    int i;
    for (i = 0; i < zeros_in_startcode; i++)
        if(Buf[i] != 0)
            return 0;
    return Buf[i];
}

int get_next_nal(FILE* inpf, unsigned char* Buf)
{
    int pos = 0;
    int StartCodeFound = 0;
    int info2 = 0;
    int info3 = 0;
    while(!feof(inpf)&&(/*Buf[pos++]=*/fgetc(inpf))==0);

    while (pos < 3) Buf[pos++] = fgetc (inpf);
    while (!StartCodeFound)
    {
        if (feof (inpf))
        {
            //            return -1;
            return pos-1;
        }
        Buf[pos++] = fgetc (inpf);
        info3 = find_start_code(&Buf[pos-4], 3);
        if(info3 != 1)
            info2 = find_start_code(&Buf[pos-3], 2);
        StartCodeFound = (info2 == 1 || info3 == 1);
    }
    fseek (inpf, - 4 + info2, SEEK_CUR);
    return pos - 4 + info2;
}
typedef struct Info {
    int NbFrame;
    int Poc;
    int Tid;
    int Qid;
    int type;
    int size;
} Info;

static void video_decode_example(const char *filename)
{
    AVFormatContext *pFormatCtx=NULL;
    AVPacket        packet;
#if FRAME_CONCEALMENT
    FILE *fin_loss = NULL, *fin1 = NULL;
    Info info;
    Info info_loss;
    char filename0[1024];
    int is_received = 1;
#endif
    FILE *fout  = NULL;
    int width   = -1;
    int height  = -1;
    int nbFrame = 0;
    int stop    = 0;
    int stop_dec= 0;
    int got_picture;
    float time  = 0.0;
#ifdef TIME2
    long unsigned int time_us = 0;
#endif
    int video_stream_idx;
    char output_file2[256];

    OpenHevc_Frame     openHevcFrame;
    OpenHevc_Frame_cpy openHevcFrameCpy;
    OpenHevc_Handle    openHevcHandle;

    if (filename == NULL) {
        printf("No input file specified.\nSpecify it with: -i <filename>\n");
        exit(1);
    }

    openHevcHandle = libOpenHevcInit(nb_pthreads, thread_type/*, pFormatCtx*/);
    libOpenHevcSetCheckMD5(openHevcHandle, check_md5_flags);

    if (!openHevcHandle) {
        fprintf(stderr, "could not open OpenHevc\n");
        exit(1);
    }
    av_register_all();
    pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx, filename, NULL, NULL)!=0) {
        printf("%s",filename);
        exit(1); // Couldn't open file
    }
    if ( (video_stream_idx = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0)) < 0) {
        fprintf(stderr, "Could not find video stream in input file\n");
        exit(1);
    }

 //   av_dump_format(pFormatCtx, 0, filename, 0);

    const size_t extra_size_alloc = pFormatCtx->streams[video_stream_idx]->codec->extradata_size > 0 ?
    (pFormatCtx->streams[video_stream_idx]->codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE) : 0;
    if (extra_size_alloc)
    {
        libOpenHevcCopyExtraData(openHevcHandle, pFormatCtx->streams[video_stream_idx]->codec->extradata, extra_size_alloc);
    }

    libOpenHevcSetDebugMode(openHevcHandle, 0);
    libOpenHevcStartDecoder(openHevcHandle);
    openHevcFrameCpy.pvY = NULL;
    openHevcFrameCpy.pvU = NULL;
    openHevcFrameCpy.pvV = NULL;
#if USE_SDL
    Init_Time();
    if (frame_rate > 0) {
        initFramerate_SDL();
        setFramerate_SDL(frame_rate);
    }
#endif
#ifdef TIME2
    time_us = GetTimeMs64();
#endif
   
    libOpenHevcSetTemporalLayer_id(openHevcHandle, temporal_layer_id);
    libOpenHevcSetActiveDecoders(openHevcHandle, quality_layer_id);
    libOpenHevcSetViewLayers(openHevcHandle, quality_layer_id);
#if FRAME_CONCEALMENT
    fin_loss = fopen( "/Users/wassim/Softwares/shvc_transmission/parser/hevc_parser/BascketBall_Loss.txt", "rb");
    fin1 = fopen( "/Users/wassim/Softwares/shvc_transmission/parser/hevc_parser/BascketBall.txt", "rb");
    sprintf(filename0, "%s \n", "Nbframe  Poc Tid  Qid  NalType Length");
    fread ( filename0, strlen(filename), 1, fin_loss);
    fread ( filename0, strlen(filename), 1, fin1);
#endif

    while(!stop) {
        if (stop_dec == 0 && av_read_frame(pFormatCtx, &packet)<0) stop_dec = 1;
#if FRAME_CONCEALMENT
        // Get the corresponding frame in the trace
        if(is_received)
            fscanf(fin_loss, "%d    %d    %d    %d    %d        %d \n", &info_loss.NbFrame, &info_loss.Poc, &info_loss.Tid, &info_loss.Qid, &info_loss.type, &info_loss.size);
        fscanf(fin1, "%d    %d    %d    %d    %d        %d \n", &info.NbFrame, &info.Poc, &info.Tid, &info.Qid, &info.type, &info.size);
        if(info_loss.NbFrame == info.NbFrame)
            is_received = 1;
        else
            is_received = 0;
#endif
        if (packet.stream_index == video_stream_idx || stop_dec == 1) {
#if FRAME_CONCEALMENT
            if(is_received)
                got_picture = libOpenHevcDecode(openHevcHandle, packet.data, !stop_dec ? packet.size : 0, packet.pts);
            else
                got_picture = libOpenHevcDecode(openHevcHandle, NULL,  0, packet.pts);
#else
            got_picture = libOpenHevcDecode(openHevcHandle, packet.data, !stop_dec ? packet.size : 0, packet.pts);
#endif
            if (got_picture > 0) {
                fflush(stdout);
                libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrame.frameInfo);
                if ((width != openHevcFrame.frameInfo.nWidth) || (height != openHevcFrame.frameInfo.nHeight)) {
                    width  = openHevcFrame.frameInfo.nWidth;
                    height = openHevcFrame.frameInfo.nHeight;
                    if (fout)
                       fclose(fout);
                    if (output_file) {
                        sprintf(output_file2, "%s_%dx%d.yuv", output_file, width, height);
                        fout = fopen(output_file2, "wb");
                    }
#if USE_SDL
                    if (display_flags == ENABLE) {
                        Init_SDL((openHevcFrame.frameInfo.nYPitch - openHevcFrame.frameInfo.nWidth)/2, openHevcFrame.frameInfo.nWidth, openHevcFrame.frameInfo.nHeight);
                    }
#endif
                    if (fout) {
                        int format = openHevcFrameCpy.frameInfo.chromat_format == YUV420 ? 1 : 0;
                        libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrameCpy.frameInfo);
                        if(openHevcFrameCpy.pvY) {
                            free(openHevcFrameCpy.pvY);
                            free(openHevcFrameCpy.pvU);
                            free(openHevcFrameCpy.pvV);
                        }
                        openHevcFrameCpy.pvY = calloc (openHevcFrameCpy.frameInfo.nYPitch * openHevcFrameCpy.frameInfo.nHeight, sizeof(unsigned char));
                        openHevcFrameCpy.pvU = calloc (openHevcFrameCpy.frameInfo.nUPitch * openHevcFrameCpy.frameInfo.nHeight >> format, sizeof(unsigned char));
                        openHevcFrameCpy.pvV = calloc (openHevcFrameCpy.frameInfo.nVPitch * openHevcFrameCpy.frameInfo.nHeight >> format, sizeof(unsigned char));
                    }
                }
#if USE_SDL
                if (frame_rate > 0) {
                    framerateDelay_SDL();
                }                
                if (display_flags == ENABLE) {
                    libOpenHevcGetOutput(openHevcHandle, 1, &openHevcFrame);
                    libOpenHevcGetPictureInfo(openHevcHandle, &openHevcFrame.frameInfo);
                    SDL_Display((openHevcFrame.frameInfo.nYPitch - openHevcFrame.frameInfo.nWidth)/2, openHevcFrame.frameInfo.nWidth, openHevcFrame.frameInfo.nHeight,
                            openHevcFrame.pvY, openHevcFrame.pvU, openHevcFrame.pvV);
                }
#endif
                if (fout) {
                    int format = openHevcFrameCpy.frameInfo.chromat_format == YUV420 ? 1 : 0;
                    libOpenHevcGetOutputCpy(openHevcHandle, 1, &openHevcFrameCpy);
                    fwrite( openHevcFrameCpy.pvY , sizeof(uint8_t) , openHevcFrameCpy.frameInfo.nYPitch * openHevcFrameCpy.frameInfo.nHeight, fout);
                    fwrite( openHevcFrameCpy.pvU , sizeof(uint8_t) , openHevcFrameCpy.frameInfo.nUPitch * openHevcFrameCpy.frameInfo.nHeight >> format, fout);
                    fwrite( openHevcFrameCpy.pvV , sizeof(uint8_t) , openHevcFrameCpy.frameInfo.nVPitch * openHevcFrameCpy.frameInfo.nHeight >> format, fout);
                }
                nbFrame++;
                if (nbFrame == num_frames)
                    stop = 1;
            } else {
                if (stop_dec==1 && nbFrame)
                stop = 1;
            }
        }
        av_free_packet(&packet);
    }
#if USE_SDL
    time = SDL_GetTime()/1000.0;
#ifdef TIME2
    time_us = GetTimeMs64() - time_us;
#endif
    CloseSDLDisplay();
#endif
    if (fout) {
        fclose(fout);
        if(openHevcFrameCpy.pvY) {
            free(openHevcFrameCpy.pvY);
            free(openHevcFrameCpy.pvU);
            free(openHevcFrameCpy.pvV);
        }
    }
    avformat_close_input(&pFormatCtx);
    libOpenHevcClose(openHevcHandle);
#if USE_SDL
#ifdef TIME2
    printf("frame= %d fps= %.0f time= %ld video_size= %dx%d\n", nbFrame, nbFrame/time, time_us, openHevcFrame.frameInfo.nWidth, openHevcFrame.frameInfo.nHeight);
#else
    printf("frame= %d fps= %.0f time= %.2f video_size= %dx%d\n", nbFrame, nbFrame/time, time, openHevcFrame.frameInfo.nWidth, openHevcFrame.frameInfo.nHeight);
#endif
#endif
}



// LM - RANGEMENT A FAIRE !! 
struct mem_bdw_sample_s {
	uint64_t read_bdw;
	uint64_t write_bdw;
};

typedef struct mem_bdw_sample_s mem_bdw_sample_t;

mem_bdw_sample_t **mem_bdw_samples;
struct numap_bdw_measure mem_bdw_measure;
int i;
int nb_mem_bdw_samples;
int numap_rc; // numap return codes
struct numap_sampling_measure measures[MAX_THREAD_NB];
///////



void *mem_bdw_sampling_routine(void *arg) {

	unsigned int *freq ;
	unsigned int periodInMicroS;
	int res;
	int i;

	/* if (pthread_setname_np(pthread_self(), "mem_bdw_samp") != 0) { */
	/* 	fprintf(stderr, "Failed to set name of mem_bdw_samp thread"); */
	/* } */

	freq = (unsigned int *)arg;
	periodInMicroS = (unsigned int)(1.0E6 / *freq);

	while (1) {
		res = numap_bdw_start(&mem_bdw_measure);
		if(res < 0) {
			fprintf(stderr, "numap_start error : %s\n", numap_error_message(res));
			exit(-1);
		}
		usleep(periodInMicroS);
		res = numap_bdw_stop(&mem_bdw_measure);
		if(res < 0) {
			fprintf(stderr, "numap_stop error : %s\n", numap_error_message(res));
			exit(-1);
		}
		for(i = 0; i < mem_bdw_measure.nb_nodes; i++) {
			uint64_t r = (mem_bdw_measure.reads_count[i] * 64);
			uint64_t w = (mem_bdw_measure.writes_count[i] * 64);
		    mem_bdw_samples[i][nb_mem_bdw_samples].read_bdw = r;
		    mem_bdw_samples[i][nb_mem_bdw_samples].write_bdw = w;
		}
		nb_mem_bdw_samples++;
	}
	
}


int print_samples (struct numap_sampling_measure *measure, char* file_name) {

	FILE *f;
	int i, j;
	struct perf_event_mmap_page *metadata_page;
	struct perf_event_header *header;
	uint64_t head;
	uint64_t consumed;

	char *actor_found_name;
	char *action_found_name;
	uint64_t action_found_start;
	uint64_t action_found_end;
	char *fifo_found_src_name;
	char *fifo_found_dst_name;
	int action_found;

	f = fopen(file_name, "a");

	//	fprintf(f, "// New Thread %d has %d actors\n", sched->id, sched->num_actors);
	metadata_page = measure->metadata_pages_per_tid[0];
	head = metadata_page -> data_head;
	rmb();
	header = (struct perf_event_header *)((char *)metadata_page + measure->page_size);
	consumed = 0;

	// Parse all samples
	while (consumed < head) {
		if (header->size == 0) {
			fprintf(stderr, "Error: invalid header size = 0\n");
			return -1;
		}
		else {
			fprintf(stderr, "Header size = %d\n", header->size);
		}
		if (header -> type == PERF_RECORD_SAMPLE) {
		  struct read_sample *sample = (struct read_sample *)((char *)(header) + 8);

			actor_found_name = "NOT_FOUND";
			action_found_name = "NOT_FOUND";
			action_found_start = 0;
			action_found_end = 0;
			action_found = 0;
			fifo_found_src_name = "NOT_FOUND";
			fifo_found_dst_name = "NOT_FOUND";

			// dump result
			fprintf(f,  "ip = %" PRIx64
					", @ = %" PRIx64
					", weight = %" PRIu64
					", src_level = %s", 
					//						", conn_src = %s"
					//		", conn_dst = %s\n",
					sample->ip, sample->addr, sample->weight,
					get_data_src_level(sample->data_src)); //,
			//						fifo_found_src_name,
			//				fifo_found_dst_name);
		}
		consumed += header->size;
		header = (struct perf_event_header *)((char *)header + header -> size);
	}
	fprintf(f, "// End Thread\n");
	fclose(f);
}





void dump_mem_samples(void) {
  int i;
  int res;
  size_t stack_size;
  size_t stack_guard_size;
  void *stack_addr;
  pthread_attr_t attr;

  // Stop memory sampling and save results to a file
  for (i = 0; i < nb_pthreads; i++) {
	res = numap_sampling_read_stop(&measures[i]);
	if(res < 0) {
	  fprintf(stderr, "numap_sampling_read_stop error : %s\n", numap_error_message(res));
	  exit(-1);
	} else {
	  fprintf(stdout, "numap_sampling_read_stop OK\n");
	}
  }
  for (i = 0; i < nb_pthreads; i++) {
	print_samples(&measures[i], profiling_output_file);
  }

  // Stop instruction counting
  //ioctl(inst_fd, PERF_EVENT_IOC_DISABLE, 0);
  //read(inst_fd, &insts_count, sizeof(insts_count));

  /* // Get threads stack informations */
  /* for (i = 0; i < nb_pthreads; i++) { */
  /* 	res = pthread_getattr_np(threads[i], &attr); */
  /* 	if (res != 0) { */
  /* 	  fprintf(stderr, "pthread_getattr_np error\n"); */
  /* 	  exit(-1); */
  /* 	} */
  /* 	res = pthread_attr_getguardsize(&attr, &stack_guard_size); */
  /* 	if (res != 0) { */
  /* 	  fprintf(stderr, "pthread_attr_getguardsize error\n"); */
  /* 	  exit(-1); */
  /* 	} */
  /* 	res = pthread_attr_getstack(&attr, &stack_addr, &stack_size); */
  /* 	if (res != 0) { */
  /* 	  fprintf(stderr, "pthread_attr_getstack error\n"); */
  /* 	  exit(-1); */
  /* 	} */
  /* } */
}




int main(int argc, char *argv[]) {
  int i;
  init_main(argc, argv);


  // #ifdef MEMORY_SAMPLING_ENABLE
  // LM - initialize numap
  if (mem_profiling == ENABLE || memory_bdw_sampling_freq > 0) {
	numap_rc = numap_init();
	if(numap_rc < 0) {
	  fprintf(stderr, "numap_init : %s\n", numap_error_message(numap_rc));
	} else {
	  fprintf(stdout, "numap_init OK\n");
	}
  }


  
  for (i = 0; i < nb_pthreads; i++) {
	// init memory access sampling
	numap_rc = numap_sampling_init_measure(&measures[i],1,memory_bdw_sampling_freq,mmap_pages_count);
	if(numap_rc < 0) {
	  fprintf(stderr, "numap_sampling_init error : %s\n", numap_error_message(numap_rc));
	  pthread_exit(NULL);
	}

	//Start memory access sampling
	numap_rc = numap_sampling_read_start(&measures[i]);
	if(numap_rc < 0) {
	  fprintf(stderr, "numap_sampling_start error : %s\n", numap_error_message(numap_rc));
	  pthread_exit(NULL);
	} else {
	  fprintf(stdout, "numap_sampling_start every %" PRIu64 " events OK\n", memory_bdw_sampling_freq);
	}
  }
  
  // MANU starts memory bandwidth sampling if requested
  if (memory_bdw_sampling_freq > 0) {

	numap_rc = numap_bdw_init_measure(&mem_bdw_measure);
	if(numap_rc < 0) {
	  fprintf(stderr, "numap_init_measure error : %s\n", numap_error_message(numap_rc));
	  exit(-1);
	}
	mem_bdw_samples = malloc(sizeof(mem_bdw_sample_t *) * mem_bdw_measure.nb_nodes);
 	for(i = 0; i < mem_bdw_measure.nb_nodes; i++) { 
 	  mem_bdw_samples[i] = malloc(sizeof(mem_bdw_sample_t) * memory_bdw_sampling_freq * 100); // 100 seconds max 
 	  assert(mem_bdw_samples); 
 	} 
	
	
	nb_mem_bdw_samples = 0; 
	pthread_t memory_bdw_sampling_thread; 
	numap_rc = pthread_create(&memory_bdw_sampling_thread, NULL, &mem_bdw_sampling_routine, &memory_bdw_sampling_freq);
	  if (numap_rc != 0) { 
		fprintf(stderr, "Couldn't create memory bandwidth sampling thread\n");
	  }
  }
  // #endif

  // numap_bdw_start(&mem_bdw_measure);
  // perform the actual decoding
  video_decode_example(input_file);
	
  // stop numap mem sampling
  //numap_bdw_stop(&mem_bdw_measure);
  dump_mem_samples();
  
  return 0;
}

