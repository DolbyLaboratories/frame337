/************************************************************************************************************
 * Copyright (c) 2016, Dolby Laboratories Inc.
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
 *
 *	File:	frame337.c
 *		AC-3/EC-3/Dolby E smpte conversion program
 *
 *		This program converts encoded AC-3, EC-3 or Dolby E files to the SMPTE format
 *		complient with the SMPTE S337M and S340M standards.
 *
 *	History:
 *      12/12/16    Preparation for open source contribution
 *		11/27/14	Commented legacy code to allow Linux binary build (line 1135, char * p_strn)
 *		2/08/07		Modified spdif.c to correctly support smpte formatting and data types
 *		2/08/07		Added correct support for less than 6 blocks per frame and multiple substreams for DD+
 *		2/14/07		Added support for formatting Dolby E onto smpte
 *		2/22/07		Added support for deformatting Dolby E from smpte
 *		8/14/07		Added wave file input support for deformatting
 *		8/14/07		Added support for outputting .wav files if Dolby E input
 ****************************************************************************/

/**** Include Files ****/

#include "frame337.h"

/**** Constants ****/

extern const int16_t frmsizetab [NFSCOD] [NDATARATE];
extern const int16_t varratetab [NFSCOD] [NDATARATE];
extern const uint16_t fratetab [NFSCOD];
extern const int bitdepthtab [3];

const char *pa_alignment_text[2] = {"Left", "Right"};
const char *default_ac3fname =    "output.ac3";
const char *default_smpte_fname = "output.wav";

/**** Main function ****/

int main (int argc, char *argv [])
{
	uint16_t iobuf  [BUFWORDSIZE];			/* Holds 1 packed AC-3 frame = 8 AES blocks */
	uint32_t Eiobuf [MAX_DDE_BURST_SIZE];	/* Holds 1 packed Dolby E frame */

	/* iobuf Buffer pointers */
	uint16_t *p_buf;
	int i,j;

	SLC_INFO sinfo;
	File_Info file_info;
	long numbytes;
	short status;
	int wave_bps;
	int wave_frate;
	int outbufval;
	int done; /* looping flag */
	char *outbyteptr;
	int framesizecod;				/* size of frame */
	int sampratecod;				/* sample rate */

	int nwords;						/* # words in frame */
	long framecount = 0;			/* current frame number */
	long file_length;				/* in bytes */
	long numframes;					/* derived from file_length */
	double percent = 0.0;			/* 1 frame percent of file */
	int altformat = 0;				/* flag for 2/3 sync pt format */
	int size_23 = 0;				/* 2/3 size of current frame */
	int deformat_mode = 0;
	int nStreamNum = 0;
	int burst_size;                 /* size of IEC payload per data burst */
									/* for DD, this is in bits            */  
									/* for DDplus, this is in bytes       */
	int16_t dolbye_fps;	 			/* Dolby E fps */

	int numblocks;							/* accumulated # of blocks per 1536 AES frames */
	int lastnumblocks;						/* to catch changes in numblocks */
	int frameset;							/* indicator of complete frame set */
	int flushbuf = 0;
	int dolbye;
	int dde_frame_ctr = 0;
	unsigned int accumwords = 0;
	int no_bit_depth_specified = 0;
	int scratch_int;
	short scratch_short;
	char *in_fname = NULL;
	char *out_fname = NULL;
	char errstr[64];				/* string for error message */				
	uint16_t altbuf [BUFWORDSIZE];			/* Alternate buffer for 2/3 alignment */
	int verbose = 0;				/* print progress messages */

	Wave_Struct wavInfo;



	/* set up file info structure */
	file_info.ac3fname = (char *)default_ac3fname;
	file_info.smpte_fname = (char *)default_smpte_fname;

	/* Set up counters */
	file_info.pa_align_changes = 0;
	file_info.pc_value_changes = 0;
	file_info.pd_value_changes = 0;

	/*	Display sign-on banner */
	fprintf (stderr, "\nCopyright 2007-2016 Dolby Laboratories, Inc. and");
	fprintf (stderr, "\nDolby Laboratories Licensing Corporation. All Rights Reserved.\n");
	fprintf (stderr, "\nDolby Digital(AC-3), Dolby Digital Plus (E-AC-3), Dolby E SMPTE Conversion Program Ver 2.0.0\n\n");

	/*	Parse command line arguments */
	for (i = 1; i < argc; i++)
	{
		if (*(argv [i]) == '-')
		{
			switch (*((argv [i]) + 1))
			{
				case 'h':
				case 'H':
					show_usage ();
					break;
				case 'i':
				case 'I':
					in_fname = argv [i] + 2;
					break;
				case 'o':
				case 'O':
					out_fname = argv [i] + 2;
					break;
				case 'v':
				case 'V':
					verbose = 1;
					break;
				case 'a':
				case 'A':
					altformat = 1;
					break;
				case 'b':
				case 'B':
					file_info.bits_per_sample = atoi(((argv[i]) + 2));
					if((file_info.bits_per_sample != 16) && (file_info.bits_per_sample != 24) && (file_info.bits_per_sample != 32))
					{
						fprintf (stderr, "\nInvalid Bit Depth!\n\n");
						show_usage ();
					}
					file_info.bytes_per_word = file_info.bits_per_sample / 8;
					break;
				case 'd':
				case 'D':
					deformat_mode = 1;
					break;              
				default:
					show_usage ();
					break;
			}
		}
		else
		{
			show_usage ();
		}
	}

	if(!file_info.bits_per_sample)
	{
		no_bit_depth_specified = 1;
	}


	/*	Open i/o files */
	if (deformat_mode) {
		if (in_fname)
		{
			file_info.smpte_fname = in_fname;
		}
		else
		{
			file_info.smpte_fname = (char *)default_smpte_fname;
		}
		if (out_fname)
		{
			file_info.ac3fname = out_fname;
		}
		else
		{
			file_info.ac3fname = (char *)default_ac3fname;
		}
		
		if ((file_info.ac3file = fopen (file_info.ac3fname, "wb")) == NULL)
		{
			sprintf (errstr, "decode: Unable to create output file, %s.", file_info.ac3fname);
			error_msg (errstr, FATAL);
		}
		
		if ((file_info.smpte_file = fopen (file_info.smpte_fname, "rb")) == NULL)
		{
			fprintf (stderr, "\nFATAL ERROR: decode: Input file, %s, not found.\n\n", file_info.smpte_fname);
			show_usage ();
			//		error_msg (errstr, FATAL);	
		}

		file_info.in_file_start = ftell(file_info.smpte_file);

		if( parse_header(file_info.smpte_file, &wavInfo) )
		{
			file_info.in_file_start = ftell(file_info.smpte_file);
			// wave file
			file_info.bits_per_sample = wavInfo.nbits;			

			file_info.bytes_per_word = file_info.bits_per_sample / 8;
		}
		else
		{
			if(no_bit_depth_specified)
			{
				fprintf(stderr, "Error: Bit Depth Must Be Specified If Input is Not a Wave File\n");
				exit(0);
			}
		}
		
		fseek (file_info.smpte_file, 0, SEEK_END);
		file_length = ftell (file_info.smpte_file);
		rewind (file_info.smpte_file);
		fseek (file_info.smpte_file, wavInfo.wavheadersize, SEEK_SET);
		framecount += deformat (&file_info, verbose); // smpte_file, ac3file);
		exit (0);

	}

	else {	//	!deformat_mode, original

		if (in_fname)
		{
			file_info.ac3fname = in_fname;
		}
		else
		{
			file_info.ac3fname = (char *)default_ac3fname;
		}
		if (out_fname)
		{
			file_info.smpte_fname = out_fname;
		}
		else
		{
			file_info.smpte_fname = (char *)default_smpte_fname;
		}

		if ((file_info.ac3file = fopen (file_info.ac3fname, "rb")) == NULL)
		{
			fprintf (stderr, "\nFATAL ERROR: decode: Input file, %s, not found.\n\n", file_info.ac3fname);
			show_usage ();
			error_msg (errstr, FATAL);
		}
		
		fseek (file_info.ac3file, 0, SEEK_END);
		file_length = ftell (file_info.ac3file);
		rewind (file_info.ac3file);
		
		if ((file_info.smpte_file = fopen (file_info.smpte_fname, "wb")) == NULL)
		{
			sprintf (errstr, "decode: Unable to create output file, %s.", file_info.smpte_fname);
			error_msg (errstr, FATAL);
		}

		fseek(file_info.smpte_file, 44, SEEK_SET); // advance pointer beyond wave header size

	}		//	!deformat_mode

	done = 0;
	/*	Read frames of DD data */
	while (! done)
	{
		frameset = 0;
		numblocks = 0;
		accumwords = 0;

		dolbye = parse_preamble(&file_info);

		if(dolbye && (dolbye != SMPTE_DDE_ID))
		{ 
			done = checkstatus(dolbye);
			if(done){ break; }
		}

		if(dolbye == SMPTE_DDE_ID)
			file_info.stream_type = DOLBYE;
		else
		{
			fread(iobuf, 2, 1, file_info.ac3file);

			if(*iobuf == SYNC_WD || *iobuf == SYNC_WD_REV)
				file_info.stream_type = AC3;
			else
				file_info.stream_type = -1;

			fseek(file_info.ac3file, -2, SEEK_CUR);
		}

		if(file_info.stream_type == DOLBYE)
		{	
			wave_bps = 24;
			wave_frate = 48000;

			if(fread((void *)(&Eiobuf[0]), sizeof(int), file_info.dolbye_frame_sz + PRMBLSIZE, file_info.ac3file) 
				!= (file_info.dolbye_frame_sz + PRMBLSIZE))
			{
				error_msg("File read error", FATAL);
			}
			else
			{					
				/* verify that the sync word is correct */
				switch(file_info.bit_depth)
				{
					case BITD16:
						if(((Eiobuf[4] & 0xFFFE0000) >> 16) != DDE_SYNC16)
						{
							error_msg("Invalid Dolby E syncword.", WARNING);
						}
						break;
					case BITD20:
						if(((Eiobuf[4] & 0xFFFFE000) >> 12) != DDE_SYNC20)
						{
							error_msg("Invalid Dolby E syncword.", WARNING);
						}
						break;
					case BITD24:
						if(((Eiobuf[4] & 0xFFFFFE00) >> 8) != DDE_SYNC24)
						{
							error_msg("Invalid Dolby E syncword.", WARNING);
						}
						break;
					default:
						error_msg("Invalid preamble syncword 2.", FATAL);
						break;
				}

				//get frame rate
				dolbye_fps = get_dde_frame_rate(&Eiobuf[0], file_info.bit_depth);					
				switch(dolbye_fps)
				{
					case FPS_2398:
						burst_size = DDE_BURST_SIZE_2398FPS;							
						break;
					case FPS_24:
						burst_size = DDE_BURST_SIZE_24FPS;							
						break;
					case FPS_25:
						burst_size = DDE_BURST_SIZE_25FPS;							
						break;
					case FPS_2997:
						burst_size = DDE_BURST_SIZE_2997FPS[dde_frame_ctr % DDE_2997_REPRATE];							
						break;
					case FPS_30:
						burst_size = DDE_BURST_SIZE_30FPS;							
						break;
				}					
			}			

			memset((void *)(&Eiobuf[file_info.dolbye_frame_sz + PRMBLSIZE]), 0,
				(burst_size - (file_info.dolbye_frame_sz + PRMBLSIZE))*sizeof(int));

			outbyteptr = (char *)Eiobuf;

			for (i=0; i<burst_size; i++)
			{ 
				outbufval = Eiobuf[i];

				*outbyteptr++ = (char)(outbufval >> 8);
				*outbyteptr++ = (char)(outbufval >> 16);
				*outbyteptr++ = (char)(outbufval >> 24);
			}

			if (fwrite ((void *)Eiobuf, 3, burst_size, file_info.smpte_file) != (size_t) burst_size)
			{
				sprintf (errstr, "decode: Unable to write to output file, %s.", file_info.smpte_fname);
				error_msg (errstr, FATAL);
			}

			dde_frame_ctr++;
		}
		else if(file_info.stream_type == AC3)//DD or DD+
		{
			wave_bps = 16;

			/* Write preamble */
			iobuf[0] = (int16_t) 0x0f872;						/* IEC958_SYNCA */
			iobuf[1] = (int16_t) 0x04e1f;						/* IEC958_SYNCB */

			/* Clear rest of buffer */
			for (i = 3; i < BUFWORDSIZE; i++)
			{
				iobuf[i] = 0;
			}

			accumwords += PRMBLSIZE;

			/* Set the buffer pointer */
			p_buf = iobuf;
			p_buf += PRMBLSIZE;

			/*****************************************************/
			/* Read frames until we accumulate a full frameset   */
			/*****************************************************/
			while(!frameset && !done)
			{
				
				/* get the whole frame (and skip the timecode) */

				status = get_timeslice(2, p_buf, file_info.ac3file, &numbytes, &sinfo, 0, accumwords);

				wave_frate = fratetab[sinfo.fscod];

				if(numblocks == 0){ lastnumblocks = sinfo.numblks; }

				/* if blocks per frame changes at non-frameset boundary */
				if(sinfo.numblks != lastnumblocks)
				{
					// zero out what was just added to iobuf
					memset(p_buf, 0, sinfo.framesize*sizeof(short));	

					// rewind file ptr to beginning of frame with new bpf			
					fseek(file_info.ac3file, -sinfo.framesize * sizeof(short), SEEK_CUR);	

					break; //jump to write out partial frame set
				}


				if (sinfo.is_ddp == 0) 
				{
					file_info.stream_type = EAC3;
					iobuf[2] = (int16_t)(SMPTE_DD_ID | (nStreamNum << 13)); /* AC3BURSTINFO */
					burst_size = DD_BURST_SIZE;
				}
				else
				{
					iobuf[2] = (int16_t)(SMPTE_DD_PLUS_ID | (nStreamNum << 13)); /* EC3BURSTINFO */			    
					burst_size = DD_PLUS_BURST_SIZE;
				}		      
				if(status) 
				{
					done = checkstatus(status);
					if(numblocks > 0){ flushbuf = 1; }
				}
				else 
				{
					

					nwords = sinfo.framesize;
					framesizecod = sinfo.frmsizecod;		/* size of frame */
					sampratecod = sinfo.fscod;				/* sample rate */
					
					/* Byte Reverse */
					if(sinfo.byte_rev)
					{
						for(i = 0; i < nwords; i++)
						{
							p_buf[i] = bytereverse(p_buf[i]);
						}
					}
					
					numframes = file_length / (2 * nwords);
					percent = 100. * (1. / (double) numframes);

					if (verbose)
					{
						printf ("\nReformatting frame %ld     (%d%% done)\n", framecount,
							(int)(framecount * percent));

						if(framecount == 0)
						{
							if(sinfo.is_ddp == 0)
							{
								printf("Dolby Digital frames detected\nBlocks per frame: 6\n");
								printf("bsid: %d\n", sinfo.bsid);
							}
							else
							{
								printf("Dolby Digital Plus frames detected\nBlocks per frame: %d\n", sinfo.numblks);
								printf("bsid: %d\n", sinfo.bsid);
							}
						}
					}

					/* increment # of blocks received on independent or transcoded frames*/
					if(((sinfo.strmtyp == 0) || (sinfo.strmtyp == 2)) 
						&& (sinfo.substreamid == 0))
					{					
						numblocks += sinfo.numblks;
					}

					p_buf += nwords;
					accumwords += nwords;
					
					if(nwords + PRMBLSIZE > burst_size - 4)
					{
						error_msg ("decode: frame size too large for SMPTE format", FATAL);
					}

					if((unsigned int) accumwords > (unsigned int) burst_size - 4)
					{					
						sprintf(errstr, "decode: Accumulation of frameset data exceeds 1.536Mbps data limit");
						error_msg(errstr, FATAL);
					}

					framecount++;

					//look ahead at next frame *required to support substreams*
					status = get_timeslice(2, p_buf, file_info.ac3file, &numbytes, &sinfo, 1, 0);
					if(status)
					{
						done = checkstatus(status); // check to see if file is eof or an error occured
						if(numblocks > 0){ flushbuf = 1; }
					}
					memset(p_buf, 0, 4*sizeof(short)); // zero out 4 info words written to buffer							
					fseek(file_info.ac3file, -8, SEEK_CUR); // rewind file pointer by look ahead amount

					if((numblocks == 6) && (((sinfo.strmtyp == 0) || (sinfo.strmtyp == 2)) 
						&& (sinfo.substreamid == 0)))
					{ 
						frameset = 1; 
					}
					
				}

			}

			if(!done || flushbuf) 
			{
				iobuf[3] = (uint16_t)((accumwords - PRMBLSIZE)*16);						/* length code */
				
				/*	Write AES frame to output file */
				if (altformat)
				{
					if(sinfo.is_ddp)
					{
						error_msg("decode: Cannot use alternate packing with DD+ inputs.", FATAL);
					}

					size_23 = (int) (varratetab [sampratecod] [framesizecod]);
					for (i = 0; i < (2048 - size_23 - PRMBLSIZE); i++)
					{
						altbuf [i] = 0;
					}
					j = 0;
					for (i = (2048 - size_23 - PRMBLSIZE); i < (2048 - size_23 + nwords); i++)
					{
						altbuf [i] = iobuf [j];
						j++;
					}
					for (i = (2048 - size_23 + nwords); i < burst_size; i++)
					{
						altbuf[i] = 0;
					}
					if (verbose)
					{
						printf ("Two thirds size = %d", size_23);
					}
					if (fwrite ((void *)altbuf, 2, burst_size, file_info.smpte_file) != (size_t) burst_size)
					{
						sprintf (errstr, "decode: Unable to write to output file, %s.", file_info.smpte_fname);
						error_msg (errstr, FATAL);
					}
				}
				else
				{
					if (fwrite ((void *)iobuf, 2, burst_size, file_info.smpte_file) != (size_t) burst_size)
					{
						sprintf (errstr, "decode: Unable to write to output file, %s.", file_info.smpte_fname);
						error_msg (errstr, FATAL);
					}
				}

			}
		} /* if ac3 or eac3 */
		else
		{
			sprintf (errstr,"input file type not recognized");
			error_msg (errstr, FATAL);	
		}
	}	/* while */

	// Write wave header to output file
	
	fseek(file_info.smpte_file, 0, SEEK_END);	
	file_length = ftell (file_info.smpte_file);	
	rewind(file_info.smpte_file);	
	
	fwrite("RIFF", 4, 1, file_info.smpte_file);
	scratch_int = file_length - 8;
	fwrite(&scratch_int, 4, 1, file_info.smpte_file); //file length
	fwrite("WAVE", 4, 1, file_info.smpte_file);
	fwrite("fmt ", 4, 1, file_info.smpte_file);
	scratch_int = 16;
	fwrite(&scratch_int, 4, 1, file_info.smpte_file);	//format length
	scratch_short = 1;
	fwrite(&scratch_short, 2, 1, file_info.smpte_file); //format tab
	scratch_short = 2;
	fwrite(&scratch_short, 2, 1, file_info.smpte_file); //channels
	fwrite(&wave_frate, 4, 1, file_info.smpte_file);  //sample rate
	scratch_int = wave_frate * 2 * (wave_bps / 8);
	fwrite(&scratch_int, 4, 1, file_info.smpte_file);  // avg bytes per sec
	scratch_short = 2 * (wave_bps / 8);
	fwrite(&scratch_short, 2, 1, file_info.smpte_file);  // block align
	fwrite(&wave_bps, 2, 1, file_info.smpte_file);  // bits per sample
	fwrite("data", 4, 1, file_info.smpte_file);  // data size
	scratch_int = file_length - 44;
	fwrite(&scratch_int, 4, 1, file_info.smpte_file);

/*	Close i/o files */

	if (fclose (file_info.smpte_file))
	{
		sprintf (errstr,"decode: Unable to close output file, %s.", file_info.smpte_fname);
		error_msg (errstr, FATAL);
	}

	if (fclose (file_info.ac3file))
	{
		sprintf(errstr, "decode: Unable to close input file, %s.", file_info.ac3fname);
		error_msg (errstr, FATAL);
	}

	return(0);
} //  main


void show_usage (void)
{
	puts(
		"Usage: frame337 [-h][-i<filename.ext>][-o<filename.ext>][-a][-b][-v][-d][-n<#>]\n"
		"       -h     Show this usage message and abort\n"
		"       -i     Input AC-3, E-AC-3, or Dolby E file name \n"
		"              (default output.ac3) (or .smp if deformat)\n"
		"       -o     Output SMPTE337 file name (default output.wav) (or .ext if deformat)\n"
		"       -a     Use alternate packing method - aligns 2/3 pt \n"
		"              (not valid for E-AC-3 or Dolby E files)\n"
		"       -b     Bits per sample of input file. Default = 16. \n"
		"              (only used for deformatting)\n"
		"       -v     Verbose mode. \n"
		"              Display frame # and % done (SMPTE 337M status if deformat)\n"
		"       -d     Deformat. Output AC-3, E-AC-3 or Dolby E file\n"
		"              from SMPTE file\n"
	);
	exit(1);
}

void error_msg (char *msg, int errcode)
{
	if (errcode == FATAL)
	{
		fprintf(stderr, "\nFATAL ERROR: %s\n", msg);
		exit (errcode);
	}
	else
	{
		fprintf(stderr, "\nWARNING: %s\n", msg);
	}
}


int getsync (File_Info *file_info, uint8_t *dfbuf) {

	uint32_t word32val;

	while (fread (dfbuf, file_info->bytes_per_word, 1, file_info->smpte_file)) {
		// look for preamble A
		word32val = getword32value(dfbuf, file_info->bits_per_sample);		

		if (((word32val >> file_info->shiftbits) & 0x0000FFFF) == (uint32_t)PREAMBLE_A16)
			{ file_info->bit_depth = 16; break;}
		if (((word32val >> (file_info->shiftbits - 4)) & 0x000FFFFF) == (uint32_t)PREAMBLE_A20)
			{ file_info->bit_depth = 20; break;}
		if (((word32val >> (file_info->shiftbits - 8)) & 0x00FFFFFF) == (uint32_t)PREAMBLE_A24)
			{ file_info->bit_depth = 24; break;}

		if (feof (file_info->smpte_file)) return 0;
	}

	if (feof (file_info->smpte_file)) return 0;
	fread (dfbuf, file_info->bytes_per_word, 1, file_info->smpte_file);

	// look for preamble B
	word32val = getword32value(dfbuf, file_info->bits_per_sample);

	if ((((word32val >> file_info->shiftbits) & 0x0000FFFF) != (uint32_t)PREAMBLE_B16) && (file_info->bit_depth == 16)) 
		return getsync (file_info, dfbuf);
	if ((((word32val >> (file_info->shiftbits - 4)) & 0x000FFFFF) != (uint32_t)PREAMBLE_B20) && (file_info->bit_depth == 20)) 
		return getsync (file_info, dfbuf);
	if ((((word32val >> (file_info->shiftbits - 8)) & 0x00FFFFFF) != (uint32_t)PREAMBLE_B24) && (file_info->bit_depth == 24)) 
		return getsync (file_info, dfbuf);

	fread (dfbuf, file_info->bits_per_sample / 8, 3, file_info->smpte_file);
	if (feof (file_info->smpte_file)) return 0;

	// check value of preamble C
	word32val = getword32value(dfbuf, file_info->bits_per_sample);

	if(((word32val >> file_info->shiftbits) & 0x0000001F) == SMPTE_DD_PLUS_ID)
		file_info->stream_type = EAC3;
	else if(((word32val >> file_info->shiftbits) & 0x0000001F) == SMPTE_DD_ID)
		file_info->stream_type = AC3;
	else if(((word32val >> file_info->shiftbits) & 0x0000001F) == SMPTE_DDE_ID)
		file_info->stream_type = DOLBYE;
	else
		file_info->stream_type = UNKNOWN;

	if (file_info->stream_type == AC3 || file_info->stream_type == EAC3)
	{
		word32val = getword32value((unsigned char *)dfbuf + (file_info->bytes_per_word * 2), file_info->bits_per_sample);

		// check if the DD/DD+ frame sync word is valid
		if((((word32val >> file_info->shiftbits) & 0x0000FFFF) != (uint32_t)SYNC_WD) && 
			(((word32val >> file_info->shiftbits) & 0x0000FFFF) != (uint32_t)SYNC_WD_REV))
		{
			error_msg("Invalid Sync Word Found", FATAL);
		}

		fseek(file_info->smpte_file, -file_info->bytes_per_word, SEEK_CUR);

		// calculate the burst size
		word32val = getword32value((unsigned char *)dfbuf + file_info->bytes_per_word, file_info->bits_per_sample);
		word32val = (word32val >> (file_info->bits_per_sample - file_info->bit_depth));

		return (int) word32val;
	}
	else if(file_info->stream_type == DOLBYE)
	{
		word32val = getword32value((unsigned char *)dfbuf + (file_info->bytes_per_word * 2), file_info->bits_per_sample);

		// check if the Dolby E frame sync word is valid
		if(((((word32val >> file_info->shiftbits) & 0x0000FFFE) != (uint32_t)DDE_SYNC16) && (file_info->bit_depth == 16))
		  || ((((word32val >> (file_info->shiftbits - 4)) & 0x000FFFFE) != (uint32_t)DDE_SYNC20) && (file_info->bit_depth == 20))
		  || ((((word32val >> (file_info->shiftbits - 8)) & 0x00FFFFFE) != (uint32_t)DDE_SYNC24) && (file_info->bit_depth == 24)))
		{
			error_msg("Invalid Sync Word Found", WARNING);
		}

		fseek(file_info->smpte_file, -(file_info->bytes_per_word * 5), SEEK_CUR);

		// calculate the burst size
		word32val = getword32value((unsigned char *)dfbuf + file_info->bytes_per_word, file_info->bits_per_sample);
		word32val = ((uint32_t)word32val >> (file_info->bits_per_sample - file_info->bit_depth));

		//return the DDE burst size + (4*bps); //+4 to compensate for the IEC header words
		return (int)(word32val + (4 * file_info->bit_depth));
	}
	else // unknown data
	{
		fseek(file_info->smpte_file, -file_info->bytes_per_word, SEEK_CUR);

		// calculate the burst size
		word32val = getword32value((unsigned char *)dfbuf + file_info->bytes_per_word, file_info->bits_per_sample);
		word32val = (word32val >> (file_info->bits_per_sample - file_info->bit_depth));

		return (int) word32val;
	}
}


uint32_t getword32value( unsigned char *buf,/* IN: pointer to 4 bytes */
						 int bps)			/* IN: bits per sample */
											/* OUT: left aligned value */
{
	if(bps == 16){ return (uint32_t)*(uint16_t *)buf; }
	else if(bps == 24){ return( buf[0] + (buf[1] << 8) + (buf[2] << 16)); }
	else if(bps == 32){ return *(uint32_t *)buf; }
	else{ return 0; }
}

/* deformat from file pointer */
int deformat (File_Info *file_info, int verbose)
{
	uint32_t dde_temp_buf [MAX_DDE_BURST_SIZE];	/* Holds 1 packed Dolby E frame */
	uint16_t *dd_temp_buf = (uint16_t *)&dde_temp_buf[0];
	uint8_t dfbuf  [MAX_DDE_BURST_SIZE	/* Deformat Buffer */
			* sizeof(uint32_t)];
	static int last_file_loc;
	static int prev_pc_value, prev_pd_value;
	int spacing_temp;
	int remaining_bytes;
	int nreadbytes;
	int nbits;						/* # of bits in SMPTE payload */
	int i,j;
	int num_frames = 0;
	uint16_t *shortbuf;

	int pa_first = 0;
	int pa_spacing, pa_spacing_sum = 0, preamble_count = 0;
	int pa_max, pa_min;
	int pa_alignment;
	double pa_spacing_average;
	int pc_value, pd_value;
	char errstr[64];				/* string for error message */				


#ifdef LITEND
	int k;
#endif

	if(file_info->bits_per_sample == 16){ file_info->shiftbits = 0; }
	else if(file_info->bits_per_sample == 24){ file_info->shiftbits = 8; }
	else if(file_info->bits_per_sample == 32){ file_info->shiftbits = 16; }
	else { error_msg ("Unknown bit depth", FATAL); }

/*	Read frames of AC-3, EC-3, or Dolby E data */

		while ((nbits = getsync (file_info, dfbuf)))
		{
			/* SMPTE formatting statistics */
			switch(file_info->bit_depth)
			{
				case 16:
					pc_value = (getword32value(dfbuf, file_info->bits_per_sample) >> file_info->shiftbits) & 0x0000ffff;
					pd_value = (getword32value((unsigned char *)dfbuf + file_info->bytes_per_word, 
						file_info->bits_per_sample) >> file_info->shiftbits) & 0x0000ffff;
					break;
				case 20:
					pc_value = (getword32value(dfbuf, file_info->bits_per_sample) >> (file_info->shiftbits - 4)) & 0x000fffff;
					pd_value = (getword32value((unsigned char *)dfbuf + file_info->bytes_per_word, 
						file_info->bits_per_sample) >> (file_info->shiftbits - 4)) & 0x000fffff;
					break;
				case 24:
					pc_value = (getword32value(dfbuf, file_info->bits_per_sample) >> (file_info->shiftbits - 8)) & 0x00ffffff;
					pd_value = (getword32value((unsigned char *)dfbuf + file_info->bytes_per_word, 
						file_info->bits_per_sample) >> (file_info->shiftbits - 8)) & 0x00ffffff;
					break;
			}	

			if(preamble_count)
			{	
				spacing_temp = (ftell(file_info->smpte_file) - last_file_loc) / file_info->bytes_per_word;

				if((spacing_temp % 2) || (pc_value != prev_pc_value) || (pd_value != prev_pd_value))
				{
					if(spacing_temp % 2)
						file_info->pa_align_changes++;
					if(pc_value != prev_pc_value)
						file_info->pc_value_changes++;
					if(pd_value != prev_pd_value)
						file_info->pd_value_changes++;

					pa_alignment = ((ftell(file_info->smpte_file) - file_info->in_file_start) / file_info->bytes_per_word) % 2;					
					
					if(verbose)
						print_337_info(preamble_count, pa_alignment_text[pa_alignment], pc_value, pd_value);
				}

				pa_spacing = ((ftell(file_info->smpte_file) - last_file_loc) / file_info->bytes_per_word) / 2;
				pa_spacing_sum += pa_spacing;

				if(preamble_count == 1)
				{
					pa_max = pa_spacing;
					pa_min = pa_spacing;
				}

				if(pa_spacing > pa_max)
					pa_max = pa_spacing;
				if(pa_spacing < pa_min)
					pa_min = pa_spacing;
			}
			else
			{
				pa_alignment = ((ftell(file_info->smpte_file) - file_info->in_file_start) / file_info->bytes_per_word) % 2;
				
				if(verbose)
					print_337_info(preamble_count, pa_alignment_text[pa_alignment], pc_value, pd_value);			

				pa_first = (((ftell(file_info->smpte_file) - file_info->in_file_start) / file_info->bytes_per_word) - PRMBLSIZE) / 2;
			}

			preamble_count++;
			pa_spacing_average = (double)pa_spacing_sum / (double)(preamble_count - 1);

			prev_pc_value = pc_value;
			prev_pd_value = pd_value;

			last_file_loc = ftell(file_info->smpte_file);
			/*------------------------------*/

			nreadbytes = ((nbits * file_info->bits_per_sample) / file_info->bit_depth) / 8;
			if(nbits % file_info->bit_depth)
				nreadbytes += (file_info->bit_depth / 8);

			if(nreadbytes == fread(dfbuf, 1, nreadbytes, file_info->smpte_file)) 
			{

#ifdef LITEND

/*	Byte swap the buffer words on little-endian machines */

				if(file_info->bits_per_sample == 16)
				{
					shortbuf = (uint16_t *)dfbuf;

					for (i = 0; i < (nbits/16); i ++)
					{
						j = shortbuf [i];
						k = ( j >> 8 ) & 0x00ff;
						j = ( j << 8 ) & 0xff00;
						shortbuf [i] = (uint16_t)(j | k);
					}
				}
#endif /* LITEND */
				// convert buffer //
				if(file_info->stream_type == DOLBYE)
				{
					convertbuffer(dfbuf, (void *)dde_temp_buf, file_info->bits_per_sample, 32, nbits, file_info->bit_depth);

					if(fwrite((void *)dde_temp_buf, 4, (nbits / file_info->bit_depth), file_info->ac3file) 
						!= (size_t) (nbits / file_info->bit_depth))
					{
						sprintf (errstr, "decode: Unable to write to output file, %s.", file_info->ac3fname);
						error_msg (errstr, FATAL);
					}
				}
				else if(file_info->stream_type == AC3 || file_info->stream_type == EAC3) //DD, DD+
				{
					convertbuffer(dfbuf, (void *)dd_temp_buf, file_info->bits_per_sample, 16, nbits, file_info->bit_depth);
					if(fwrite((void *)dd_temp_buf, 1, (nbits / 8), file_info->ac3file) != (size_t) (nbits / 8))
					{
							sprintf (errstr, "decode: Unable to write to output file, %s.", file_info->ac3fname);
							error_msg (errstr, FATAL);
					}	
				}
				// ensure we're aligned to a word boundary
				// to begin searching for next SMPTE preamble
				if((remaining_bytes = (nreadbytes % file_info->bytes_per_word)))
					fseek(file_info->smpte_file, (file_info->bytes_per_word - remaining_bytes), SEEK_CUR);

				// clear the deformat buffer to remove
				// any stale data
				memset(dfbuf, 0, sizeof(dfbuf));
			}	
			num_frames++;
		}	/* while */

/*	Close i/o files */

	if (fclose (file_info->smpte_file))
	{
		sprintf (errstr, "decode: Unable to close input file, %s.", file_info->smpte_fname);
		error_msg (errstr, FATAL);
	}

	if (fclose (file_info->ac3file))
	{
		sprintf (errstr, "decode: Unable to close output file, %s.", file_info->ac3fname);
		error_msg (errstr, FATAL);
	}

	if(verbose)
	{
		printf("SMPTE 337M Statistics:\n");
		printf("-----------------------\n");
		printf("Initial Pa Offset = %i\n", pa_first);
		printf("Pa Spacing Average = %4.2f\n", pa_spacing_average);
		printf("Pa Spacing Maximum = %i\n", pa_max);
		printf("Pa Spacing Minimum = %i\n", pa_min);
		printf("Pa Alignment Changes = %i\n", file_info->pa_align_changes);
		printf("Pc Value Changes = %i\n", file_info->pc_value_changes);
		printf("Pd Value Changes = %i\n", file_info->pd_value_changes);
	}

	return(num_frames);
}		//		deformat ()


void print_337_info(int frame_count, const char *pa_alignment_text, int pc_value, int pd_value)
{
	printf("-------------------------\n");
	printf("Frame %i:\n", frame_count);
	printf("Pa Alignment = %s\n", pa_alignment_text);
	printf("Pc Value = %#010x\n", pc_value);
	printf("Pd Value = %#010x\n", pd_value);
	printf("-------------------------\n\n");
}

/* Function to format words into elementary stream layout when deformatting */

void convertbuffer(unsigned char *inbuf, /* IN: input buffer */
				   void *outbuf,		 /* OUT: output buffer */
				   int bps,				 /* IN: bits per sample */
				   int outwordsize,		 /* IN: output word size, 16,24 or 32 */
				   int bitcount,		 /* IN: number of bits to conver */
				   int bit_depth)		 /* IN: bit depth */
{
	short *dd_buf;
	uint32_t *de_buf;
	int wordcount = bitcount / bit_depth;
	int i;

	if(bitcount % bit_depth)
		wordcount++;

	if(outwordsize == 16)
	{ 
		dd_buf = (short *)outbuf;
		if(bps == 16)
		{
			for(i = 0; i < wordcount; i++)
			{
				dd_buf[i] = *(((short *)inbuf) + i);
			}	
		}
		if(bps == 24)
		{
			for(i = 0; i < wordcount; i++)
			{
				dd_buf[i] = inbuf[(i * 3) + 1] + (inbuf[(i * 3) + 2] << 8);
			}
		}
		if(bps == 32)
		{			
			de_buf = (uint32_t *)inbuf;
			for(i = 0; i < wordcount; i++)
			{
				dd_buf[i] = (short)(de_buf[i] >> 16);
			}
		}
	}
	if(outwordsize == 32)
	{ 
		de_buf = (uint32_t *)outbuf;
		if(bps == 16)
		{
			dd_buf = (short *)inbuf;
			for(i = 0; i < wordcount; i++)
			{
				de_buf[i] = (uint32_t)(dd_buf[i] << 16);
			}	
		}
		if(bps == 24)
		{
			for(i = 0; i < wordcount; i++)
			{
				de_buf[i] = (inbuf[i * 3] << 8) + (inbuf[(i * 3) + 1] << 16)
					+ (inbuf[(i * 3) + 2] << 24);
			}
		}
		if(bps == 32)
		{
			for(i = 0; i < wordcount; i++)
			{
				de_buf[i] = *(((uint32_t *)inbuf) + i);
			}	
		}
	}
}		// convertbuffer

/* Function to generate error message from error code */
int checkstatus(int status) 
{
	if(status) 
	{
		switch(status) 
		{
			case ERR_READ_ERROR: 
				error_msg("File read error", FATAL);	
				break;
			case ERR_SYNCH_ERROR:
				error_msg("Sync word not found", FATAL);
				break;
			case ERR_INV_SAMP_RATE:
				error_msg("Invalid sample rate", FATAL);
				break;
			case ERR_INV_DATA_RATE: 
				error_msg("Invalid data rate", FATAL);
				break;
			case ERR_BSID:
				error_msg("Invalid bitstream ID", FATAL);
				break;
			case ERR_EOF:
				return(1);
		}
	}
	else
	{
		return(0);
	}

	return(1);
}		//		checkstatus()

/* Read frame to be formatting from elementary stream file */
int get_timeslice(short readtype,    /* IN: */
				  uint16_t *inbuf,	 /* IN/OUT: inpute frame buffer to written to, used even if justinfo=1 */
				  FILE *fileptr,     /* IN/OUT: Pointer to input elementary stream file*/
				  long *numbytes,    /* OUT: number of bytes read*/
				  SLC_INFO *sinfo,   /* OUT: Time slice info structure */
				  int justinfo,    	 /* IN: return just want the slide info (1) or the frame as well (0)*/
				  int bufwords  	 /* IN: Indicates the maximum number of words to be written */
				  )
{
	
	uint16_t *p_buf;
	uint16_t *p_frame;
	short syncword;
	short tc = 0;
	short tcread = 0;
	short ddread = 0;
	short ddinfo[4];
	short i;
	int numblkscod;

	sinfo->has_tc = 0;
	sinfo->byte_rev = 0;
	sinfo->bytecount = 0;

	p_buf = inbuf;
	
	while(1)
	{
		/*******************/
		/* Read first word */
		/*******************/
		if (fread((void *)(&syncword), sizeof(short), 1, fileptr) != 1)
		{
			if(feof(fileptr)) /* end of file reached */
			{
				if(ddread || tcread)
				{
					return(ERR_NO_ERROR);
				}
				else
				{
					return(ERR_EOF);
				}
			}
			else
			{
				return(ERR_READ_ERROR);
			}
		}

		if(fseek(fileptr, -2, SEEK_CUR) != 0) /* reset fileptr to beginning of timeslice */
		{
			return(ERR_READ_ERROR);
		}
		
		/***************************/
		/* Determine type of frame */
		/***************************/
		switch (syncword)
		{
		case TC_SYNC_WD:
			sinfo->has_tc = 1;
			tc = 1;
			break;
		case TC_SYNC_WD_REV:
			sinfo->has_tc = 1;	
			tc = 1;
			break;
		case SYNC_WD:
			tc = 0;
			break;
		case SYNC_WD_REV:
			tc = 0;
			break;
		default:			
			return(ERR_SYNCH_ERROR);
			break;
		}

		/******************/
		/* Parse timecode */
		/******************/
		if(tc) 
		{
			/* If already read a TC frame or DD/DD+ frame, and detect another, must be a new timeslice */
			if(tcread || ddread)
			{
				break;
			}

			/* Output TC frame */
			if((readtype == 0) || (readtype == 1)) 
			{
				if (fread((void *)(p_buf), sizeof(char), TC_FRMSIZE,fileptr) 
					!= TC_FRMSIZE)
				{
					return(ERR_READ_ERROR);
				}
				p_buf += 8;
				*numbytes += TC_FRMSIZE;
			}
			/* Skip TC frame */
			else 
			{
				fseek(fileptr, TC_FRMSIZE, SEEK_CUR);
			}

			sinfo->bytecount += TC_FRMSIZE;
			tcread = 1;
			
		}
		/**********************/
		/* Parse DD/DD+ Frame */
		/**********************/
		else
		{
			/* If already read a DD/DD+ frame, and detect another, must be a new timeslice */
			if(ddread)
			{
				break;
			}

			/* Read words 1-4 of DD/DD+ frame */
			if (fread((void *)(p_buf), sizeof(short), 4, fileptr) != 4)
			{
				return(ERR_READ_ERROR);
			}
			p_frame = p_buf;
			p_buf += 4;
			
			if(p_frame[0] == SYNC_WD_REV)
			{
				sinfo->byte_rev = 1;
			}
			else
			{
				sinfo->byte_rev = 0;
			}

			/* Handle byte reversal */
			for(i = 0; i < 4; i++)
			{
				if(sinfo->byte_rev)
				{
					ddinfo[i] = bytereverse(p_frame[i]);
				}
				else
				{
					ddinfo[i] = p_frame[i];
				}
			}
			
			sinfo->bsid  = (int)((ddinfo[2] >> 3) & 0x001F);
			
			if (BSI_ISDD(sinfo->bsid)) /* Dolby Digital */
			{
				sinfo->is_ddp = 0;
				sinfo->fscod = (int)((ddinfo[2] >> 14) & 0x0003);
				
				if (sinfo->fscod > MAXFSCOD)
				{
					return(ERR_INV_SAMP_RATE);
				}
				
				sinfo->frmsizecod = (int)((ddinfo[2] >> 8) & 0x003f);
				
				if (sinfo->frmsizecod > MAXFRMSIZECOD)
				{
					return(ERR_INV_DATA_RATE);
				}
				
				sinfo->datarate = sinfo->frmsizecod >> 1;
				sinfo->framesize = frmsizetab[sinfo->fscod][sinfo->frmsizecod];
				sinfo->numblks = 6; /* DD always has 6 frames per block */
				sinfo->strmtyp = 0; /* DD only has one stream type */
				sinfo->substreamid = 0; /* DD does not support substreams */
			}
			else if (BSI_ISDDP(sinfo->bsid)) /* DD+ */
			{
				sinfo->is_ddp = 1;
				sinfo->fscod = (int)((ddinfo[2] >> 14) & 0x0003);
				
				if (sinfo->fscod > MAXFSCOD)
				{
					return(ERR_INV_SAMP_RATE);
				}
				
				sinfo->framesize = (int)(ddinfo[1] & 0x07FF) + 1;
	
				if(sinfo->fscod == 3)
				{					
					sinfo->numblks = 6;
				}
				else
				{
					numblkscod = (int)((ddinfo[2] >> 12) & 0x0003);
					
					switch(numblkscod) 
					{
					case 0:
					case 1:
					case 2:
						sinfo->numblks = numblkscod + 1;
						break;
					case 3:
						sinfo->numblks = 6;
						break;
					default:
						error_msg("Invalid numblkscod", FATAL);
					}
				}

				sinfo->strmtyp = (int)((ddinfo[1] >> 14) & 0x0003);		
				sinfo->substreamid = (int)((ddinfo[1] >> 11) & 0x0007);

			}
			else	
			{
				return(ERR_BSID);
			}
			
			if(!justinfo)
			{
				if(readtype == 0 || readtype == 2)
				{
					if((sinfo->framesize + bufwords) > BUFWORDSIZE)
					{
						return 0; //break out of here and let main function catch error
					}

					/* Read rest of frame */
					if (fread((void *)(p_buf), sizeof(short), sinfo->framesize - 4,
						fileptr) != (size_t)(sinfo->framesize - 4))
					{
						return(1);
					}
					
					*numbytes += (sinfo->framesize * sizeof(short));
					p_buf += sinfo->framesize - 4;
				}
				else
				{
					fseek(fileptr, (sinfo->framesize - 4) * 2, SEEK_CUR);
				}
				
				sinfo->bytecount += (sinfo->framesize * sizeof(short));
				ddread = 1;
			}
			else
			{
				break;
			}

		}
	
	}

	return(0);
}

/* This function reads a wave file header for deformatting */
int parse_header(FILE *infile, Wave_Struct *wavInfo)
{
	char headerbytes[5] = "";
	int subchunk_size;

	if (infile != NULL)
	{		
		unsigned char status = 0;

		fread(headerbytes, 4, 1, infile);

		if (!strcmp(headerbytes, "RIFF"))
		{  //We got a RIFF
			status = status | 0x01;

			////////////////
			// Wave File
			////////////////
			fread(&subchunk_size, 4, 1, infile);	// read in size of RIFF chunk

			fread(headerbytes, 4, 1, infile);
			if (!strcmp(headerbytes, "WAVE"))		// if WAVE, continue 
				status = status | 0x02;					
			else								// else, PCM, exit 
				return 0;

			while(1)
			{
				if(fread(headerbytes, 1, 4, infile) != 4)	// read next subchunk ID 
					break;
				if(fread(&subchunk_size, 1, 4, infile) != 4)	// read subchunk size 
					break;

				if (!strcmp(headerbytes, "fmt ")) 
				{
					status = status | 0x04;

					if(subchunk_size < 16)
						return 1;

					fseek(infile, 2, SEEK_CUR);

					fread(&wavInfo->nchannels, 2, 1, infile);

					fread(&wavInfo->sample_rate, 4, 1, infile);

					fseek(infile, 6, SEEK_CUR);

					fread(&wavInfo->nbits, 2, 1, infile);

					if(subchunk_size > 16)	/* if necessary, advance beyond remaining subchunk bytes */
						fseek(infile, subchunk_size - 16, SEEK_CUR);

					if(wavInfo->nchannels != 2)
					{
						fprintf(stderr, "Error: Wave File Must be 2-Channels!\n");
						exit(0);
					}

					if((wavInfo->nbits%8 != 0) || (wavInfo->nbits > 32) || (wavInfo->nbits < 16))
					{
						fprintf(stderr, "Error: Only 16, 24, & 32 bit wave files are supported\n");
						exit(0);
					}				
				}
				else if (!strcmp(headerbytes,"data")) 
				{
					status = status | 0x08;

					wavInfo->wavfilesize = subchunk_size;
					
					wavInfo->wavheadersize = ftell(infile);
					wavInfo->wavfilesize += wavInfo->wavheadersize;

					break;					
				}
				else
				{
					// Advance beyond unsupported subchunks
					fseek(infile, subchunk_size, SEEK_CUR);
				}
			}
			if(status == 0xF)
				return(1);
			else
				return(0);

		}		
		else
		{
			// Assume a PCM file
			return(0);
		}
	}

	return(0);
}

short bytereverse(short in)
{

	short out;
	out = 0;

	out = out | ((in & (0x00FF)) << 8);
	out = out | ((in & (0xFF00)) >> 8);

	return out;

}

/* function to parse 337 header when deformatting, detects Dolby E*/
int parse_preamble(File_Info *file_info) /* rewinds file pointer */
/* returns SMPTE_DDE_ID if Dolby E, returns 0 or an error code otherwise */
{
	int iecsync[4];

	if (fread((void *)(&iecsync[0]), sizeof(int), 4, file_info->ac3file) != 4)
	{
		if(feof(file_info->ac3file)) /* end of file reached */
		{
			return(ERR_EOF);
		}
		else
		{
			return(ERR_READ_ERROR);
		}
	}
	/* extract preamble syncword 1 */
	/* mask off the most significant 16 bits of the sync word */
	if(((iecsync[0] & 0xFFFF0000) >> 16) == PREAMBLE_A16)
	{
		file_info->bit_depth = BITD16;		/* frame bit depth code = 0 (16 bits) */
	}
	/* mask off the most significant 20 bits of the sync word */
	else if (((iecsync[0] & 0xFFFFF000) >> 12) == PREAMBLE_A20)
	{
		file_info->bit_depth = BITD20;		/* frame bit depth code = 1 (20 bits) */
	}
	/* mask off the most significant 24 bits of the first sync word */
	else if (((iecsync[0] & 0xFFFFFF00) >> 8) == PREAMBLE_A24)
	{
		file_info->bit_depth = BITD24;		/* frame bit depth code = 2 (24 bits) */
	}
	else
	{
		fseek(file_info->ac3file, -16, SEEK_CUR);
		return(0);
	}


	/* extract preamble syncword 2 */
	switch(file_info->bit_depth)
	{
		case BITD16:
			if(((iecsync[1] & 0xFFFF0000) >> 16) != PREAMBLE_B16)
			{
				fseek(file_info->ac3file, -16, SEEK_CUR);
				return(0);
			}
			break;

		case BITD20:
			if(((iecsync[1] & 0xFFFFF000) >> 12) != PREAMBLE_B20)
			{
				fseek(file_info->ac3file, -16, SEEK_CUR);
				return(0);
			}
			break;

		case BITD24:
			if(((iecsync[1] & 0xFFFFFF00) >> 8) != PREAMBLE_B24)
			{
				fseek(file_info->ac3file, -16, SEEK_CUR);
				return(0);
			}
			break;

		default:
			fseek(file_info->ac3file, -16, SEEK_CUR);
			return(0);
			break;
	}

	/* extract preamble burst_info. find data type, error flag */
	switch(file_info->bit_depth)
	{
		case BITD16:
			if(((iecsync[2] & 0xFFFF0000) >> 16) != PREAMBLE_C16)	/* stream 0, 16-bit, Dolby E */
			{
				fseek(file_info->ac3file, -16, SEEK_CUR);
				return(0);
			}
			break;

		case BITD20:
			if(((iecsync[2] & 0xFFFFF000) >> 12) != PREAMBLE_C20)	/* stream 0, 20-bit, Dolby E */
			{
				fseek(file_info->ac3file, -16, SEEK_CUR);
				return(0);
			}
			break;

		case BITD24:
			if(((iecsync[2] & 0xFFFFFF00) >> 8) != PREAMBLE_C24)	/* stream 0, 24-bit, Dolby E */
			{
				fseek(file_info->ac3file, -16, SEEK_CUR);
				return(0);
			}
			break;

		default:
			fseek(file_info->ac3file, -16, SEEK_CUR);
			return(0);
			break;
	}

	/* extract preamble length_code */
	/* length_code is left-justified and in bits. convert to rj words (16, 20 ,or 24-bit) . */
	switch(file_info->bit_depth)
	{
		case BITD16:
			file_info->dolbye_frame_sz = ((iecsync[3] & 0xffff0000) >> 16) / 16;
			break;

		case BITD20:
			file_info->dolbye_frame_sz = (iecsync[3] >> 12) / 20;
			break;

		case BITD24:
			file_info->dolbye_frame_sz = (iecsync[3] >> 8) / 24;
			break;	
	}

	/* return file pointer by bytes read */
	fseek(file_info->ac3file, -16, SEEK_CUR);
	return(SMPTE_DDE_ID);
}

int get_dde_frame_rate(	uint32_t *dde_frame,	/* IN: Input buffer */
						 int bit_depth)			/* IN: bit depth (16, 20 or 24) */
{
	int value, preamble[4], key_present, metadata_key;
	uint32_t	  Epbuf  [MAX_DDE_BURST_SIZE];  /* Modifiable E buffer for frame parsing */
	int BitPtr;
	uint32_t *BufPtr;					/* Pointer used to transverse input frame */

	// copy Dolby E buffer to modifyable buffer due to possible 
	// unkeying to get frame_rate_code
	memcpy(Epbuf, dde_frame, MAX_DDE_BURST_SIZE*sizeof(uint32_t));

	BufPtr = Epbuf;				/* reset data buffer pointer */
	BitPtr = 0;					/* reset current bit pointer */

	BufPtr = BitUnp_rj(BufPtr, &preamble[0], &BitPtr, 4, bitdepthtab[bit_depth], bit_depth); // read in preamble
	BufPtr = BitUnp_rj(BufPtr, &value, &BitPtr, 1, bitdepthtab[bit_depth] - 1, bit_depth); // sync word
	BufPtr = BitUnp_rj(BufPtr, &value, &BitPtr, 1, 1, bit_depth); // key_present

	key_present = value;
	if(key_present)
	{ 
		BufPtr = BitUnp_rj(BufPtr, &metadata_key, &BitPtr, 1, bitdepthtab[bit_depth], bit_depth); // metadata_key
		BitUnkey(BufPtr, metadata_key, BitPtr, 1, bit_depth);
	}

	BufPtr = BitUnp_rj(BufPtr, &value, &BitPtr, 1, 4, bit_depth); // metadata_revision_id
	BufPtr = BitUnp_rj(BufPtr, &value, &BitPtr, 1, 10, bit_depth); // metadata_segment_size

	if(key_present){ BitUnkey(BufPtr, metadata_key, BitPtr, value, bit_depth); }

	BufPtr = BitUnp_rj(BufPtr, &value, &BitPtr, 1, 6, bit_depth); // program_config
	BufPtr = BitUnp_rj(BufPtr, &value, &BitPtr, 1, 4, bit_depth); // frame_rate_code

	return value;
}

int BitUnkey(
	uint32_t *in_buf,			/* IN: Input buffer */					
	int key_value,				/* IN: key value */
	int bit_pointer,			/* IN: bit pointer into input buffer */
	int num_items,				/* IN: # items to be unpacked */
	int bit_depth) 				/* IN: bit depth (16, 20 or 24) */
{
	uint32_t *payload;
	int i;

	key_value <<= (32 - bitdepthtab[bit_depth]);

	if (bit_pointer != 0){ payload = in_buf + 1; }
	else{ payload = in_buf; }

	for (i = 0; i < num_items; i++)
	{
		payload[i] ^= key_value;
	}

	return(0);
}

uint32_t *BitUnp_rj(					
	uint32_t *in_buf,			/* IN/OUT: Input buffer, Note new pointer position is returned to caller */					
	int dataPtr[],				/* IN/OUT: ptr to data array to be filled */
	int *bit_pointer,			/* IN/OUT: bit pointer into data */
	int numitems, 				/* IN: # items to be unpacked  */
	int numbits,				/* IN: # bits per item */
	int bit_depth)				/* IN: bit_dpeth */
{
	const unsigned int ljMask[32] =
	{	0x00000000, 0x80000000, 0xc0000000, 0xe0000000, 
		0xf0000000, 0xf8000000, 0xfc000000, 0xfe000000, 
		0xff000000, 0xff800000, 0xffc00000, 0xffe00000, 
		0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000, 
		0xffff0000, 0xffff8000, 0xffffc000, 0xffffe000, 
		0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00, 
		0xffffff00, 0xffffff80, 0xffffffc0, 0xffffffe0, 
		0xfffffff0, 0xfffffff8, 0xfffffffc, 0xfffffffe };

	int data, i;
	unsigned int ulsbdata;

	for (i = 0; i < numitems; i++)
	{

	/*	Unpack data as a left-justified element */
		data = (int)((*((int32_t *)in_buf) << *bit_pointer) & ljMask[numbits]);
		*bit_pointer += numbits;
		while (*bit_pointer >= bitdepthtab[bit_depth])
		{
			*bit_pointer -= bitdepthtab[bit_depth];
			ulsbdata = (unsigned int)*++in_buf;
			data |= ((ulsbdata >> (numbits - *bit_pointer)) & ljMask[numbits]);
		}

	/*	Right-justify the element and store to output array */
		*dataPtr++ = (int)((unsigned int)(data) >> (32 - numbits));
	}

	return(in_buf);
}	
