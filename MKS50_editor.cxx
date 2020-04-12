 /*
 * This file is part of MKS50_editor, which is free software:
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "MKS50_editor.h"
#include "MKS50_images.h"

typedef unsigned char t_byte;

#define _NAMESIZE 128

snd_seq_t *seq_handle;
int portid;
int npfd;
struct pollfd *pfd;
t_byte midi_channel; // 0-based
bool auto_send=true; // Sends data after loading file

//  MKS-50 edit buffer
#define TONE_NAME_SIZE 10
#define TONE_PARMS 36
#define TONE_LEVEL 0x20
t_byte tone_parameters[TONE_PARMS];
Fl_Valuator* tone_controls[TONE_PARMS];
char tone_name[TONE_NAME_SIZE+1];

#define PATCH_NAME_SIZE 10
#define PATCH_PARMS 13
#define PATCH_LEVEL 0x30
t_byte patch_parameters[PATCH_PARMS];
Fl_Valuator* patch_controls[PATCH_PARMS];
char patch_name[PATCH_NAME_SIZE+1];
t_byte key_mode_map[]="\x00\x40\x60"; // poly, chord, mono
const char *key_mode_names[] = {"Poly", "Chord", "Mono"};
const char *dco_range_names[] = {"4'", "8'", "16'", "32'"};
const char *on_off_names[] = {"Off", "On"};

#define CHORD_NOTES 6
#define CHORD_LEVEL 0x40
t_byte chord_notes[CHORD_NOTES];
Fl_Valuator* chord_note_controls[CHORD_NOTES];

char mks50_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 -";

Fl_Double_Window *main_window=(Fl_Double_Window *)0;

Fl_Button *btn_testnote=(Fl_Button *)0;
Fl_Button *btn_all_notes_off=(Fl_Button *)0;
Fl_Button *btn_save_current=(Fl_Button *)0;
Fl_Button *btn_load_current=(Fl_Button *)0;
Fl_Button *btn_send_current=(Fl_Button *)0;
Fl_Output *txt_tone_name=(Fl_Output *)0;
Fl_Output *txt_patch_name=(Fl_Output *)0;

char midiName[_NAMESIZE] = "MKS-50 editor";


// Image helper functions - belong somewhere else
void ascii_to_rgb(const unsigned char *src, unsigned char *dest, unsigned int pixels){
	for(int p=0; p<pixels; p++){
		if (*src++!=' '){
			*dest++=0;
			*dest++=0;
			*dest++=0;
		}else{
			*dest++=192;
			*dest++=192;
			*dest++=192;
		}
	}
}

void v_mirror_rgb(const unsigned char *src, unsigned char *dest, unsigned int w, unsigned int h){
	for(int l=0; l<h; l++){ // One line at a time
		for(int c=0; c<w; c++){ // Loop over columns
			dest[(w*(h-l-1)+c)*3]=*src++;
			dest[(w*(h-l-1)+c)*3+1]=*src++;
			dest[(w*(h-l-1)+c)*3+2]=*src++;
		}
	}
}

// MKS-50 specific functions
t_byte char_to_mks50(const char c){
	t_byte d;
	if (c >= 'A' &&  c<= 'Z') d = c - 'A';
	else if (c >= 'a' && c <= 'z') d = c - 'a' + 26;
	else if (c >= '0' && c <= '9') d = c - '0' + 52;
	else if (c == '-') d = 63;
	else d=62; // Non-mappable characters will appear as spaces
	return(d);
}

void set_tone_apr(const t_byte *tone_parameters, const char* tone_name, t_byte* buf){
	// buf must be at least 54 bytes
	*buf++=0xF0; // Start of sysex
	*buf++=0x41; // Roland
	*buf++=0x35; // APR
	*buf++=midi_channel;
	*buf++=0x23; // MKS-50 etc.
	*buf++=TONE_LEVEL; // Level for tone parameters
	*buf++=0x01; // Group
	for (int i=0; i < TONE_PARMS; i++)
		*buf++=tone_parameters[i];
	for (int i=0; i < TONE_NAME_SIZE; i++)
		*buf++=char_to_mks50(tone_name[i]);
	*buf++=0xF7;
}

void set_patch_apr(const t_byte *patch_parameters, const char* patch_name, t_byte* buf){
	// buf must be at least 31 bytes
	*buf++=0xF0; // Start of sysex
	*buf++=0x41; // Roland
	*buf++=0x35; // APR
	*buf++=midi_channel;
	*buf++=0x23; // MKS-50 etc.
	*buf++=PATCH_LEVEL;
	*buf++=0x01; // Group
	for (int i=0; i < PATCH_PARMS; i++){
		if (i==12) // key mode
			*buf++=key_mode_map[patch_parameters[i]];
		else // Regular parameter
			*buf++=patch_parameters[i];
	}
	for (int i=0; i < PATCH_NAME_SIZE; i++)
		*buf++=char_to_mks50(patch_name[i]);
	*buf++=0xF7;
}

void set_chord_apr(const t_byte *chord_notes, t_byte* buf){
	// buf must be at least 14 bytes
	*buf++=0xF0; // Start of sysex
	*buf++=0x41; // Roland
	*buf++=0x35; // APR
	*buf++=midi_channel;
	*buf++=0x23; // MKS-50 etc.
	*buf++=CHORD_LEVEL;
	*buf++=0x01; // Group
	for (int i=0; i < CHORD_NOTES; i++)
		*buf++=chord_notes[i];
	*buf++=0xF7;
}

void fc_save_current_callback(Fl_File_Chooser *w, void *userdata){
	// ?? should remember directory across calls
#ifdef _DEBUG	
	printf("File: %s\n", w->value());
	printf("Directory: %s\n", w->directory());
	printf("Filter: %u\n", w->filter_value());
#endif	
	t_byte buf[54+31+14];
	set_tone_apr(tone_parameters, tone_name, buf);
	set_patch_apr(patch_parameters, patch_name, buf+54);
	set_chord_apr(chord_notes, buf+54+31);

	char * filename;
	FILE *sysex_file;
	filename=(char *)malloc(strlen(w->value())+5);
	if (filename){
		strcpy(filename, w->value());	
		if (strchr(filename,'.') == 0 && w->filter_value() == 0 )
			strcat(filename, ".syx");
		sysex_file=fopen(filename,"wb");
		if (sysex_file==0){
			fprintf(stderr,"ERROR: Can not open sysex file %s\n", filename);
		}else{		
			printf("Opened %s\n", filename);
			fwrite(buf, 1, 54+31+14, sysex_file);
			fclose(sysex_file);
		}
		free(filename);
	}else{
		fprintf(stderr, "ERROR: Cannot allocate memory");
	}
}

static void btn_save_current_callback(Fl_Widget* o, void*) {
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","sysex files(*.syx)",Fl_File_Chooser::CREATE,"Output file");
    fc->value(tone_name);
    fc->callback(fc_save_current_callback);
    fc->show();
}

void get_tone_parameters(const t_byte *buf, t_byte *tone_parameters, char *tone_name){
	// buf points to sysex data, past 7 bytes sysex header
	Fl::lock();
	for (int i=0; i < TONE_PARMS; i++){
		tone_parameters[i]=buf[i];
		if (tone_controls[i]){
			 // FIXME ?? how do we handle different control types 
			tone_controls[i]->value(tone_parameters[i]);
			tone_controls[i]->redraw();
#ifdef _DEBUG											
			printf("Redraw tone parameter #%u\n", i);
#endif											
		}
	}
	for (int i=0; i < TONE_NAME_SIZE; i++)
		tone_name[i]=mks50_chars[buf[i+TONE_PARMS]];
	tone_name[TONE_NAME_SIZE]=0;
	printf("Got all tone parameters for %s\n", tone_name);
	txt_tone_name->value(tone_name);
	txt_tone_name->redraw();
	Fl::awake();
	Fl::unlock();
}

void get_patch_parameters(const t_byte *buf, t_byte *patch_parameters, char *patch_name){
	// buf points to sysex data, past 7 bytes sysex header
	Fl::lock();
	char *k;
	for (int i=0; i < PATCH_PARMS; i++){
		if(i==12){ // Key mode
			k=strchr((char *)key_mode_map, (char)buf[i]);
			if(k){
				patch_parameters[i]=k-(char *)key_mode_map;
			}else{
				patch_parameters[i]=0;
				fprintf(stderr,"ERROR: invalid key mode %02x\n", buf[i]);
			}
		}else{ // Regular parameter
			patch_parameters[i]=buf[i];
		}
		if (patch_controls[i]){
			 // FIXME ?? how do we handle different control types 
			patch_controls[i]->value(patch_parameters[i]);
			patch_controls[i]->redraw();
#ifdef _DEBUG											
			printf("Redraw patch parameter #%u\n", i);
#endif											
		}
	}
	for (int i=0; i < PATCH_NAME_SIZE; i++)
		patch_name[i]=mks50_chars[buf[i+PATCH_PARMS]];
	patch_name[PATCH_NAME_SIZE]=0;
	printf("Got all patch parameters for %s\n", patch_name);
	if(txt_patch_name){
		txt_patch_name->value(patch_name);
		txt_patch_name->redraw();
	}
	Fl::awake();
	Fl::unlock();
}

void get_chord_notes(const t_byte *buf, t_byte *chord_notes){
	// buf points to sysex data, past 7 bytes sysex header
	Fl::lock();
	for (int i=0; i < CHORD_NOTES; i++){
		chord_notes[i]=buf[i];
		if (chord_note_controls[i]){
			 // FIXME ?? how do we handle different control types 
			chord_note_controls[i]->value(chord_notes[i]);
			chord_note_controls[i]->redraw();
#ifdef _DEBUG											
			printf("Redraw chord note #%u\n", i);
#endif											
		}
	}
	printf("Got all chord notes\n");
	Fl::awake();
	Fl::unlock();
}

static void *alsa_midi_process(void *handle) {
	struct sched_param param;
	int policy;
	snd_seq_t *seq_handle = (snd_seq_t *)handle;

//	pthread_getschedparam(pthread_self(), &policy, &param);
//	policy = SCHED_FIFO;
//  param.sched_priority = 95;
//	pthread_setschedparam(pthread_self(), policy, &param);

  snd_seq_event_t *ev; // event data is up to 12 bytes
  snd_seq_ev_ext_t *evx; // for sysex
  t_byte *ptr;
  int len, errs, channel;

  do {
	while (snd_seq_event_input(seq_handle, &ev)) {
		switch (ev->type) {
		case SND_SEQ_EVENT_SYSEX:
		    printf("alsa_midi_process: sysex event, received %u bytes.\n", ev->data.ext.len);
		    ptr=(t_byte *)ev->data.ext.ptr;
		    len=ev->data.ext.len;
#ifdef _DEBUG
			// Hex dump of sysex contents
		    for (int i=0; i < len; i++)
				printf("%02x ", ptr[i]);
			printf("\n");
#endif			
		    errs=0;
			if(ptr[0]!=0xF0) {fprintf(stderr, "ERROR: No sysex header\n"); errs++;}
			if(ptr[1]!=0x41) {fprintf(stderr, "ERROR: Not a Roland sysex\n"); errs++;}
			if(errs==0 && ptr[2]==0x35) { // "APR" All parameters
				channel=ptr[3];
				if (ptr[len-1]==0Xf7) { // APR sysex is shorter than buffer !!
					if(ptr[4]=0x23){ // MKS-50 etc.
						switch(ptr[5]){
							case TONE_LEVEL:
								if(len==54){
									get_tone_parameters(ptr+7, tone_parameters, tone_name);
								}else{
									fprintf(stderr, "ERROR: Not a tone parameter sysex\n");
									errs++;
								}
								break;
							case PATCH_LEVEL:
								if(len==31){
									get_patch_parameters(ptr+7, patch_parameters, patch_name);
								}else{
									fprintf(stderr, "ERROR: Not a patch parameter sysex\n");
									errs++;
								}
								break;
							case CHORD_LEVEL:
								if(len==14){
									for (int i=0; i < CHORD_NOTES; i++)
										chord_notes[i]=ptr[i+7];
									printf("Received chord memory\n");
								}else{
									fprintf(stderr, "ERROR: Not a chord memory sysex\n");
									errs++;
								}
								break;
							default:
								fprintf(stderr, "ERROR: unknown sysex level %02x\n", ptr[5]);
								errs++;
						}
					}else{
						fprintf(stderr, "ERROR: no end of sysex %02x\n", ptr[len-1]);
						errs++;
					}
					
				}else{
					fprintf(stderr, "ERROR: Not a MKS-50 sysex\n"); errs++;
				}
			}
			
/*			
alsa_midi_process: sysex event, received 31 bytes. (all patch parameters
f0 41 35 00 23 30 01 01 0c 6d 14 00 20 00 7f 00 00 0c 00 00 09 1a 33 33 06 2e 22 2d 1a 2b f7 
alsa_midi_process: sysex event, received 14 bytes. (chord memory)
f0 41 35 00 23 40 01 3c 7f 7f 7f 7f 7f f7 
alsa_midi_process: sysex event, received 54 bytes.  (all tone parameters)
f0 41 35 00 23 20 01 00 02 02 03 00 00 02 00 00 00 00 00 00 28 27 00 2b 00 00 6b 50 00 68 00 56 2a 00 7f 22 68 4a 00 2c 28 47 02 09 1a 33 33 06 2e 22 2d 1a 2b f7
*/
			// f0 41 40 00 23 f7
			// bulk dump: buffer seems to be 256 bytes, 266 needed
			// TODO Should loop until f7 is received
		    break;
		case SND_SEQ_EVENT_PGMCHANGE:
		  {
			int channel = ev->data.control.channel;
			int value = ev->data.control.value;
#ifdef _DEBUG
			printf("alsa_midi_process: program change event on Channel %2d: %2d %5d\n",
					channel, ev->data.control.param, value);
#endif

			break;
		  }
		case SND_SEQ_EVENT_CONTROLLER:
		  {
#ifdef _DEBUG
			fprintf(stderr, "Control event on Channel %2d: %2d %5d       \r",
					ev->data.control.channel, ev->data.control.param, ev->data.control.value);
#endif
			// MIDI Controller  0 = Bank Select MSB (Most Significant Byte)
			// MIDI Controller 32 = Bank Select LSB (Least Significant Byte)

			break;
		  }
		} // end of switch
		snd_seq_free_event(ev);
	} // end of first while, emptying the seqdata queue
  } while (true); // doing forever, was  (snd_seq_event_input_pending(seq_handle, 0) > 0);
  return 0;
}


snd_seq_t *open_seq() {
  snd_seq_t *seq_handle;
  if (snd_seq_open(&seq_handle, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
	fprintf(stderr, "Error opening ALSA sequencer.\n");
	exit(1);
  }
  snd_seq_set_client_name(seq_handle, midiName);
  if ((portid = snd_seq_create_simple_port(seq_handle, midiName,
			SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ|
			SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
			SND_SEQ_PORT_TYPE_APPLICATION)) < 0) {
	fprintf(stderr, "Error creating sequencer port.\n");
	exit(1);
  }
  return(seq_handle);
}

bool state;
static void btn_testnote_callback(Fl_Widget* o, void*) {
	state=!state;
#ifdef _DEBUG	
	printf("State %u\n",state);
#endif
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, portid); 
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	ev.queue = SND_SEQ_QUEUE_DIRECT; // maybe redundant

	if (state)
	    ev.type=SND_SEQ_EVENT_NOTEON;
	else
	    ev.type=SND_SEQ_EVENT_NOTEOFF;
	ev.data.note.channel=midi_channel;
	ev.data.note.note=60;
	ev.data.note.velocity=64;
	
	snd_seq_event_output_direct(seq_handle, &ev);
}

void all_notes_off(int channel){
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, portid); 
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	ev.queue = SND_SEQ_QUEUE_DIRECT; // maybe redundant
	
    ev.type=SND_SEQ_EVENT_CONTROLLER;
	ev.data.control.channel=channel;
	ev.data.control.param=123;
	ev.data.control.value=0;
	
	snd_seq_event_output_direct(seq_handle, &ev);
}

static void btn_all_notes_off_callback(Fl_Widget* o, void*) {
	state=0; // Reset test note state
	all_notes_off(midi_channel);
}

static int send_sysex(t_byte * sysex, const unsigned int len){
#ifdef _DEBUG		
	// Hex dump of sysex contents
	for (int i=0; i < len; i++)
		printf("%02x ", sysex[i]);
	printf("\n");		
#endif			
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, portid); 
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	ev.queue = SND_SEQ_QUEUE_DIRECT; // Possibly redundant
	ev.flags |= SND_SEQ_EVENT_LENGTH_VARIABLE; // Mandatory !!
	ev.type = SND_SEQ_EVENT_SYSEX;
	ev.data.ext.len = len;
	ev.data.ext.ptr = sysex;
	int status = snd_seq_event_output_direct(seq_handle, &ev);
#ifdef _DEBUG		
	printf("Output status %i\n", status);
#endif
	return(status);
}

static void parm_callback(Fl_Widget* o, void*) {
	t_byte sysex[]="\xF0\x41\x36\x00\x23\x20\x01\x00\x00\xF7";
	if (o){
		printf("%s parameter #%u = %u\n", (unsigned int)o->argument() & 0x80 ? "Patch": "Tone", (unsigned int)o->argument() & 0x3F, (unsigned int)((Fl_Valuator*)o)->value());
		sysex[3]=midi_channel;
		// Parameter number coded on 7 bits
		// Parameter type on bit 7, 0 for tone, 1 for patch
		sysex[5]=(t_byte)o->argument() & 0x80 ? 0x30: 0x20;
		sysex[7]=(t_byte)o->argument() & 0x3F;
		sysex[8]=(t_byte)((Fl_Valuator*)o)->value();
		send_sysex(sysex, 10);
		// check_error(status, "snd_seq_event_output_direct");
	}
}

static void parm_keymode_callback(Fl_Widget* o, void*) {
	t_byte sysex[]="\xF0\x41\x36\x00\x23\x20\x01\x00\x00\xF7";
	if (o){
		printf("%s keymode parameter #%u = %u\n", (unsigned int)o->argument() & 0x80 ? "Patch": "Tone", (unsigned int)o->argument() & 0x3F, (unsigned int)((Fl_Valuator*)o)->value());
		sysex[3]=midi_channel;
		// Parameter number coded on 7 bits
		// Parameter type on bit 7, 0 for tone, 1 for patch
		sysex[5]=(t_byte)o->argument() & 0x80 ? 0x30: 0x20;
		sysex[7]=(t_byte)o->argument() & 0x3F;
		sysex[8]=key_mode_map[(t_byte)((Fl_Valuator*)o)->value()];
		send_sysex(sysex, 10);
		// check_error(status, "snd_seq_event_output_direct");
	}
}

static void send_current(){
	// Sends current contents (patch, tone and chord) over MIDI
	t_byte buf[54];
	set_tone_apr(tone_parameters, tone_name, buf);
	send_sysex(buf,54);
	set_patch_apr(patch_parameters, patch_name, buf);
	send_sysex(buf,31);
	set_chord_apr(chord_notes, buf);
	send_sysex(buf,14);
}

static void btn_send_current_callback(Fl_Widget* o, void*) {
	send_current();
}

void fc_load_current_callback(Fl_File_Chooser *w, void *userdata){
	// ?? should remember directory across calls
	printf("File: %s\n", w->value());
	printf("Directory: %s\n", w->directory());
	printf("Filter: %u\n", w->filter_value());
	t_byte buf[54];
	int len, errs=0;
	bool keep_running=true;

	FILE *sysex_file;
	sysex_file=fopen(w->value(),"rb");
	if (sysex_file==0){
		fprintf(stderr,"ERROR: Can not open sysex file %s\n", w->value());
	}else{		
		printf("Opened %s\n", w->value());
		while(keep_running && errs==0){
			len=fread(buf, 1, 7, sysex_file); // Get header
			if (len==7 && buf[0]==0xF0 && buf[1]==0x41 && buf[2]==0x35 && buf[4]==0x23 && buf[6]==0x01){
				 // ?? what about apr without name
				switch (buf[5]){
					case TONE_LEVEL:
						len=fread(buf+7, 1, 54-7, sysex_file);
						if(len == 54-7 && buf[53] == 0xF7){
							get_tone_parameters(buf+7, tone_parameters, tone_name);
							printf("Read tone parameters\n");
						}else{
							fprintf(stderr,"ERROR: invalid tone parameters\n");
							errs++;
						}
						break;
					case PATCH_LEVEL:
						len=fread(buf+7, 1, 31-7, sysex_file);
						if(len == 31-7 && buf[30] == 0xF7){
							get_patch_parameters(buf+7, patch_parameters, patch_name);
							printf("Read patch parameters\n");
						}else{
							fprintf(stderr,"ERROR: invalid patch parameters\n");
							errs++;
						}
						break;
					case CHORD_LEVEL:
						len=fread(buf+7, 1, 14-7, sysex_file);
						if(len == 14-7 && buf[13] == 0xF7){
							get_chord_notes(buf+7, chord_notes);
							printf("Read chord notes\n");
						}else{
							fprintf(stderr,"ERROR: invalid chord notes\n");
							errs++;
						}
						break;
					default:
						fprintf(stderr,"ERROR: unknown level value %u\n", buf[5]);
						errs++;
				}
			}else{
				if(len == 0){
					fprintf(stderr,"End of file\n");
					keep_running=false;
				}else{
					fprintf(stderr,"ERROR: invalid sysex header\n");
					errs++;
				}
			}
		} // end of while
		fclose(sysex_file);
		if (auto_send && errs==0) send_current();
	}
}

static void btn_load_current_callback(Fl_Widget* o, void*) {
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","sysex files(*.syx)",Fl_File_Chooser::SINGLE,"Input file");
    fc->callback(fc_load_current_callback);
    fc->show();
}

Fl_Value_Slider *make_value_slider(int x, int y, int w, int h, const char * label, int parm_number, int max_value){
	Fl_Value_Slider *o = new Fl_Value_Slider(x, y, w, h, label);
	if(parm_number & 0x80)
		patch_controls[parm_number & 0x3f]=o;
	else
		tone_controls[parm_number & 0x3f]=o;
	o->argument(parm_number); // Including bit 7
	o->bounds(max_value, 0); // Possibly useless
	o->range(max_value, 0);
	o->step(1);
	o->align(FL_ALIGN_TOP);
	o->callback((Fl_Callback*)parm_callback); // Handles both tone and patch parameters
	o->slider_size(.04);
	o->textsize(12);
}

// GUI creation helper functions
Fl_List_Slider *make_list_slider(int x, int y, int w, int h, const char * label, int parm_number, int max_value, const char** names){
	Fl_List_Slider *o = new Fl_List_Slider(x, y, w, h, label);
	if(parm_number & 0x80)
		patch_controls[parm_number & 0x3f]=o;
	else
		tone_controls[parm_number & 0x3f]=o;
	o->argument(parm_number); // Including bit 7
	o->bounds(max_value, 0);
	o->range(max_value, 0); // Possibly useless
	o->step(1);
	o->align(FL_ALIGN_TOP);
	o->callback((Fl_Callback*)parm_callback); // Handles both tone and patch parameters
	o->slider_size(.04);
	o->list(names, max_value+1); // Starting from 0
}

Fl_Image_List_Slider *make_image_list_slider(int x, int y, int w, int h, const char * label, int parm_number, int max_value, const Fl_RGB_Image ** images){
	Fl_Image_List_Slider *o = new Fl_Image_List_Slider(x, y, w, h, label);
	if(parm_number & 0x80)
		patch_controls[parm_number & 0x3f]=o;
	else
		tone_controls[parm_number & 0x3f]=o;
	o->argument(parm_number); // Including bit 7
	o->bounds(max_value, 0);
	o->range(max_value, 0); // Possibly useless
	o->step(1);
	o->align(FL_ALIGN_TOP);
	o->callback((Fl_Callback*)parm_callback); // Handles both tone and patch parameters
	o->slider_size(.04);
	o->list(images, max_value+1); // Starting from 0
}

// GUI creation
Fl_Double_Window* make_window() {
	int w=50, h=25, spacing=5;
	int x=spacing, y=spacing;

	// Prepare images
	ascii_to_rgb(env_image_data_ascii, env_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(env_dyn_image_data_ascii, env_dyn_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(env_gate_image_data_ascii, env_gate_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(env_dyn_gate_image_data_ascii, env_dyn_gate_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	v_mirror_rgb(env_image_data_rgb, env_rev_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH);
	v_mirror_rgb(env_dyn_image_data_rgb, env_dyn_rev_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH);
	env_images_rgb[0]=&env_image_rgb;
	env_images_rgb[1]=&env_rev_image_rgb;
	env_images_rgb[2]=&env_dyn_image_rgb;
	env_images_rgb[3]=&env_dyn_rev_image_rgb;
	env_vca_images_rgb[0]=&env_image_rgb;
	env_vca_images_rgb[1]=&env_gate_image_rgb;
	env_vca_images_rgb[2]=&env_dyn_image_rgb;
	env_vca_images_rgb[3]=&env_dyn_gate_image_rgb;
	ascii_to_rgb(pulse_image_data_ascii, pulse_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(pulse_mod_image_data_ascii, pulse_mod_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(square_image_data_ascii, square_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(off_image_data_ascii, off_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(narrow_square_image_data_ascii, narrow_square_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(narrow_pulse_image_data_ascii, narrow_pulse_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(double_pulse_image_data_ascii, double_pulse_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(quad_pulse_image_data_ascii, quad_pulse_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	pulse_images_rgb[0]=&off_image_rgb;
	pulse_images_rgb[1]=&square_image_rgb;
	pulse_images_rgb[2]=&pulse_image_rgb;
	pulse_images_rgb[3]=&pulse_mod_image_rgb;
	sub_images_rgb[0]=&narrow_square_image_rgb;
	sub_images_rgb[1]=&narrow_pulse_image_rgb;
	sub_images_rgb[2]=&double_pulse_image_rgb;
	sub_images_rgb[3]=&quad_pulse_image_rgb;
	sub_images_rgb[4]=&square_image_rgb;
	sub_images_rgb[5]=&pulse_image_rgb;
	ascii_to_rgb(saw1_image_data_ascii, saw1_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(saw2_image_data_ascii, saw2_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(saw3_image_data_ascii, saw3_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(saw4_image_data_ascii, saw4_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(saw5_image_data_ascii, saw5_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	saw_images_rgb[0]=&off_image_rgb;
	saw_images_rgb[1]=&saw1_image_rgb;
	saw_images_rgb[2]=&saw2_image_rgb;
	saw_images_rgb[3]=&saw3_image_rgb;
	saw_images_rgb[4]=&saw4_image_rgb;
	saw_images_rgb[5]=&saw5_image_rgb;


	// Create controls
	{ main_window = new Fl_Double_Window(spacing+20*(w+spacing), 768, "Yet another MKS-50 patch editor");
		btn_testnote = new Fl_Button(x, y, 2*w+spacing, h, "Test note");
		btn_testnote->callback((Fl_Callback*)btn_testnote_callback);
		x += 2*w + 2*spacing;
		btn_all_notes_off = new Fl_Button(x, y, 2*w+spacing, h, "Panic");
		btn_all_notes_off->callback((Fl_Callback*)btn_all_notes_off_callback);
		x += 2*w + 2*spacing;
		btn_save_current = new Fl_Button(x, y, 2*w+spacing, h, "Save");
		btn_save_current->callback((Fl_Callback*)btn_save_current_callback);
		x += 2*w + 2*spacing;
		btn_load_current = new Fl_Button(x, y, 2*w+spacing, h, "Load");
		btn_load_current->callback((Fl_Callback*)btn_load_current_callback);
		x += 2*w + 2*spacing;
		btn_send_current = new Fl_Button(x, y, 2*w+spacing, h, "Send");
		btn_send_current->callback((Fl_Callback*)btn_send_current_callback);
		x += 2*w + 2*spacing;
		txt_tone_name = new Fl_Output(x, y, 3*w + 2*spacing, h);
		// All sliders for tone parameters unless otherwise noted
		x=spacing; y += 3*h + spacing; // New row
		make_value_slider(x, y, w, 8*h, "LFO\nrate", 24, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "LFO\ndelay", 25, 127); x += w + spacing;
		x+=w+spacing;
		make_list_slider(x, y, w, 4*h, "DCO\nrange", 6, 3, dco_range_names); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "DCO\nlfo", 11, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "DCO\nenv", 12, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "DCO\nafter", 13, 127); x += w + spacing;
		// make_value_slider(x, y, w, 4*h, "DCO\nenv\nmode", 0, 3); x += w + spacing;
		make_image_list_slider(x, y, w, 4*h, "DCO\nenv\nmode", 0, 3, env_images_rgb); x += w + spacing;
		x+=w+spacing;
		make_list_slider(x, y, w, 4*h, "Key\nmode", 128 + 12, 2, key_mode_names); x += w + spacing; // Patch parameter
		patch_controls[12]->callback((Fl_Callback*)parm_keymode_callback); // Requires special mapping
		make_value_slider(x, y, w, 4*h, "Chord\nmemory", 128 + 11, 15); x += w + spacing; // Patch parameter
			
		x=spacing; y += 9*h + 2*spacing; // New row
		// make_value_slider(x, y, w, 4*h, "DCO\npulse", 3, 3); x += w + spacing;
		make_image_list_slider(x, y, w, 4*h, "DCO\npulse", 3, 3, pulse_images_rgb); x += w + spacing;
		// make_value_slider(x, y, w, 5*h, "DCO\nsaw", 4, 5); x += w + spacing;
		make_image_list_slider(x, y, w, 5*h, "DCO\nsaw", 4, 5, saw_images_rgb); x += w + spacing;
		// make_value_slider(x, y, w, 5*h, "DCO\nsub", 5, 5); x += w + spacing;
		make_image_list_slider(x, y, w, 5*h, "DCO\nsub", 5, 5, sub_images_rgb); x += w + spacing;
		make_value_slider(x, y, w, 5*h, "Sub\nlevel", 7, 3); x += w + spacing;
		make_value_slider(x, y, w, 5*h, "Noise\nlevel", 8, 3); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "Pulse\nwidth", 14, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "Pulse\nmod\nrate", 15, 127); x += w + spacing;
		make_value_slider(x, y, w, 5*h, "HPF\nfreq", 9, 3); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "VCF\nfreq", 16, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "VCF\nreso", 17, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "VCF\nlfo", 18, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "VCF\nenv", 19, 127); x += w + spacing;
		make_image_list_slider(x, y, w, 4*h, "VCF\nenv\nmode", 1, 3, env_images_rgb); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "VCF\nkey\nfollow", 20, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "VCF\nafter", 21, 127); x += w + spacing;
		make_list_slider(x, y, w, 3*h, "Chorus", 10, 1, on_off_names); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "Chorus\nrate", 34, 127); x += w + spacing;
		x=spacing; y += 9*h + 2*spacing; // New row
		make_value_slider(x, y, w, 8*h, "VCA\nlevel", 22, 127); x += w + spacing;
		// make_value_slider(x, y, w, 4*h, "VCA\nenv\nmode", 2, 3); x += w + spacing;
		make_image_list_slider(x, y, w, 4*h, "VCA\nenv\nmode", 2, 3, env_vca_images_rgb); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "VCA\nafter", 23, 127); x += w + spacing;
		x+=w+spacing;
		make_value_slider(x, y, w, 8*h, "ENV\nT1", 26, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "ENV\nL1", 27, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "ENV\nT2", 28, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "ENV\nL2", 29, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "ENV\nT3", 30, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "ENV\nL3", 31, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "ENV\nT4", 32, 127); x += w + spacing;
		make_value_slider(x, y, w, 8*h, "ENV\nkey", 33, 127); x += w + spacing;
		x+=w+spacing;
		make_value_slider(x, y, w, 8*h, "Bender\nrange", 35, 12); x += w + spacing;
		x+=w+spacing;
		make_list_slider(x, y, w, 3*h, "Porta.", 128 + 4, 1, on_off_names); x += w + spacing; // Patch parameter
		make_value_slider(x, y, w, 8*h, "Porta.\ntime", 128 + 3, 127); x += w + spacing; // Patch parameter
		x+=w+spacing;
	
		// Fl_Image_List_Slider *o = new Fl_Image_List_Slider(env_images_rgb, 4, x, y, w, 4*h, "kiki"); // ok
		/* ok
		{ Fl_Image_List_Slider *o = new Fl_Image_List_Slider(x, y, w, 8*h);
				o->list(env_images_rgb, 4); // Starting from 0
				// printf("Image list at %p\n", o->list());
		}
		* */
		x+=w+spacing;
		/* Ok
		{ Fl_Box* o = new Fl_Box(x, y, 50, 25);
			printf("Image 2 at %p\n", &env_images_rgb[2]);
			o->image((Fl_RGB_Image *) env_images_rgb[2]);}
		*/
		main_window->end();
		main_window->resizable(main_window);
	} // Fl_Double_Window* main_window
	return main_window;
}

int main(int argc, char **argv) {
// ------------------------ midi init ---------------------------------
	pthread_t midithread;
	seq_handle = open_seq();
	npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);

	// create the thread and tell it to use Midi::work as thread function
	int err = pthread_create(&midithread, NULL, alsa_midi_process, seq_handle);
	if (err) {
		fprintf(stderr, "Error %u creating MIDI thread\n", err);
		// should exit? This is non-blocking for the GUI.
	}

	Fl_Window* win = make_window();
	win->show();
	return Fl::run();
}
