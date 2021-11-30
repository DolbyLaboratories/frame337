/************************************************************************************************************
 * Copyright (c) 2021, Dolby Laboratories
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:

 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 *    and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
 *    promote products derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 ************************************************************************************************************/

/****************************************************************************
 *	File:	smpte.h
 *		Defines for AC-3 SMPTE337 conversion program
 *
 *	History:
 *		2/08/07		modified spdif.h to correctly support smpte formatting
 *					and data types
 ***************************************************************************/

/**** Platform-related modifiers ****/

/* Note that only little Endian platforms supported (Intel) */
#define LITEND 1

#ifdef WIN32
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <stdint.h>

#endif

#ifdef		UNIX
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#endif /* UNIX */

/* Enumerations */

enum {AC3=0, EAC3, DOLBYE, AC4, UNKNOWN};
enum { FPS_2398 = 1, FPS_24, FPS_25, FPS_2997, FPS_30 };
enum { BITD16, BITD20, BITD24 };		/* bit depth enumeration */
enum { WRITE, APPEND };					/* File types						*/
enum { WARNING, FATAL };				/* Error message types				*/
enum {ERR_NO_ERROR, ERR_READ_ERROR, ERR_SYNCH_ERROR, ERR_INV_SAMP_RATE, ERR_INV_DATA_RATE, ERR_BSID, ERR_EOF};


/*	Frame size equates */

#define NDATARATE		64				/* size of framesize/crcloc tables	*/
#define FRMRSRV			2000			/* value for reserve data rates		*/
#define CRCRSRV			1250			/* value for reserve data rates		*/

/**** Sampling rate equates ****/

#define NFSCOD			4
enum { FS48, FS441, FS32, FSRESERVE };
#define WTHRESH 1000000000				/* modulo value for 44.1 counters */
#define W441RSRV	44100 * 2000		/* reserved data rate */

/**** Coding mode equates ****/

#define NACMOD			8
enum { MODE11, MODE10, MODE20, MODE30, MODE21, MODE31, MODE22, MODE32 };

/**** Data packing equates ****/

/* #define NOUTWORDS		(3840/2) */		/* max # words per frame			*/
/* #define PKWRDSZ			16 */				/* packed buffer word size in bits	*/
/* #define IOBUFSZ			3072 */			/* I/O buffer size (1536*2*32)/32 */
/* #define OUTBUFSZ		1536 */			/* SMPTE output buffer size (192*2*64)/16 */

#define DD_BURST_SIZE			3072				/* 32ms = 8 blocks = 1536 frames = 3072 words */
#define DD_PLUS_BURST_SIZE		3072             /* 32ms = 8 blocks = 1536 frames = 3072 words */
#define MAX_DDE_BURST_SIZE		4096
#define DDE_BURST_SIZE_2398FPS	4004
#define DDE_BURST_SIZE_24FPS	4000
#define DDE_BURST_SIZE_25FPS	3840
#define DDE_BURST_SIZE_30FPS	3200
#define BUFWORDSIZE	3072

#define DDE_2997_REPRATE	5

static const int DDE_BURST_SIZE_2997FPS[DDE_2997_REPRATE] = {3204, 3202, 3204, 3204, 3202};

/* AC4 */
#define AC4_BURST_SIZE_2398FPS   4004
#define AC4_BURST_SIZE_24FPS	 4000
#define AC4_BURST_SIZE_25FPS	 3840
#define AC4_BURST_SIZE_30FPS	 3200
#define AC4_BURST_SIZE_4795FPS	 2002
#define AC4_BURST_SIZE_48FPS	 2000
#define AC4_BURST_SIZE_50FPS	 1920
#define AC4_BURST_SIZE_60FPS	 1600
#define AC4_BURST_SIZE_100FPS	 960
#define AC4_BURST_SIZE_120FPS	 800
#define AC4_BURST_SIZE_2343FPS	 4096

#define AC4_BURST_SIZE_2997FPS_HIGH 3204
#define AC4_BURST_SIZE_2997FPS_LOW 3202
#define AC4_BURST_SIZE_599FPS_HIGH 1602
#define AC4_BURST_SIZE_599FPS_LOW 1600
#define AC4_BURST_SIZE_11988FPS_HIGH 802
#define AC4_BURST_SIZE_11988FPS_LOW 800

static const int32_t AC4_BURST_SIZE_2997FPS[DDE_2997_REPRATE] = { AC4_BURST_SIZE_2997FPS_HIGH, AC4_BURST_SIZE_2997FPS_LOW, AC4_BURST_SIZE_2997FPS_HIGH, AC4_BURST_SIZE_2997FPS_LOW, AC4_BURST_SIZE_2997FPS_HIGH };
static const int32_t AC4_BURST_SIZE_599FPS[DDE_2997_REPRATE] = { AC4_BURST_SIZE_599FPS_HIGH, AC4_BURST_SIZE_599FPS_HIGH, AC4_BURST_SIZE_599FPS_LOW, AC4_BURST_SIZE_599FPS_HIGH, AC4_BURST_SIZE_599FPS_HIGH };
static const int32_t AC4_BURST_SIZE_11988FPS[DDE_2997_REPRATE] = { AC4_BURST_SIZE_11988FPS_LOW, AC4_BURST_SIZE_11988FPS_HIGH, AC4_BURST_SIZE_11988FPS_LOW, AC4_BURST_SIZE_11988FPS_HIGH, AC4_BURST_SIZE_11988FPS_LOW };

/* AC4 Frame Rates */
enum { AC4_FPS_2398 = 0, AC4_FPS_24, AC4_FPS_25, AC4_FPS_2997, AC4_FPS_30, AC4_FPS_4795, AC4_FPS_48, AC4_FPS_50, AC4_FPS_599, AC4_FPS_60, AC4_FPS_100, AC4_FPS_11988, AC4_FPS_120, AC4_FPS_2343 };


#define PRMBLSIZE	4
#define INITREADSIZE  3 


/**** System sync equates ****/

#define SYNC_WD				0x0b77		/* DD/DD+ packed data stream sync word	*/
#define AAC_ADTS_SYNC_WD	0xfff		/* AAC ADTS sync word */
#define AAC_LOAS_SYNC_WD	0x2b7		/* AAC LOAS sync word */
#define AC4SIMPLE_SYNC_WD0	0xac40		/* AC4 simple sync word */
#define AC4SIMPLE_SYNC_WD1	0xac41		/* AC4 simple sync word */
#define PREAMBLE_A16		0xf872		/* IEC 958 preamble a (sync word 1) */
#define PREAMBLE_A20		0x6f872		/* IEC 958 preamble a (sync word 1) */
#define PREAMBLE_A24		0x96f872	/* IEC 958 preamble a (sync word 1) */
#define PREAMBLE_B16		0x4e1f		/* IEC 958 preamble b (sync word 2) */
#define PREAMBLE_B20		0x54e1f		/* IEC 958 preamble b (sync word 2) */
#define PREAMBLE_B24		0xa54e1f	/* IEC 958 preamble b (sync word 2) */
#define PREAMBLE_C16		0x001c		/* preamble c (stream 0, 16-bit, Dolby E) */
#define PREAMBLE_C20		0x003c0		/* preamble c (stream 0, 20-bit, Dolby E) */
#define PREAMBLE_C24		0x005c00	/* preamble c (stream 0, 24-bit, Dolby E) */
#define DDE_SYNC16			0x078e		/* 16-bit Dolby E sync word */
#define DDE_SYNC16K			0x078f		/* 16-bit Dolby E sync word (with key) */
#define DDE_SYNC20			0x0788e		/* 20-bit Dolby E sync word */
#define DDE_SYNC20K			0x0788f		/* 20-bit Dolby E sync word (with key) */
#define DDE_SYNC24			0x07888e	/* 24-bit Dolby E sync word */
#define DDE_SYNC24K			0x07888f	/* 24-bit Dolby E sync word (with key) */

#define AC3_BSID_LOW		0			/* AC3 BSID */
#define AC3_BSID_HIGH		8			
#define EC3_BSID_HIGH		16			/* EC3 BSID */
#define EC3_BSID_LOW		11

#define SMPTE_DD_ID			1
#define SMPTE_DD_PLUS_ID    16
#define SMPTE_DDE_ID		28
#define SMPTE_AAC_ID		10
#define SMPTE_AACPLUS_ID	11
#define SMPTE_AC4SIMPLE_ID	24

#define MAXDDBSID  		8
#define MINDDPBSID		12
#define DDPBSID			16

/* SMPTE Preambles */

#define PREAMBLE_X_1 0xe2
#define PREAMBLE_X_0 0x1d
#define PREAMBLE_Y_1 0xe4
#define PREAMBLE_Y_0 0x1b
#define PREAMBLE_Z_1 0xe8
#define PREAMBLE_Z_0 0x17


/* Sync words */
#define TC_SYNC_WD_REV	0x1001
#define SYNC_WD_REV		0x770B
#define TC_SYNC_WD		0x0110
#define MAXFSCOD		3
#define MAXFRMSIZECOD	38
#define TC_FRMSIZE		16

#define MAXSLICE		2056

#define		BSI_BSID_STD			8					/* Standard ATSC A/52 bit-stream id	*/
#define		BSI_BSID_AXD			6					/* Annex D bit-stream id			*/
#define		BSI_BSID_AXE			16					/* Annex E bit-stream id			*/


#define	BSI_ISDDP(bsid)	((bsid) <= BSI_BSID_AXE && (bsid) > 10)
#define	BSI_ISDD(bsid)			((bsid) <= BSI_BSID_STD)
#define	BSI_ISDD_AXD(bsid)		((bsid) == BSI_BSID_AXD)
#define	BSI_ISDD_STD(bsid)		(BSI_ISDD(bsid) && !BSI_ISDD_AXD(bsid))

/* Structure definitions */

/* Timeslice */
typedef struct {
	long bytecount;
	short has_tc;
	short byte_rev;
	int fscod;
	int datarate;
	int bsid;
	int framesize;
	int frmsizecod;
	int strmtyp;
	int substreamid;
	short is_ddp;
	int numblks;	
	} SLC_INFO;

typedef struct
{
	int wavfilesize;
	int nchannels;
	int nbits;
	int sample_rate;
	int wavheadersize;

}Wave_Struct;


typedef struct 
{
	int bit_depth;
	int bytes_per_word;
	int bits_per_sample;
	FILE *smpte_file;
	FILE *ac3file;
	char *ac3fname;
	char *smpte_fname;
	int shiftbits;
	int stream_type;
	int dolbye_frame_sz;  
	int in_file_start;
	int pa_align_changes;
	int pc_value_changes;
	int pd_value_changes;
    int b_ac4_with_crc;
}File_Info;

typedef struct {
    unsigned char *p;
    size_t bytes;
    unsigned long bit_offs;
} AC4_BITREADER;


/**** User code function prototypes ****/

void show_usage(void);
void error_msg(char *msg, int errcode);
int deformat(File_Info *file_info, int verbose);
int getsync (File_Info *file_info, uint8_t *dfbuf);
uint32_t getword32value(unsigned char *buf, int bps);
void convertbuffer(unsigned char *inbuf, void *outbuf, int bps, int outwordsize, int bitcount, int bit_depth);
int parse_preamble(File_Info *file_info);
int get_dde_frame_rate(uint32_t *dde_frame, int bit_depth);
int BitUnkey(uint32_t *in_buf, int keyvalue, 	int bit_pointer, int numitems, int bit_depth);
uint32_t *BitUnp_rj(uint32_t *in_buf, int datalist[], int *bit_pointer, int numitems, int numbits, int bit_depth);
int checkstatus(int status);
int parse_header(FILE *infile, Wave_Struct *wavInfo);
short bytereverse(short in);
void print_337_info(int frame_count, const char *pa_alignment_text, int pc_value, int pd_value);
int get_timeslice(short readtype, uint16_t *inbuf, FILE *fileptr, long *numbytes, SLC_INFO *sinfo, int justinfo, int bufwords);
unsigned long ac4_bread(AC4_BITREADER *bs, unsigned long nbits);
int16_t get_ac4_data_type_dependent(int32_t burst_size, int32_t fr_idx);
int16_t get_ac4_preamble_c(int32_t burst_size, int32_t fr_idx);