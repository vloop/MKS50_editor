 /*
 * This file is part of MKS50_editor, which is free software:
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
 
/* TODO
 * controls for all 13 patch parameters
 * make sure changed flag is updated properly for recall, rename, store, load, save
 * warn about handshake not supported
 * buttons copy and paste for tables (single button changes from copy to paste?)
 * is 0 patch and tone acceptable as valid, in banks and in current? (chords are not)
 */

#include "MKS50_editor.h"
#include "MKS50_images.h"

typedef unsigned char t_byte;

#define _NAMESIZE 128

// MIDI related
char midiName[_NAMESIZE] = "MKS-50 editor";
snd_seq_t *seq_handle;
int portid;
int npfd;
struct pollfd *pfd;
t_byte midi_channel=0; // 0-based
t_byte midi_test_note=60;
t_byte midi_test_velocity=80;

bool auto_send=true; // Sends data after loading patch file or recalling patch

//  MKS-50 edit buffer and memory
#define TONE_NAME_SIZE 10
#define TONE_PARMS 36
#define TONES 128
#define TONE_LEVEL 0x20
t_byte tone_parameters[TONE_PARMS];
char tone_name[TONE_NAME_SIZE+1];
Fl_Valuator* tone_controls[TONE_PARMS];

typedef struct {
	bool valid=false;
	bool changed=false;
	int program_offset=0;
	// 64 tones to a bank
	t_byte parameters[64][TONE_PARMS];
	char names[64][TONE_NAME_SIZE+1];
} Tone_bank;
Tone_bank tone_bank_a, tone_bank_b;

#define PATCH_NAME_SIZE 10
#define PATCH_PARMS 13
#define PATCHES 128
#define PATCH_LEVEL 0x30
t_byte patch_parameters[PATCH_PARMS];
Fl_Valuator* patch_controls[PATCH_PARMS];
char patch_name[PATCH_NAME_SIZE+1];
t_byte key_mode_map[]="\x00\x40\x60\x00"; // poly, chord, mono
const char *key_mode_names[] = {"Poly", "Chord", "Mono"};
const char *dco_range_names[] = {"4'", "8'", "16'", "32'"};
const char *on_off_names[] = {"Off", "On"};
typedef struct {
	bool valid=false;
	bool changed=false;
	int program_offset=0;
	// 64 patches to a bank
	t_byte parameters[64][PATCH_PARMS];
	char names[64][PATCH_NAME_SIZE+1];
} Patch_bank;
Patch_bank patch_bank_a, patch_bank_b;

#define CHORD_NOTES 6
#define CHORDS 16
#define CHORD_LEVEL 0x40
#define CHORD_BASE_NOTE 0x3C
#define CHORD_MIN_NOTE (CHORD_BASE_NOTE-24)
#define CHORD_MAX_NOTE (CHORD_BASE_NOTE+24)
t_byte chord_notes[CHORD_NOTES];
// Note: chord notes are relative to CHORD_BASE_NOTE 7-bit signed
// in apr and in chord_notes, 0x3C=60 means C4 means no transpose
// Special value 127 is used for no note (chord location off)
// in bld, coding is different!
// However, we also use apr coding in chord_memory for consistency
Fl_Valuator* chord_note_controls[CHORD_NOTES];
t_byte chord_memory[CHORDS][CHORD_NOTES];
bool chord_memory_valid=false;
bool chord_memory_changed=false;
bool chord_from_apr=false;

bool current_valid=false;
bool current_patch_valid=false;
bool current_tone_valid=false;
bool current_chord_valid=false;

bool current_changed=false;
bool current_patch_changed=false;
bool current_tone_changed=false;
bool current_chord_changed=false;

#define PATCH_BLD_SIZE 266
#define TONE_BLD_SIZE 266
#define CHORD_BLD_SIZE 202
#define PATCH_APR_SIZE 31
#define TONE_APR_SIZE 54
#define CHORD_APR_SIZE 14

#define SYSEX_MAX_SIZE 266
#define SYSEX_HEADER_SIZE 6

#define OP_APR 0x35
#define OP_BLD 0x37

// User interface variables
// Some of these are not really needed
Fl_Double_Window *main_window=(Fl_Double_Window *)0;
Fl_Group *controls_group=(Fl_Group *)0;
Fl_Group *patches_group=(Fl_Group *)0;
Fl_Group *tones_group=(Fl_Group *)0;
Fl_Group *chords_group=(Fl_Group *)0;

Fl_Toggle_Button *btn_testnote=(Fl_Toggle_Button *)0;
Fl_Button *btn_all_notes_off=(Fl_Button *)0;
Fl_Button *btn_save_current=(Fl_Button *)0;
Fl_Button *btn_load_current=(Fl_Button *)0;
Fl_Button *btn_send_current=(Fl_Button *)0;
Fl_Toggle_Button *btn_rename_patch=(Fl_Toggle_Button *)0;
Fl_Toggle_Button *btn_store_patch=(Fl_Toggle_Button *)0;
Fl_Button *btn_save_patch_bank_a=(Fl_Button *)0;
Fl_Button *btn_save_patch_bank_b=(Fl_Button *)0;
Fl_Button *btn_send_patch_bank_a=(Fl_Button *)0;
Fl_Button *btn_send_patch_bank_b=(Fl_Button *)0;
Fl_Toggle_Button *btn_rename_tone=(Fl_Toggle_Button *)0;
Fl_Toggle_Button *btn_store_tone=(Fl_Toggle_Button *)0;
Fl_Button *btn_save_tone_bank_a=(Fl_Button *)0;
Fl_Button *btn_save_tone_bank_b=(Fl_Button *)0;
Fl_Button *btn_send_tone_bank_a=(Fl_Button *)0;
Fl_Button *btn_send_tone_bank_b=(Fl_Button *)0;
Fl_Button *btn_save_chords=(Fl_Button *)0;
Fl_Button *btn_send_chords=(Fl_Button *)0;
Fl_Input *txt_tone_name=(Fl_Input *)0;
Fl_Output *txt_tone_num=(Fl_Output *)0;
Fl_Input *txt_patch_name=(Fl_Input *)0;
Fl_Input *txt_chord=(Fl_Input *)0;
Fl_Input *txt_midi_channel=(Fl_Input *)0;
Fl_Input *txt_test_note=(Fl_Input *)0;
Fl_Input *txt_test_vel=(Fl_Input *)0;
Fl_Toggle_Button *btn_store_chord=(Fl_Toggle_Button *)0;
Fl_Value_Slider *vs_pulse_width=(Fl_Value_Slider *)0;
Fl_Value_Slider *vs_pulse_mod_rate=(Fl_Value_Slider *)0;
Fl_Value_Slider *vs_chorus_rate=(Fl_Value_Slider *)0;
Fl_Value_Slider *vs_porta_time=(Fl_Value_Slider *)0;
Fl_Input *txt_key_low=(Fl_Input *)0;
Fl_Input *txt_key_high=(Fl_Input *)0;
Fl_Int_Input *txt_key_shift=(Fl_Int_Input *)0;

////////////////////
// Misc utilities //
////////////////////

// see https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

void hex_dump(const t_byte* ptr, const int len){
		// Hex dump of sysex contents
		for (int i=0; i < len; i++){
			if(i%16==0) printf("%p [%04x] ", ptr+i, i);
			printf("%02x ", ptr[i]);
			if(i%16==15) printf("\n");
		}
		if (len%16) printf("\n");
}

static void show_err ( const char * format, ... ){
	char s[1024];
	va_list args;
    va_start(args, format);
    vsnprintf(s, 1024, format, args);
    fprintf(stderr, "%s\n", s);
    fl_alert("%s", s);
    va_end(args);
}

char note_names[][3]={
	"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

int sprint_note(char *s, const t_byte note){
	return(sprintf(s, "%s%i ", note_names[note % 12], note / 12 - 1));
}

int strtonote(const char* str, const char **endptr){
	int note_nums[]={9,11,0,2,4,5,7}; // A..G
	const char *ptr=str;
	char *ptr2;
	int note;
	long octave;
	while(*ptr==32) ptr++; // Skip leading spaces
	char c=toupper(*ptr);
	if(c>='A' && c<='G'){
		// got a proper note name
		note=note_nums[c-'A'];
		ptr++;
		// check for alteration (single or double)
		if (*ptr=='#') {ptr++; note++; if (*ptr=='#') {ptr++; note++;}}
		else if (*ptr=='x' || *ptr=='X') {ptr++; note+=2;}
		else if (*ptr=='b') {ptr++; note--; if (*ptr=='b') {ptr++; note--;}}
		// get octave
		octave=strtol(ptr, &ptr2, 10)+1;
		if(ptr2>ptr){
			note+=12*octave;
			if(note<0 or note>127){
				fprintf(stderr, "Note %u out of range\n", note);
				note=-1;
				ptr=str;
			}else{
				ptr=ptr2;
			}
		}else{
			fprintf(stderr, "Cannot convert \"%s\" to octave number\n", ptr);
			note=-1;
			ptr=str;
		}
	}else{
		// Illegal note name
		if (*ptr) fprintf(stderr, "Cannot convert '%c' (%u) to note\n", c, c);
		note=-1;
		ptr=str;
	}
	*endptr=ptr;
	return(note);
}

////////////////////////////
// Image helper functions //
////////////////////////////

// these functions really belong somewhere else
void ascii_to_rgb(const unsigned char *src, unsigned char *dest, unsigned int pixels){
	// space becomes grey, non-space becomes black
	// Could interpret different chars as different colors ?
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

////////////////////////////////////////////////
// MKS-50 specific low-level helper functions //
////////////////////////////////////////////////

char mks50_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 -";

t_byte char_to_mks50(const char c){
	t_byte d;
	if (c >= 'A' &&  c<= 'Z') d = c - 'A';
	else if (c >= 'a' && c <= 'z') d = c - 'a' + 26;
	else if (c >= '0' && c <= '9') d = c - '0' + 52;
	else if (c == '-') d = 63;
	else d=62; // Non-mappable characters will appear as spaces
	return(d);
}

int nibbles_to_bytes(const t_byte *src, t_byte *dest, int byte_count){
	bool illegal=false;
	while((byte_count--)>0){
		// Check legal nibbles <16
		if (*src>15){
			fprintf(stderr,"ERROR: illegal nibble value %u \n", (int)(*src));
			illegal=true;
		}
		if (*(src+1)>15){
			fprintf(stderr,"ERROR: illegal nibble value %u \n", (int)(*(src+1)));
			illegal=true;
		}
		*dest = *src + (*(src+1) << 4); // Least significant nibble first
		src+=2; // Two nibbles to a byte
		dest++;	
	}
	return (illegal);
}

int bytes_to_nibbles(const t_byte *src, t_byte *dest, int byte_count){
	while((byte_count--)>0){
		*dest++ = *src & 0x0F; // Least significant nibble first
		*dest++ = *src++ >> 4;
	}
	return (0);
}

/** Translate a 6-bit encoded string to ascii
 */

void get_mks50_name(const t_byte *src, char *dest, int len){
	for(int i=0; i<len; i++){
		// Just ignore high bits (used for other purposes in bld)
		*dest++=mks50_chars[(*src++) & 0x3F];
	}
	*dest=0;
}

/** Translate an ascii string to 6-bit encoded
 */

void set_mks50_name(const char *src, t_byte *dest, int len){
	// Will always set len characters, padding with spaces if needed
	int i;
	char c;
	t_byte d;
	for(i=0; i<len; i++){
		c=*src;
		if (c) src++; // Don't go past end!
		if (c >= 'A' &&  c<= 'Z') d = c - 'A';
		else if (c >= 'a' && c <= 'z') d = c - 'a' + 26;
		else if (c >= '0' && c <= '9') d = c - '0' + 52;
		else if (c == '-') d = 63;
		else d=62; // Non-mappable characters will appear as spaces
		*dest++=d;
	}
}

///////////////////////////////////////////////////////////////
// MKS-50 specific tone, patch and chords handling functions //
///////////////////////////////////////////////////////////////

int make_MKS50_sysex_header(t_byte *sysex, const int channel, const int op, const int level){
	sysex[0]=0xF0;
	sysex[1]=0x41;
	sysex[2]=op;
	sysex[3]=channel;
	sysex[4]=0x23;
	sysex[5]=level;
	return(SYSEX_HEADER_SIZE);
}

/** Translate tone parameters and name to apr sysex message
 */
void set_apr_from_tone_parameters(const t_byte *tone_parameters, const char* tone_name, t_byte* buf){
	// buf must be at least 54 bytes
	buf+=make_MKS50_sysex_header(buf, midi_channel, OP_APR, TONE_LEVEL);
	*buf++=0x01; // Group
	for (int i=0; i < TONE_PARMS; i++)
		*buf++=tone_parameters[i];
	for (int i=0; i < TONE_NAME_SIZE; i++){
		*buf++=char_to_mks50(*tone_name);
		if (*tone_name) tone_name++;
	}
	*buf++=0xF7;
}

/** Translate patch parameters and name to apr sysex message
 */
void set_apr_from_patch_parameters(const t_byte *patch_parameters, const char* patch_name, t_byte* buf){
	// buf must be at least 31 bytes
	buf+=make_MKS50_sysex_header(buf, midi_channel, OP_APR, PATCH_LEVEL);
	*buf++=0x01; // Group
	for (int i=0; i < PATCH_PARMS; i++){
		if (i==12){ // key mode
			if(patch_parameters[i]>2){
				fprintf(stderr, "Illegal key mode %u\n", patch_parameters[i]);
				*buf++=key_mode_map[0];
			}else{
				*buf++=key_mode_map[patch_parameters[i]];
			}
		}else{ // Regular parameter
			*buf++=patch_parameters[i];
		}
	}
	for (int i=0; i < PATCH_NAME_SIZE; i++)
		*buf++=char_to_mks50(patch_name[i]);
	*buf++=0xF7;
}

/** Translate chord parameters to apr sysex message
 */
void set_apr_from_chord_notes(const t_byte *chord_notes, t_byte* buf){
	// buf must be at least 14 bytes
	buf+=make_MKS50_sysex_header(buf, midi_channel, OP_APR, CHORD_LEVEL);
	*buf++=0x01; // Group
	for (int i=0; i < CHORD_NOTES; i++)
		*buf++=chord_notes[i];
#ifdef _DEBUG
	printf("Chord: %u %u %u %u %u %u\n", chord_notes[0], chord_notes[1], chord_notes[2], chord_notes[3], chord_notes[4], chord_notes[5]);
#endif
	*buf++=0xF7;
}

void set_tone_parameters_from_bld(const t_byte *nibbles, t_byte *tone_parameters, char *tone_name){
	// Sets 36 parameters and name from 64 nibbles making up 32 bytes
	// No clear reason why it was implemented that way by Roland
	// 36 7-bit values and 10 6-bit characters is smaller than 64 nibbles
	t_byte temp_buf[32];
	t_byte *buf=temp_buf;
	nibbles_to_bytes(nibbles, buf, 32);
	int b00, b01, b02, b03, b04, b05, b06, b07, b08, b09;
	int b10, b11, b12, b13, b14, b15, b16, b17, b18, b19;
	int b20, b21, b22;
	int c0, c1, c2, c3, c4, c5, c6, c7;
	tone_parameters[13] = (*buf >> 4) & 0x0F; // dco_after
	tone_parameters[20] = (*buf++) & 0x0F; // vcf_key_follow
	tone_parameters[21] = (*buf >> 4) & 0x0F; // vcf_after
	tone_parameters[23] = (*buf++) & 0x0F; // vca_after
	tone_parameters[33] = (*buf >> 4) & 0x0F; // env_key_follow
	tone_parameters[35] = (*buf++) & 0x0F; // bender_range
	tone_parameters[11] = (*buf++) & 0x7F; // dco_lfo_mod
	b00=*buf >> 7;
	tone_parameters[12] = (*buf++) & 0x7F; // dco_env_mod
	b01=*buf >> 7;
	tone_parameters[14] = (*buf++) & 0x7F; // dco_pwm_depth
	b02=*buf >> 7;
	tone_parameters[15] = (*buf++) & 0x7F; // dco_pwm_rate
	b03=*buf >> 7;
	tone_parameters[16] = (*buf++) & 0x7F; // vcf_cutoff
	b04=*buf >> 7;
	tone_parameters[17] = (*buf++) & 0x7F; // vcf_reso
	b05=*buf >> 7;
	tone_parameters[19] = (*buf++) & 0x7F; // vcf_env_mod
	b06=*buf >> 7;
	tone_parameters[18] = (*buf++) & 0x7F; // vcf_lfo_mod
	b07=*buf >> 7;
	tone_parameters[22] = (*buf++) & 0x7F; // vca_level
	b08=*buf >> 7;
	tone_parameters[24] = (*buf++) & 0x7F; // lfo_rate
	b09=*buf >> 7;
	tone_parameters[25] = (*buf++) & 0x7F; // lfo_delay
	b10=*buf >> 7;
	tone_parameters[26] = (*buf++) & 0x7F; // env_t1
	b11=*buf >> 7;
	tone_parameters[27] = (*buf++) & 0x7F; // env_l1
	b12=*buf >> 7;
	tone_parameters[28] = (*buf++) & 0x7F; // env_t2
	b13=*buf >> 7;
	tone_parameters[29] = (*buf++) & 0x7F; // env_l2
	b14=*buf >> 7;
	tone_parameters[30] = (*buf++) & 0x7F; // env_t3
	b15=*buf >> 7;
	tone_parameters[31] = (*buf++) & 0x7F; // env_l3
	b16=*buf >> 7;
	tone_parameters[32] = (*buf++) & 0x7F; // env_t4
	get_mks50_name(buf, tone_name, TONE_NAME_SIZE); // tone_name
	b17 = (*buf++) >> 7;
	b18 = (*buf++) >> 7;
	b19 = (*buf++) >> 7;
	b20 = (*buf++) >> 7;
	b21 = (*buf++) >> 7;
	b22 = (*buf++) >> 7;
	c1 = (*buf) >> 7; c0 = ((*buf++) >> 6 & 1);
	c3 = (*buf) >> 7; c2 = ((*buf++) >> 6 & 1);
	c5 = (*buf) >> 7; c4 = ((*buf++) >> 6 & 1);
	c7 = (*buf) >> 7; c6 = ((*buf++) >> 6 & 1);
	// Use the various bits collected above
	tone_parameters[10] = b00; // chorus
	tone_parameters[0] = (b01 << 1) + b02; // dco_env_mode
	tone_parameters[1] = (b03 << 1) + b04; // vcf_env_mode
	tone_parameters[2] = (b05 << 1) + b06; // vca_env_mode
	tone_parameters[5] = (b07 << 2) + (b08 << 1) + b09; // dco_wav_sub
	tone_parameters[4] = (b10 << 2) + (b11 << 1) + b12; // dco_wav_saw
	tone_parameters[3] = (b13 << 1) + b14; // dco_wav_pulse
	tone_parameters[9] = (b15 << 1) + b16; // hpf_cutoff
	tone_parameters[6] = (b17 << 1) + b18; // dco_range
	tone_parameters[7] = (b19 << 1) + b20; // dco_sub_level
	tone_parameters[8] = (b21 << 1) + b22; // dco_noise_level
	tone_parameters[34] =(c6 << 6) + (c5 << 5) + (c4 << 4) + (c3 << 3) + (c2 << 2) + (c1 << 1) + c0; // chorus_rate
}

void set_bld_from_tone_parameters(const t_byte *tone_parameters, const char *tone_name, t_byte *nibbles){
	// Sets 64 nibbles making up 32 bytes from 36 parameters and name
	// No clear reason why it was implemented that way by Roland
	// 36 7-bit values and 10 6-bit characters is smaller than 64 nibbles
	t_byte temp_buf[32];
	t_byte *buf=temp_buf;
	int b00, b01, b02, b03, b04, b05, b06, b07, b08, b09;
	int b10, b11, b12, b13, b14, b15, b16, b17, b18, b19;
	int b20, b21, b22;
	int c0, c1, c2, c3, c4, c5, c6, c7;
	// printf("dco_after=%u vcf_key_follow=%u 0x%02x\n", tone_parameters[13], tone_parameters[20], (tone_parameters[13]<<4) + tone_parameters[20]);
	*buf++ = (tone_parameters[13]<<4) + tone_parameters[20]; // 0 dco_after and vcf_key_follow
	*buf++ = (tone_parameters[21]<<4) + tone_parameters[23]; // 1 vcf_after and vca_after
	*buf++ = (tone_parameters[33] <<4) + tone_parameters[35]; // 2 env_key_follow and bender_range
	*buf++ = tone_parameters[11]; // 3 dco_lfo_mod
	b00 = tone_parameters[10]; // chorus
	*buf++ = tone_parameters[12] + (b00 << 7); // 4 dco_env_mod and chorus
	b01 = tone_parameters[0] >> 1; // dco_env_mode b1
	*buf++ = tone_parameters[14] + (b01<<7); // 5 dco_pwm_depth and dco_env_mode
	b02 = tone_parameters[0] & 1; // dco_env_mode b0
	*buf++ = tone_parameters[15] + (b02 << 7); // 6 dco_pwm_rate and dco_env_mode
	b03 = tone_parameters[1] >> 1; // vcf_env_mode b1
	*buf++ = tone_parameters[16] + (b03 << 7); // 7 vcf_cutoff and vcf_env_mode
	b04 = tone_parameters[1] & 1; // vcf_env_mode b0
	*buf++ = tone_parameters[17] + (b04 << 7); // 8 vcf_reso and vcf_env_mode
	b05 = tone_parameters[2] >> 1; // vca_env_mode b1
	*buf++ = tone_parameters[19] + (b05 << 7); // 9 vcf_env_mod and vca_env_mode
	b06 = tone_parameters[2] & 1; // vca_env_mode b0
	*buf++ = tone_parameters[18] + (b06 << 7); // 10 vcf_lfo_mod and vca_env_mode
	b07 = tone_parameters[5] >> 2; // dco_wav_sub b2
	*buf++ = tone_parameters[22] + (b07 << 7); // 11 vca_level and dco_wav_sub
	b08=(tone_parameters[5] >> 1) & 1; // dco_wav_sub b1
	*buf++ = tone_parameters[24] + (b08 << 7); // 12 lfo_rate and dco_wav_sub
	b09 = tone_parameters[5] & 1; // dco_wav_sub b0
	*buf++ = tone_parameters[25] + (b09 << 7); // 13 lfo_delay and dco_wav_sub
	b10 = tone_parameters[4] >> 2; // dco_wav_saw b2
	*buf++ = tone_parameters[26] + (b10 << 7); // 14 env_t1 and dco_wav_saw
	b11=(tone_parameters[4] >> 1) & 1; // dco_wav_saw b1
	*buf++ = tone_parameters[27] + (b11 << 7); // 15 env_l1 and dco_wav_saw
	b12 = tone_parameters[4] & 1; // dco_wav_saw b0
	*buf++ = tone_parameters[28] + (b12 << 7); // 16 env_t2 and dco_wav_saw
	b13 = tone_parameters[3] >> 1; // dco_wav_pulse b1
	*buf++ = tone_parameters[29] + (b13 << 7); // 17 env_l2 and dco_wav_pulse
	b14 = tone_parameters[3] & 1; // dco_wav_pulse b0
	*buf++ = tone_parameters[30] + (b14 << 7); // 18 env_t3 and dco_wav_pulse
	b15 = tone_parameters[9] >> 1; // hpf_cutoff b1
	*buf++ = tone_parameters[31] + (b15 << 7); // 19 env_l3 and hpf_cutoff
	b16 = tone_parameters[9] & 1; // hpf_cutoff b0
	*buf++ = tone_parameters[32] + (b16 << 7); // 20 env_t4 and hpf_cutoff
	// printf("hpf_cutoff=%u env_l3=%u env_t4=%u %p 0x%02x 0x%02x\n",
	//  tone_parameters[9], tone_parameters[31], tone_parameters[32], buf-2, (unsigned int)*(buf-2), (unsigned int)*(buf-1));
	set_mks50_name(tone_name, buf, TONE_NAME_SIZE); // 21..30 tone_name
	// hex_dump(temp_buf, 32);
	b17 = tone_parameters[6] >> 1; // dco_range b1
	b18 = tone_parameters[6] & 1; // dco_range b0
	// printf("dco_range=%u b1=%u b0=%u 0x%02x 0x%02x\n", tone_parameters[6], b17, b18, *buf, *(buf+1));
	b19 = tone_parameters[7] >> 1; // dco_sub_level b1
	b20 = tone_parameters[7] & 1; // dco_sub_level b0
	b21 = tone_parameters[8] >>1; // dco_noise_level b1
	b22 = tone_parameters[8] & 1; // dco_noise_level bo
	// chorus_rate
	c0 = tone_parameters[34] & 1;
	c1 = (tone_parameters[34]>>1) & 1;
	c2 = (tone_parameters[34]>>2) & 1;
	c3 = (tone_parameters[34]>>3) & 1;
	c4 = (tone_parameters[34]>>4) & 1;
	c5 = (tone_parameters[34]>>5) & 1;
	c6 = (tone_parameters[34]>>6) & 1;
	c7 = (tone_parameters[34]>>7) & 1;
	// Merge extra bits with tone name
	buf[0] += b17 << 7;
	buf[1] += b18 << 7;
	buf[2] += b19 << 7;
	buf[3] += b20 << 7;
	buf[4] += b21 << 7;
	buf[5] += b22 << 7;
	buf[6] += (c1 << 7) + (c0 << 6);
	buf[7] += (c3 << 7) + (c2 << 6);
	buf[8] += (c5 << 7) + (c4 << 6);
	buf[9] += (c7 << 7) + (c6 << 6);
	// printf("0x%02x 0x%02x\n", *buf, *(buf+1));
	// printf("0x%02x 0x%02x\n", buf[0], buf[1]);
	buf += TONE_NAME_SIZE;
	*buf = 0; // Tone data code
	// hex_dump(temp_buf, 32); printf("\n");
	bytes_to_nibbles(temp_buf, nibbles, 32);
	// printf("1) at %p:\n", nibbles);	hex_dump(nibbles, 64); printf("\n");
}

void set_tones_from_chunk(const t_byte *chunk, Tone_bank * tone_bank){
	// A chunk holds 4 sets of tone parameters
	// each coded as 64 nibbles
	t_byte *tone_parameters;
	char *tone_name;
	int program=chunk[8]; // First tone number in chunk
	for (int i=0; i<4; i++, program++){
		tone_parameters=tone_bank->parameters[program];
		tone_name=tone_bank->names[program];
		set_tone_parameters_from_bld(chunk+9+64*i, tone_parameters, tone_name);
		printf("Got tone #%u %s\n", program, tone_name);
	}
}

void set_patch_parameters_from_bld(const t_byte *nibbles, t_byte *patch_parameters, char *patch_name){
	// Sets 13 parameters and name from 64 nibbles making up 32 bytes
	// No clear reason why it was implemented that way by Roland
	t_byte temp_buf[32];
	t_byte *buf=temp_buf;
	nibbles_to_bytes(nibbles, buf, 32);
	int b00, b01, b02, b03;
	patch_parameters[0] = *buf++; // tone_number
    patch_parameters[1] = *buf++; // key_low
    patch_parameters[2] = *buf++; // key_high
    patch_parameters[3] = *buf++; // porta_time
    // patch_parameters[] = *buf++; // #04 see b03 porta_switch
    patch_parameters[5] = *buf++; // mod_sens
    patch_parameters[6] = *buf++; // key_shift
    patch_parameters[7] = *buf++; // volume
    patch_parameters[8] = *buf++; // detune
    // patch_parameters[] = *buf++; // #09 see below midi_flags
    patch_parameters[10] = *buf>>4 ; // mono_bender_range
    patch_parameters[11] = *buf++ & 0x0F; // chord_memory
    patch_parameters[9] = *buf++; // This parameter appears out of sequence midi_flags
    b00 = (*buf >> 7) & 1; // Exp. mode not in patch parameters??
    b01 = (*buf >> 6) & 1;
    b02 = (*buf >> 5) & 1;
    b03 = (*buf++ >> 4) & 1;
    patch_parameters[12] = (b01 << 1) + b02; // key_assign_mode
    patch_parameters[4] = b03; // porta_switch
	get_mks50_name(buf, patch_name, PATCH_NAME_SIZE); // patch_name
}

void set_bld_from_patch_parameters(const t_byte *patch_parameters, const char *patch_name, t_byte *nibbles){
	// Sets 64 nibbles making up 32 bytes from 13 parameters and name
	// No clear reason why it was implemented that way by Roland
	t_byte temp_buf[32];
	t_byte *buf=temp_buf;
	int b00, b01, b02, b03;
	*buf++ = patch_parameters[0]; // tone_number
    *buf++ = patch_parameters[1]; // key_low
    *buf++ = patch_parameters[2]; // key_high
    *buf++ = patch_parameters[3]; // porta_time
    // *buf++ = patch_parameters[]; // #04 see b03 porta_switch
    *buf++ = patch_parameters[5]; // mod_sens
    *buf++ = patch_parameters[6]; // key_shift
    *buf++ = patch_parameters[7]; // volume
    *buf++ = patch_parameters[8]; // detune
    // *buf++ = patch_parameters[]; // #09 see below midi_flags
    *buf++ = (patch_parameters[10] << 4) + patch_parameters[11]; // mono_bender_range and chord_memory
    *buf++ = patch_parameters[9]; // This parameter appears out of sequence midi_flags
    // Patch A18 ElectroDrm has assign mode mono, 0x60 in original bulk dump at offset 0x1E8
    // printf("%u\n", patch_parameters[12]); // 3 for ElectroDrm
    b00 = 0; // (*buf >> 7) & 1; // Exp. mode not in patch parameters ??
    b01 = (patch_parameters[12] >> 1) & 1; // key_assign_mode
    b02 = patch_parameters[12] & 1; // key_assign_mode
    b03 = patch_parameters[4] & 1; // porta_switch
    *buf++= (b00 << 7) + (b01 << 6) + (b02 << 5) + (b03 << 4);
	set_mks50_name(patch_name, buf, PATCH_NAME_SIZE); // patch_name
	buf+=PATCH_NAME_SIZE;
    // Pad with zeroes
    memset(buf, 0, 10);
    buf+=10;
    *buf=0x10; // Patch data code
	bytes_to_nibbles(temp_buf, nibbles, 32); // 64 nibbles
}

void set_patches_from_chunk(const t_byte *blk, Patch_bank * patch_bank){
	// A chunk holds 4 sets of patch parameters
	t_byte *patch_parameters;
	char *patch_name;
	int program=blk[8]; // First patch number in chunk
	for (int i=0; i<4; i++, program++){
		patch_parameters=patch_bank->parameters[program];
		patch_name=patch_bank->names[program];
		set_patch_parameters_from_bld(blk+9+64*i, patch_parameters, patch_name);
		printf("Got patch #%u %s\n", program, patch_name);
	}
}

void set_chunk_from_patches(const Patch_bank * patch_bank, t_byte *blk, int program ){
	// A chunk holds 4 sets of patch parameters
	// blk must be able to hold 266 bytes
	const t_byte *patch_parameters;
	const char *patch_name;
	if (program>60 || program%4){
		fprintf(stderr, "ERROR: illegal start program #%u", program);
		return;
	}
	make_MKS50_sysex_header(blk, midi_channel, OP_BLD, PATCH_LEVEL);
	blk[6]=1; // Group
	blk[7]=0; // Extension of program
	blk[8] = program; // First patch number in chunk
	for (int i=0; i<4; i++, program++){
		patch_parameters=patch_bank->parameters[program];
		patch_name=patch_bank->names[program];
		// printf("About to set patch #%u %s\n", program, patch_name);
		set_bld_from_patch_parameters(patch_parameters, patch_name, blk+9+64*i);
		printf("Set patch #%u %s\n", program, patch_name);
	}
	blk[265]=0xF7; // End of exclusive
}

void set_chunk_from_tones(const Tone_bank * tone_bank, t_byte *blk, int program ){
	// A chunk holds 4 sets of tone parameters
	// blk must be able to hold 266 bytes
	const t_byte *tone_parameters;
	const char *tone_name;
	if (program>60 || program%4){
		fprintf(stderr, "ERROR: illegal start program #%u", program);
		return;
	}
	make_MKS50_sysex_header(blk, midi_channel, OP_BLD, TONE_LEVEL);
	blk[6]=1; // Group
	blk[7]=0; // Extension of program
	blk[8] = program; // First tone number in chunk
	for (int i=0; i<4; i++, program++){
		tone_parameters=tone_bank->parameters[program];
		tone_name=tone_bank->names[program];
		// printf("About to set tone #%u %s\n", program, tone_name);
		set_bld_from_tone_parameters(tone_parameters, tone_name, blk+9+64*i);
		// hex_dump(blk+9+64*i, 64);
		printf("Set tone #%u %s\n", program, tone_name);
	}
	// printf("2) at %p:\n", blk+9); hex_dump(blk+9, 64);
	// printf("at %p\n:", blk); hex_dump(blk, 64);
	blk[265]=0xF7; // End of exclusive
}

void set_chord_memory_from_bld(const t_byte *buf, t_byte chord_memory[16][6]){
	for(int i=0; i<16; i++){
		nibbles_to_bytes(buf+9+2*CHORD_NOTES*i, chord_memory[i], CHORD_NOTES);
		// Unlike apr which uses the actual midi not number, bld uses an offset
		// 0 is C4, range is -24..+24 signed 7-bit value, 127 means off
		for(int j=0; j<CHORD_NOTES; j++)
			// For some reason, 7F is represented as FF in bulk dump ??
			if(chord_memory[i][j]==0xFF){
				chord_memory[i][j]=0x7F;
			}else{
				chord_memory[i][j]=60+chord_memory[i][j]-(chord_memory[i][j]>0x3f?-128:0);
			}
	}
}

int sprint_chord(char *s, const t_byte * chord){
	// S must be able to hold at least 30 chars (6 notes, 5 chars max per note)
	// This maps to C2..C6
	char *s0=s;
	int note;
	for(int i=0; i<CHORD_NOTES; i++){
		if (chord[i]<127){ // dont' print special value 127 (or above)
			note=chord[i];
			s0+=sprint_note(s0, note);		
		}else{
			s0+=sprintf(s0, "off ");
		}
	}
	return(s0-s);
}

void redraw_wave_parameter(){
	if (tone_parameters[3]==3 || tone_parameters[4]==3){ // These wave shapes allow PWM
		vs_pulse_width->color(FL_BACKGROUND_COLOR);
		vs_pulse_width->textcolor(FL_FOREGROUND_COLOR);
		vs_pulse_mod_rate->color(FL_BACKGROUND_COLOR);
		vs_pulse_mod_rate->textcolor(FL_FOREGROUND_COLOR);
	}else{ // PWM control has no effect
		vs_pulse_width->color(FL_DARK1);
		vs_pulse_width->textcolor(FL_DARK3);
		vs_pulse_mod_rate->color(FL_DARK1);
		vs_pulse_mod_rate->textcolor(FL_DARK3);
	}
	vs_pulse_width->redraw();
	vs_pulse_mod_rate->redraw();
}

void redraw_chorus_parameter(){
	if (tone_parameters[10]){ // Chorus on
		vs_chorus_rate->color(FL_BACKGROUND_COLOR);
		vs_chorus_rate->textcolor(FL_FOREGROUND_COLOR);
	}else{ // Chorus off
		vs_chorus_rate->color(FL_DARK1);
		vs_chorus_rate->textcolor(FL_DARK3);
	}
	vs_chorus_rate->redraw();
}

void redraw_tone_parameters(){
//	Fl::lock();
	for (int i=0; i < TONE_PARMS; i++){
		if (tone_controls[i]){
			tone_controls[i]->value(tone_parameters[i]);
			// PW and PWM need color change TODO ??
			tone_controls[i]->redraw();
#ifdef _DEBUG											
			printf("Redraw tone parameter #%u\n", i);
#endif											
		}
	}
	// The following parameters require special drawing
	redraw_wave_parameter();
	redraw_chorus_parameter();

	if(txt_tone_name){
		txt_tone_name->value(tone_name);
		txt_tone_name->redraw();
	}
	if(txt_tone_num){
		char s[4];
		sprintf(s, "%u", patch_parameters[0]);
		txt_tone_num->value(s);
		txt_tone_num->redraw();
	}
	Fl::awake();
//	Fl::unlock();
}

void set_tone_parameters_from_apr(const t_byte *buf, t_byte *tone_parameters, char *tone_name){
	// Translate apr to tone data and display
	// buf points to sysex APR data, past 7 bytes sysex header
	for (int i=0; i < TONE_PARMS; i++){
		tone_parameters[i]=buf[i];
	}
	for (int i=0; i < TONE_NAME_SIZE; i++)
		tone_name[i]=mks50_chars[buf[i+TONE_PARMS]];
	tone_name[TONE_NAME_SIZE]=0;
	printf("Got all tone parameters for %s\n", tone_name);
	redraw_tone_parameters();
}

void redraw_keymode_parameter(){
	if(patch_parameters[12]==1){ // Key mode is "chord"
		((Fl_Value_Slider *)patch_controls[11])->color(FL_BACKGROUND_COLOR);
		((Fl_Value_Slider *)patch_controls[11])->textcolor(FL_FOREGROUND_COLOR);
	}else{
		((Fl_Value_Slider *)patch_controls[11])->color(FL_DARK1);
		((Fl_Value_Slider *)patch_controls[11])->textcolor(FL_DARK3);
	}
	patch_controls[11]->redraw();
}

void redraw_porta_switch_parameter(){
	if(patch_parameters[4]==1){ // Portamento on
		vs_porta_time->color(FL_BACKGROUND_COLOR);
		vs_porta_time->textcolor(FL_FOREGROUND_COLOR);
	}else{ // Portamento off
		vs_porta_time->color(FL_DARK1);
		vs_porta_time->textcolor(FL_DARK3);
	}
	vs_porta_time->redraw();
}

void redraw_note_parameter(Fl_Widget* o, t_byte note){
//	Fl::lock();
	if(txt_chord){
		static char s[5];
		sprint_note(s, note);
		((Fl_Input*)o)->value(s);
		o->redraw();
	}
	Fl::awake();
//	Fl::unlock();
}

void redraw_key_shift_parameter(){
//	Fl::lock();
	if(txt_key_shift){
		static char s[5];
		int v=patch_parameters[6];
		if(v>0x3F) v-=0x80;
		sprintf(s, "%i", v);
		txt_key_shift->value(s);
		txt_key_shift->redraw();
	}
	Fl::awake();
//	Fl::unlock();
}

void redraw_patch_parameters(){
//	Fl::lock();
	for (int i=0; i < PATCH_PARMS; i++){
		if (patch_controls[i]){
			patch_controls[i]->value(patch_parameters[i]);
			patch_controls[i]->redraw();
#ifdef _DEBUG											
			printf("Redraw patch parameter #%u\n", i);
		}else{
			fprintf(stderr, "No control for patch parameter #%u\n", i);
#endif											
		}
	}
	// The following parameters require special drawing
	redraw_keymode_parameter();	// chord memory needs color update
	redraw_porta_switch_parameter(); // porta time needs color update
	redraw_note_parameter(txt_key_low, patch_parameters[1]);
	redraw_note_parameter(txt_key_high, patch_parameters[2]);
	redraw_key_shift_parameter();
	
	if(txt_patch_name){
		txt_patch_name->value(patch_name);
		txt_patch_name->redraw();
#ifdef _DEBUG											
		printf("Redraw patch name \"%s\"\n", patch_name);
#endif											
	}
	if(txt_chord && chord_memory_valid){
		// Assume chord memory contents has not been changed on the mks-50
		static char s[40];
		sprint_chord(s, chord_memory[patch_parameters[11]]);
		txt_chord->value(s);
		txt_chord->redraw();
#ifdef _DEBUG											
		printf("Redraw chord \"%s\"\n", s);
#endif											
	}
	Fl::awake();
//	Fl::unlock();
}

void set_patch_parameters_from_apr(const t_byte *buf, t_byte *patch_parameters, char *patch_name){
	// Translate apr to patch data and display
	// buf points to sysex APR data, past 7 bytes sysex header
	t_byte j;
	for (int i=0; i < PATCH_PARMS; i++){
		if(i==12){ // Key mode parameter needs special mapping
			if(buf[i]==key_mode_map[0]){ j=0;
			}else if(buf[i]==key_mode_map[1]){ j=1;
			}else if(buf[i]==key_mode_map[2]){ j=2;
			}else {
				fprintf(stderr,"ERROR: invalid key mode 0x%02x\n", buf[i]);
				j=0; // Fallback to poly
			}
			// printf("0x%02x -> 0x%02x\n", buf[i], j);			
			patch_parameters[i]=j;
		}else{ // Regular parameter
			patch_parameters[i]=buf[i];
		}
	}
	for (int i=0; i < PATCH_NAME_SIZE; i++)
		patch_name[i]=mks50_chars[buf[i+PATCH_PARMS]];
	patch_name[PATCH_NAME_SIZE]=0;
	printf("Got all patch parameters for %s\n", patch_name);
	redraw_patch_parameters();
	// printf("chord %u\n",patch_parameters[11]);
}

void redraw_chord_notes(){
//	Fl::lock();
	if(txt_chord){
		static char s[5*CHORD_NOTES]; // "C#-1 " -> 5 chars
		sprint_chord(s, chord_notes);
		txt_chord->value(s);
		txt_chord->redraw();
	}
	Fl::awake();
//	Fl::unlock();
}

void set_chord_notes_from_apr(const t_byte *buf, t_byte *chord_notes){
	// Translate apr to chord data and display
	// buf points to sysex APR data, past 7 bytes sysex header
	for (int i=0; i < CHORD_NOTES; i++){
		chord_notes[i]=buf[i];
	}
	printf("Got all chord notes\n");
	redraw_chord_notes();
}

int parse_MKS50_sysex_header(const t_byte *sysex, int &channel, int &op, int &level){
	// The 6 first bytes are common to all sysexes
	// The 7th byte is usually group and set to 1, but not always
	if(sysex[0]!=0xF0) {fprintf(stderr, "ERROR: No sysex header\n"); return(-1);}
	if(sysex[1]!=0x41) {fprintf(stderr, "ERROR: Not a Roland sysex\n"); return(-1);}
	if(sysex[4]!=0x23){fprintf(stderr, "ERROR: Not an MKS-50 sysex\n"); return(-1);}
	op=sysex[2];
	channel=sysex[3];
	level=sysex[5];
	return(0);
}

////////////////////////////
// User interface classes //
////////////////////////////

class PatchTable : public Fl_Table
{
protected:
	void draw_cell(TableContext context,
				int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);
	static void event_callback(Fl_Widget*, void*);
	void event_callback2(Patch_bank *pb);	// callback for table events
	Patch_bank *_patches;

public:
	PatchTable(int x, int y, int w, int h, const char *l=0) : Fl_Table(x,y,w,h,l)
	{
		callback(&event_callback, (void*)this);
		end(); // Fl_Table derives from Fl_Group, so end() it
		rows(8);
		cols(8);
	}
	~PatchTable() { }
	
	Patch_bank *patches() {return _patches;}
	void patches(Patch_bank *p) {_patches=p;}
};

PatchTable *patch_table_a, *patch_table_b;

int rename_patch(unsigned int patch_num){
	Patch_bank *patch_bank;
	patch_bank=patch_num>63 ? &patch_bank_b : &patch_bank_a;
	int program=patch_num & 0x3F;
	
	if(!patch_bank->valid){
		show_err("ERROR: patch data not set");
		return(-1);
	}
	const char *new_name=fl_input("Please enter new patch name", patch_bank->names[program]);
	if (new_name){
		strncpy(patch_bank->names[program], new_name, PATCH_NAME_SIZE);
		if (patch_num>63) patch_table_b->redraw();
		else patch_table_a->redraw();
		// if (strlen(new_name)<PATCH_NAME_SIZE)
		//	patch_bank->names[program][strlen(new_name)]=' ';
		patch_bank->changed=true;
	}
}


void PatchTable::draw_cell(TableContext context, 
			  int R, int C, int X, int Y, int W, int H)
{
	static char s[40];
	if(R<0 || R>=8 || C<0 || C>=8 ) return;
	
	switch ( context )
	{
	case CONTEXT_STARTPAGE:
		fl_font(FL_HELVETICA, 12);
		return;

	case CONTEXT_CELL:
		// BG COLOR
		/*
		if (C==_selected_col && R==_selected_row){
			fl_color((C&1) ? FL_RED: _BGCOLOR);

		}else if (Speicher.getSoundName(m).length()==0){
			fl_color((C&1) ? FL_BLACK: _BGCOLOR);
		}else{
			fl_color((C&1) ? FL_WHITE: _BGCOLOR);
		}
		*/
		fl_color(FL_WHITE); // : _BGCOLOR);
		fl_push_clip(X, Y, W, H);
		fl_rectf(X, Y, W, H);

		// TEXT
		fl_color(FL_BLACK);
		fl_draw(patches()->names[R*8+C], X+1, Y, W-1, H, FL_ALIGN_CENTER);

		// BORDER
		fl_color(color()); 
		fl_rect(X, Y, W, H);

		fl_pop_clip();
		return;

	case CONTEXT_TABLE:
#ifdef _DEBUG
	    fprintf(stderr, "TABLE CONTEXT CALLED\n");
	    return;
#endif
	case CONTEXT_ROW_HEADER:
	case CONTEXT_COL_HEADER:
	case CONTEXT_ENDPAGE:
	case CONTEXT_RC_RESIZE:
	case CONTEXT_NONE:
	    return;
    }
}

void PatchTable::event_callback(Fl_Widget* t, void *data)
{
	PatchTable *o = (PatchTable*)data;
	o->event_callback2(((PatchTable *)t)->patches());
}

void PatchTable::event_callback2(Patch_bank *pb)
{
	switch(Fl::event()){
	case FL_PUSH:
		int R = callback_row(), C = callback_col();
		int program=8*R+C+pb->program_offset;
#ifdef _DEBUG
		printf("Click %d %d -> %d store patch btn %u\n", R, C, program, btn_store_patch->value());
#endif		
		if (btn_store_patch->value()){
			if(pb->valid && current_patch_valid){
				store_patch(program, false);
			}else{
				show_err("No patch data, cannot store!");
			}
			btn_store_patch->clear();
		}else if (btn_rename_patch->value()){
			rename_patch(program);
			btn_rename_patch->clear();
		}else{
			recall_patch(program);
		}
		break;
	}

}

class ToneTable : public Fl_Table
{
protected:
	void draw_cell(TableContext context, // table cell drawing
				int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);
	static void event_callback(Fl_Widget*, void*);
	void event_callback2(Tone_bank *pb);	// callback for table events
	Tone_bank *_tones;

public:

	ToneTable(int x, int y, int w, int h, const char *l=0) : Fl_Table(x,y,w,h,l)
	{
		callback(&event_callback, (void*)this);
		end(); // Fl_Table derives from Fl_Group, so end() it
		rows(8);
		cols(8);
	}
	
	~ToneTable() { }
	
	Tone_bank *tones() {return _tones;}
	void tones(Tone_bank *p) {_tones=p;}
};

ToneTable *tone_table_a, *tone_table_b;

int rename_tone(unsigned int tone_num){
	Tone_bank *tone_bank;
	tone_bank=tone_num>63 ? &tone_bank_b : &tone_bank_a;
	int program=tone_num & 0x3F;
	
	if(!tone_bank->valid){
		fprintf(stderr, "ERROR: tone data not set\n");
		return(-1);
	}
	const char *new_name=fl_input("Please enter new tone name", tone_bank->names[program]);
	if (new_name){
		// memset(tone_bank->names[program], ' ', PATCH_NAME_SIZE)
		strncpy(tone_bank->names[program], new_name, PATCH_NAME_SIZE);
		if (tone_num>63) tone_table_b->redraw();
		else tone_table_a->redraw();
		// if (strlen(new_name)<PATCH_NAME_SIZE)
		//	tone_bank->names[program][strlen(new_name)]=' ';
		tone_bank->changed=true;
	}
}

void ToneTable::event_callback(Fl_Widget* t, void *data)
{
	ToneTable *o = (ToneTable*)data;
	o->event_callback2(((ToneTable *)t)->tones());
}

void ToneTable::event_callback2(Tone_bank *pb)
{
	switch(Fl::event()){
	case FL_PUSH:
		int R = callback_row(), C = callback_col();
		int program=8*R+C+pb->program_offset;
#ifdef _DEBUG		
		printf("Click %d %d -> %d store tone btn %u\n", R, C, program, btn_store_tone->value());
#endif		
		if (btn_store_tone->value()){
			if(pb->valid && current_patch_valid){
				store_tone(program, false);
			}else{
				show_err("No tone data, cannot store!");
			}
			btn_store_tone->clear();
		}else if (btn_rename_tone->value()){
			rename_tone(program);
			btn_rename_tone->clear();
		}else{
			recall_tone(program);
		}
		break;
	}
}

void ToneTable::draw_cell(TableContext context, 
			  int R, int C, int X, int Y, int W, int H)
{
	static char s[40];
	if(R<0 || R>=8 || C<0 || C>=8 ) return;
	
	switch ( context )
	{
	case CONTEXT_STARTPAGE:
		fl_font(FL_HELVETICA, 12);
		return;

	case CONTEXT_CELL:
		// BG COLOR
		/*
		if (C==_selected_col && R==_selected_row){
			fl_color((C&1) ? FL_RED: _BGCOLOR);

		}else if (Speicher.getSoundName(m).length()==0){
			fl_color((C&1) ? FL_BLACK: _BGCOLOR);
		}else{
			fl_color((C&1) ? FL_WHITE: _BGCOLOR);
		}
		*/
		fl_color(FL_WHITE); // : _BGCOLOR);
		fl_push_clip(X, Y, W, H);
		fl_rectf(X, Y, W, H);

		// TEXT
		fl_color(FL_BLACK);
		fl_draw(tones()->names[R*8+C], X+1, Y, W-1, H, FL_ALIGN_CENTER);

		// BORDER
		fl_color(color()); 
		fl_rect(X, Y, W, H);

		fl_pop_clip();
		return;

	case CONTEXT_TABLE:
#ifdef _DEBUG	
	    fprintf(stderr, "TABLE CONTEXT CALLED\n");
	    return;
#endif
	case CONTEXT_ROW_HEADER:
	case CONTEXT_COL_HEADER:
	case CONTEXT_ENDPAGE:
	case CONTEXT_RC_RESIZE:
	case CONTEXT_NONE:
	    return;
    }
}

class ChordTable : public Fl_Table
{
protected:
	void draw_cell(TableContext context, // table cell drawing
				int R=0, int C=0, int X=0, int Y=0, int W=0, int H=0);
	static void event_callback(Fl_Widget*, void*);
	void event_callback2(int offset);	// callback for table events
	int _offset=0;

public:

	ChordTable(int x, int y, int w, int h, const char *l=0) : Fl_Table(x,y,w,h,l)
	{
		row_header(1);
		rows(8);
		cols(1);
		callback(&event_callback, (void*)this);
		end(); // Fl_Table derives from Fl_Group, so end() it
	}
	~ChordTable() { }
	int offset() {return _offset;}
	void offset(int i) {_offset=i;}
	
};

ChordTable *chord_table_1, *chord_table_2;

int store_chord(unsigned int chord_num, bool forced){
	// Implicit source is edit buffer

    // Don't allow setting a chord when the chord memory has not been init'ed
    // This would create a partly valid chord memory
	if(!forced &&(!chord_memory_valid || !current_chord_valid)){
		fprintf(stderr,"ERROR: chord data not set\n");
		return(-1);
	}
	if(chord_num<0 or chord_num>=CHORDS){
		fprintf(stderr,"ERROR: illegal chord number %u\n", chord_num);
		return(-1);
	}
	
	printf("Storing to chord #%u\n", chord_num);

	if (!forced) patch_parameters[11]=chord_num; // Make this chord current
	memcpy(chord_memory[chord_num], chord_notes, CHORD_NOTES);

	if(chord_num>7){
		if(chord_table_2) chord_table_2->redraw();
	}else{
		if(chord_table_1) chord_table_1->redraw();
	}
	chord_memory_changed=true;
	return(0);
}

void ChordTable::event_callback(Fl_Widget* t, void *data)
{
	ChordTable *o = (ChordTable*)data;
	o->event_callback2(((ChordTable*)t)->offset());
}

void ChordTable::event_callback2(int offset)
{
	switch(Fl::event()){
	case FL_PUSH:
		int R = callback_row(), C = callback_col();
		int chord_num=R+offset;
		if(chord_num<0 or chord_num>15){
			show_err("Illegal chord number %u", chord_num);
			return;
		}
#ifdef _DEBUG		
		printf("Click %d %d -> %d store chord btn %u\n", R, C, chord_num, btn_store_chord->value());
#endif		
		if (btn_store_chord->value()){
			if(chord_memory_valid && current_chord_valid){
				store_chord(chord_num, false);
			}else{
				show_err("No chord data, cannot store!");
			}
			btn_store_chord->clear();
		}else{
			recall_chord(chord_num);
		}
		break;
	}
}

void ChordTable::draw_cell(TableContext context, 
			  int R, int C, int X, int Y, int W, int H)
{
	static char s[40];
	if(R<0 || R>=8 || C<0 || C>=8 ) return;
	
	switch ( context )
	{
	case CONTEXT_STARTPAGE:
		fl_font(FL_HELVETICA, 12);
		return;

	case CONTEXT_CELL:
		// BG COLOR
		/*
		if (C==_selected_col && R==_selected_row){
			fl_color((C&1) ? FL_RED: _BGCOLOR);

		}else if (Speicher.getSoundName(m).length()==0){
			fl_color((C&1) ? FL_BLACK: _BGCOLOR);
		}else{
			fl_color((C&1) ? FL_WHITE: _BGCOLOR);
		}
		*/
		fl_color(FL_WHITE); // : _BGCOLOR);
		fl_push_clip(X, Y, W, H);
		fl_rectf(X, Y, W, H);

		// TEXT
		fl_color(FL_BLACK);
		sprint_chord(s, chord_memory[R+offset()]);
		fl_draw(s, X+1, Y, W-1, H, FL_ALIGN_LEFT);

		// BORDER
		fl_color(color()); 
		fl_rect(X, Y, W, H);

		fl_pop_clip();
		return;

	case CONTEXT_ROW_HEADER:
		fl_color(FL_WHITE); // _BGCOLOR not in scope??
		fl_push_clip(X, Y, W, H);
		fl_rectf(X, Y, W, H);

		// TEXT
		fl_color(FL_BLACK);
		sprintf(s, "%d", R+offset()); // text for sound number cell
		fl_draw(s, X+1, Y, W-1, H, FL_ALIGN_LEFT);

		// BORDER
		fl_color(color()); 
		fl_rect(X, Y, W, H);

		fl_pop_clip();
	    return;

	case CONTEXT_TABLE:
	case CONTEXT_COL_HEADER:
	case CONTEXT_ENDPAGE:
	case CONTEXT_RC_RESIZE:
	case CONTEXT_NONE:
	    return;
    }
}

void set_chord_memory_valid(bool b);
void set_tone_bank_valid(bool b, Tone_bank &tone_bank, Fl_Widget* tone_table);
void set_patch_bank_valid(bool b, Patch_bank &patch_bank, Fl_Widget* patch_table);

////////////////////////////
// MIDI related functions //
////////////////////////////

// Some messages can be split over two events (not more with MKS-50)
// Therefore we need an extra buffer that survives alsa_midi_process calls
t_byte bulk_buffer[PATCH_BLD_SIZE>TONE_BLD_SIZE?PATCH_BLD_SIZE:TONE_BLD_SIZE];
t_byte bulk_chord_buffer[CHORD_BLD_SIZE];
t_byte *bulk_ptr;
static bool bulk_pending=false; // true when a bulk dump spans several events
static bool is_tone_bank_b=false;
static bool is_patch_bank_b=false;

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

int aconnect(char *sender_id, char *dest_id)
{
	// Slightly adapted from ftp://ftp.alsa-project.org/pub/utils/
	int queue = 0, convert_time = 0, convert_real = 0, exclusive = 0;

	snd_seq_t *seq;
	int list_perm = 0;
	int client;
	int list_subs = 0;
	snd_seq_port_subscribe_t *subs;
	snd_seq_addr_t sender, dest;

	if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
		fprintf(stderr, "can't open sequencer\n");
		return 1;
	}
	
	/* connection or disconnection */

	if ((client = snd_seq_client_id(seq)) < 0) {
		snd_seq_close(seq);
		fprintf(stderr, "can't get client id\n");
		return 1;
	}

	/* set client info */
	if (snd_seq_set_client_name(seq, "ALSA Connector") < 0) {
		snd_seq_close(seq);
		fprintf(stderr, "can't set client info\n");
		return 1;
	}

	/* set subscription */
	if (snd_seq_parse_address(seq, &sender, sender_id) < 0) {
		snd_seq_close(seq);
		fprintf(stderr, "invalid sender address %s\n", sender_id);
		return 1;
	}
	if (snd_seq_parse_address(seq, &dest, dest_id) < 0) {
		snd_seq_close(seq);
		fprintf(stderr, "invalid destination address %s\n", dest_id);
		return 1;
	}
	snd_seq_port_subscribe_alloca(&subs);
	snd_seq_port_subscribe_set_sender(subs, &sender);
	snd_seq_port_subscribe_set_dest(subs, &dest);
	snd_seq_port_subscribe_set_queue(subs, queue);
	snd_seq_port_subscribe_set_exclusive(subs, exclusive);
	snd_seq_port_subscribe_set_time_update(subs, convert_time);
	snd_seq_port_subscribe_set_time_real(subs, convert_real);

	if (snd_seq_get_port_subscription(seq, subs) == 0) {
		snd_seq_close(seq);
		fprintf(stderr, "Connection is already subscribed\n");
		return 1;
	}
	if (snd_seq_subscribe_port(seq, subs) < 0) {
		snd_seq_close(seq);
		fprintf(stderr, "Connection failed (%s)\n", snd_strerror(errno));
		return 1;
	}

	snd_seq_close(seq);

	return 0;
}

static int get_port_by_name(snd_seq_t *seq, const char * port_name, char *port_id, int len) // , int perm)
{
	// Slightly adapted from ftp://ftp.alsa-project.org/pub/utils/
	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;
	const char * current_port_name;
	bool found=false;

	snd_seq_client_info_alloca(&cinfo);
	snd_seq_port_info_alloca(&pinfo);
	snd_seq_client_info_set_client(cinfo, -1);
	while (!found && snd_seq_query_next_client(seq, cinfo) >= 0) {
		/* reset query info */
		snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
		snd_seq_port_info_set_port(pinfo, -1);
		int client=snd_seq_client_info_get_client(cinfo);
		while (!found && snd_seq_query_next_port(seq, pinfo) >= 0) {
			/*
			if (check_permission(pinfo, perm)) {
				do_action(seq, cinfo, pinfo, count);
				count++;
			}
			*/
			current_port_name=snd_seq_port_info_get_name(pinfo);
			if(!strcmp(port_name, snd_seq_port_info_get_name(pinfo))){
				int port=snd_seq_port_info_get_port(pinfo);
				printf("Found \"%s\" at \"%d:%d\"\n", port_name, client, port);
				snprintf(port_id, len, "%d:%d", client, port);
				found=true;
			}
		}
	}
	return(!found);
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
	int status, remaining=len;
	// Send buffer one chunk at a time, with throttling
	// It seems the MKS-50 cannot cope when sending all at once
	int chunk=266;
	// chunk 256 or 266 usleep 1000000 ok
	// chunk 266 usleep 100000 ok
	while (remaining>chunk){
		ev.data.ext.len = chunk;
		ev.data.ext.ptr = sysex;	
		// printf("sysex %02x..%02x\n", sysex[0], sysex[chunk-1]);
		status = snd_seq_event_output_direct(seq_handle, &ev);
		sysex+=chunk;
		remaining -=chunk;
		usleep(10000); // 10000 ok for MKS-50
	}
	if(remaining){
		ev.data.ext.len = remaining;
		ev.data.ext.ptr = sysex;	
		status = snd_seq_event_output_direct(seq_handle, &ev);
	}
#ifdef _DEBUG		
	printf("send_sysex: Output status %i (len %u)\n", status, len);
	// status=len+28 ?? 202 -> 230, 4256 -> 4284
	// we do get 202 and 4256 in kmidimon
#endif
	return(status);
}

static void send_current_tone(){
	// Sends current tone over MIDI
	t_byte buf[TONE_APR_SIZE];
	if(current_tone_valid){
		set_apr_from_tone_parameters(tone_parameters, tone_name, buf);
		send_sysex(buf,TONE_APR_SIZE);
		printf("Sent current tone\n");
	}else{
		fprintf(stderr, "No tone parameters, cannot send\n");
	}
}

static void send_current_chord(){
	// Sends current chord over MIDI
	t_byte buf[CHORD_APR_SIZE];
	if(current_chord_valid){
		set_apr_from_chord_notes(chord_notes, buf);
		send_sysex(buf,CHORD_APR_SIZE);
		printf("Sent current chord\n");
	}else{
		fprintf(stderr, "No chord notes, cannot send\n");
	}
}

static void send_current_patch(){
	// Sends current chord over MIDI
	t_byte buf[PATCH_APR_SIZE];
	if(current_patch_valid){
		set_apr_from_patch_parameters(patch_parameters, patch_name, buf);
		send_sysex(buf, PATCH_APR_SIZE);
		printf("Sent current patch\n");
	}else{
		fprintf(stderr, "No patch parameters, cannot send\n");
	}
}

static void send_current(){
	// Sends current edit buffer contents (patch, tone and chord) over MIDI
	send_current_patch();
	send_current_tone();
	send_current_chord();
}

int recall_chord(unsigned int chord_num){
	// Implicit destination is edit buffer
	// always chord_notes in this case
		
	if(!chord_memory_valid){
		fprintf(stderr, "ERROR: chord data not set\n");
		return(-1);
	}
	if (chord_num > CHORDS){
		fprintf(stderr, "ERROR: illegal chord number %u\n", chord_num);
		return(-1);
	}
	printf("Recalling from chord #%u\n", chord_num);

	memcpy(chord_notes, chord_memory[chord_num], CHORD_NOTES);

	redraw_chord_notes();

	current_chord_valid=true;
	current_chord_changed=false;
	current_changed=current_patch_changed || current_chord_changed || current_chord_changed;
	chord_from_apr=false;
	current_valid=current_patch_valid && current_chord_valid && current_chord_valid;
	if (auto_send) send_current_chord();

// Should we set chord number in patch parameters ??

	return(0);
}

int recall_tone(unsigned int tone_num){
	// Implicit destination is edit buffer
	Tone_bank *tone_bank;
	tone_bank=tone_num>63 ? &tone_bank_b : &tone_bank_a;
		
	if(!tone_bank->valid){
		fprintf(stderr, "ERROR: tone data not set\n");
		return(-1);
	}
	
	int tone=tone_num & 0x3F;
	printf("Recalling from tone #%u \"%s\"\n", tone_num, tone_bank->names[tone]);

	memcpy(tone_parameters, tone_bank->parameters[tone], TONE_PARMS);
	memcpy(tone_name, tone_bank->names[tone], TONE_NAME_SIZE+1);
	patch_parameters[0]=tone_num;
	
	redraw_tone_parameters();

	current_tone_valid=true;
	current_tone_changed=false;
	current_valid=current_patch_valid && current_tone_valid && current_chord_valid;
	current_changed=current_patch_changed || current_chord_changed || current_chord_changed;

	if (auto_send) send_current_tone();

	return(0);
}

int recall_patch(unsigned int patch_num){
	// Implicit destination is edit buffer
	Patch_bank *patch_bank;
	Tone_bank *tone_bank;

	if(patch_num>PATCHES){
		fprintf(stderr, "ERROR: illegal patch number %u\n", patch_num);
		return(-1);
	}

	patch_bank=patch_num>63 ? &patch_bank_b : &patch_bank_a;
	// Always the same bank for tone and patch ??
	// tone_bank=patch_num>63 ? &tone_bank_b : &tone_bank_a;
		
	if(!patch_bank->valid || !chord_memory_valid){
		fprintf(stderr, "ERROR: patch or chord data not set\n");
		// fprintf(stderr, "PA %u PB %u TA %u TB %u CM %u\n", patch_bank_a.valid, patch_bank_b.valid, tone_bank_a.valid, tone_bank_b.valid, chord_memory_valid);
		return(-1); // ?? proceed anyway, load what's available
	}
	
	int program, tone_num, tone, chord;
	program=patch_num & 0x3F;
	tone_num=patch_bank->parameters[program][0];
	tone_bank=tone_num>63 ? &tone_bank_b : &tone_bank_a;
	tone=tone_num & 0x3F;
	if(!tone_bank->valid){
		fprintf(stderr, "ERROR: tone data not set\n");
		return(-1); // ?? proceed anyway, load what's available
	}
	chord=patch_bank->parameters[program][11];
	printf("Recalling from patch #%u \"%s\"\n", patch_num, patch_bank->names[program]);
	printf("Recalling tone #%u \"%s\", chord #%u\n", tone_num, tone_bank->names[tone], chord);

	memcpy(patch_parameters, patch_bank->parameters[program], PATCH_PARMS);
	memcpy(patch_name, patch_bank->names[program], PATCH_NAME_SIZE+1);
	current_patch_valid=true;
	current_patch_changed=false;
	send_current_patch();
	current_changed=current_patch_changed || current_chord_changed || current_chord_changed;
	redraw_patch_parameters();
	
	recall_tone(tone_num);
	recall_chord(chord);
	
	current_valid=true;
	// if (auto_send) send_current();

	return(0);
}

int store_patch(unsigned int patch_num, bool forced){
	// Implicit source is edit buffer
	// Destination includes patch, tone and chord
	// Destination is in bank b for patch numbers above 63
	Patch_bank *patch_bank;
	Tone_bank *tone_bank;
	patch_bank=patch_num>63 ? &patch_bank_b : &patch_bank_a;
	// Not always the same bank for tone and patch
	// tone_bank=patch_num>63 ? &tone_bank_b : &tone_bank_a;

    // Don't allow setting a patch when the bank has not been init'ed
    // This would create a partly valid bank
	if(!forced &&(!patch_bank->valid || !chord_memory_valid)){
		fprintf(stderr,"ERROR: patch or chord data not set\n");
		return(-1); // ?? store whatever is available anyway
	}
	if(patch_num>=PATCHES){
		fprintf(stderr,"ERROR: illegal patch number %u\n", patch_num);
		return(-1);
	}

	int program, tone, chord;
	program=patch_num & 0x3F;
	tone=forced?patch_num:patch_parameters[0];
	patch_parameters[0]=tone;
	tone_bank=tone>63 ? &tone_bank_b : &tone_bank_a;
	chord=patch_parameters[11];
	
	if (!forced && !tone_bank->valid){
		fprintf(stderr,"ERROR: tone data not set\n");
		return(-1);
	}

	printf("Storing to patch #%u, tone #%u, chord #%u\n", patch_num, tone, chord);

	memcpy(patch_bank->parameters[program], patch_parameters, PATCH_PARMS);
	memcpy(patch_bank->names[program], patch_name, PATCH_NAME_SIZE+1);
	patch_bank->changed=true;
	if (tone_bank->valid){
		memcpy(tone_bank->parameters[tone], tone_parameters, TONE_PARMS);
		memcpy(tone_bank->names[tone], tone_name, TONE_NAME_SIZE+1);
		tone_bank->changed=true;
	}
	if (chord_memory_valid){
		memcpy(chord_memory[chord], chord_notes, CHORD_NOTES);
		chord_memory_changed=true;
	}
	
	return(0);
}

int store_tone(unsigned int tone_num, bool forced){
	// Implicit source is edit buffer
	Tone_bank *tone_bank;
	tone_bank=tone_num>63 ? &tone_bank_b : &tone_bank_a;

    // Don't allow setting a tone when the bank has not been init'ed
    // This would create a partly valid bank
	if(!forced &&(!tone_bank->valid || !current_tone_valid)){
		fprintf(stderr,"ERROR: tone data not set\n");
		return(-1);
	}
	if(tone_num<0 or tone_num>=TONES){
		fprintf(stderr,"ERROR: illegal tone number %u\n", tone_num);
		return(-1);
	}
	
	int tone;
	tone=tone_num & 0x3F;

	printf("Storing to tone #%u\n", tone_num);

	// Update current patch and gui to reflect new tone number
	patch_parameters[0]=tone_num;
	current_patch_changed=true;
	current_changed=true;
	if(txt_tone_num){
		char s[4];
		sprintf(s, "%u", tone_num);
		txt_tone_num->value(s);
		txt_tone_num->redraw();
	}
	tone_bank->changed=true;
	
	memcpy(tone_bank->parameters[tone], tone_parameters, TONE_PARMS);
	memcpy(tone_bank->names[tone], tone_name, TONE_NAME_SIZE+1);
	
	return(0);
}

///////////////////////////
// MIDI receive callback //
///////////////////////////
static void *alsa_midi_process(void *handle) {
	// This function is way too long, should be split ??
	struct sched_param param;
	int policy;
	snd_seq_t *seq_handle = (snd_seq_t *)handle;

//	pthread_getschedparam(pthread_self(), &policy, &param);
//	policy = SCHED_FIFO;
//  param.sched_priority = 95;
//	pthread_setschedparam(pthread_self(), policy, &param);

  snd_seq_event_t *ev; // event data is up to 12 bytes
  t_byte *ptr;
  int len, errs, channel, got, needed, program, op, level;

  do {
	while (snd_seq_event_input(seq_handle, &ev)) {
		switch (ev->type) {
		case SND_SEQ_EVENT_SYSEX:
			// What about ignoring foreign sysexes, i.e wait for f7 ??
		    printf("alsa_midi_process: sysex event, received %u bytes.\n", ev->data.ext.len);
		    ptr=(t_byte *)ev->data.ext.ptr;
		    len=ev->data.ext.len;
#ifdef _DEBUG
			hex_dump(ptr, len);
#endif			
		    errs=0;
		    if(bulk_pending){
				level=bulk_buffer[5];
				if(level=TONE_LEVEL){
					needed=TONE_BLD_SIZE;
				}else if (level=PATCH_LEVEL){
					needed=PATCH_BLD_SIZE;
				}else{
					fprintf(stderr, "ERROR: Illegal bulk level %02x\n", level); errs++;
				}
				got=bulk_ptr-bulk_buffer;
				if(errs==0 && len==needed-got && ptr[len-1]==0Xf7){
					printf("Received tail of bulk dump, length %u, already had %u bytes\n", len, got);
					memcpy(bulk_ptr, ptr, len);
					program=bulk_buffer[8];
					printf("Programs from #%u, level %u\n", program, level);
					switch(bulk_buffer[5]){
						case TONE_LEVEL:
							printf("Assuming tone bank %c\n", is_tone_bank_b?'B':'A');
							set_tones_from_chunk(bulk_buffer, is_tone_bank_b? &tone_bank_b:&tone_bank_a);
							if(bulk_buffer[8]==60){ // hack - if last chunk, tone number is 60
								printf("Tone bank %c complete\n", is_tone_bank_b?'B':'A');
								if (is_tone_bank_b){
									set_tone_bank_valid(true, tone_bank_b, tone_table_b);
									tone_bank_b.changed=true;
								}else{
									set_tone_bank_valid(true, tone_bank_a, tone_table_a);
									tone_bank_a.changed=true;
								}
								is_tone_bank_b=!is_tone_bank_b;
							}
							break;
						case PATCH_LEVEL:
							printf("Assuming patch bank %c\n", is_patch_bank_b?'B':'A');
							set_patches_from_chunk(bulk_buffer, is_patch_bank_b? &patch_bank_b:&patch_bank_a);
							if(bulk_buffer[8]==60){ // hack - if last chunk, patch number is 60
								printf("Patch bank %c complete\n", is_patch_bank_b?'B':'A');
								if (is_patch_bank_b){
									set_patch_bank_valid(true, patch_bank_b, patch_table_b);
									patch_bank_b.changed=true;
								}else{
									set_patch_bank_valid(true, patch_bank_a, patch_table_a);
									patch_bank_a.changed=true;
								}
								is_patch_bank_b=!is_patch_bank_b;
							}
							break;
						default:
							fprintf(stderr, "ERROR: Illegal bulk dump level %02x\n", bulk_buffer[5]); errs++;
					}
				}else{
					// Assume the length of the dump can't exceed 2 buffers
					fprintf(stderr, "ERROR: Illegal bulk dump, had %u bytes, needed %u, received %u\n", got, needed, len); errs++;
				}
				bulk_pending=false;
			}else{ // No pending data, this should be a new sysex message
				if(parse_MKS50_sysex_header(ptr, channel, op, level)) errs++;
				if(errs==0){
					if(op==OP_APR) { // "APR" All parameters
						/* Example of APR data sent by the MKS-50		
						alsa_midi_process: sysex event, received 31 bytes. (all patch parameters
						f0 41 35 00 23 30 01 01 0c 6d 14 00 20 00 7f 00 00 0c 00 00 09 1a 33 33 06 2e 22 2d 1a 2b f7 
						alsa_midi_process: sysex event, received 14 bytes. (chord memory)
						f0 41 35 00 23 40 01 3c 7f 7f 7f 7f 7f f7 
						alsa_midi_process: sysex event, received 54 bytes.  (all tone parameters)
						f0 41 35 00 23 20 01 00 02 02 03 00 00 02 00 00 00 00 00 00 28 27 00 2b 00 00 6b 50 00 68 00 56 2a 00 7f 22 68 4a 00 2c 28 47 02 09 1a 33 33 06 2e 22 2d 1a 2b f7
						*/
						if (ptr[len-1]==0Xf7) { // APR sysex is shorter than buffer !!
							switch(level){
								case TONE_LEVEL:
									if(len==TONE_APR_SIZE){
										set_tone_parameters_from_apr(ptr+7, tone_parameters, tone_name);
										current_tone_valid=true;
										current_tone_changed=true;
										current_changed=true;
									}else{
										fprintf(stderr, "ERROR: Not a tone parameter apr sysex\n");
										errs++;
									}
									break;
								case PATCH_LEVEL:
									if(len==PATCH_APR_SIZE){
										set_patch_parameters_from_apr(ptr+7, patch_parameters, patch_name);
										current_patch_valid=true;
										current_patch_changed=true;
										current_changed=true;
									}else{
										fprintf(stderr, "ERROR: Not a patch parameter apr sysex\n");
										errs++;
									}
									break;
								case CHORD_LEVEL:
									if(len==CHORD_APR_SIZE){
										set_chord_notes_from_apr(ptr+7, chord_notes);
										current_chord_valid=true;
										current_chord_changed=true;
										current_changed=true;
										chord_from_apr=true;
										redraw_chord_notes();
									}else{
										fprintf(stderr, "ERROR: Not a chord apr sysex\n");
										errs++;
									}
									break;
								default:
									fprintf(stderr, "ERROR: unknown sysex level %02x\n", ptr[5]);
									errs++;
							} // end of switch level
							current_valid=current_patch_valid && current_tone_valid && current_chord_valid;
						}else{
							fprintf(stderr, "ERROR: no end of sysex %02x\n", ptr[len-1]);
							errs++;
						}
					}else if(op==OP_BLD){ // "BLD" bulk dump
						if(level == TONE_LEVEL || level == PATCH_LEVEL){
							needed=(level== TONE_LEVEL)?TONE_BLD_SIZE:PATCH_BLD_SIZE;
							// These dumps are longer and require extra buffering
							bulk_ptr=bulk_buffer;
							if(ptr[len-1]!=0Xf7){ // More data needed
								memcpy(bulk_ptr, ptr, len);
								printf("Received start of bulk dump, length %u, level %u\n", len, level);
								bulk_ptr+= len;
								bulk_pending=true;
							}else{
								if(len==needed){
									printf("Received complete bulk dump in one event\n");
									// TODO handle bulk data ??
									// This is needed only if alsa buffer is more than 266 bytes
								}else{
									fprintf(stderr, "ERROR: unexpected bulk dump length operation %u\n", len); errs++;
								}
							}
						}else if(level==CHORD_LEVEL){
							printf("Received chord memory dump\n");
							set_chord_memory_from_bld(ptr, chord_memory);
							set_chord_memory_valid(true);
							chord_table_1->redraw();
							chord_table_2->redraw();
						}else{
							fprintf(stderr, "ERROR: unknown level %02x\n", level); errs++;
						}
					}else{
						fprintf(stderr, "ERROR: unknown operation %02x\n", op); errs++;
					}
				} // end if errs==0
			} // end of if bulk pending
			// bulk dump: buffer seems to be 256 bytes, 266 needed
		    break;
		case SND_SEQ_EVENT_PGMCHANGE:
			// Unfortunately the MKS-50 doesn't send program change messages
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

//////////////////////////////
// User interface callbacks //
//////////////////////////////

const char patch_send_prompt[]=
	"Please start the patch bulk load on the MKS-50 before clicking on Close\n"
	"Make sure you chose the \"*\" version (no handshake)";
const char patch_load_prompt[]=
	"Patch bank not set\n"
	"Please load a patch bank sysex file or send a patch bulk dump from the MKS-50";
const char tone_send_prompt[]=
	"Please start the tone bulk load on the MKS-50 before clicking on Close\n"
	"Make sure you chose the \"*\" version (no handshake)";
const char tone_load_prompt[]=
	"Tone bank not set\n"
	"Please load a tone bank sysex file or send a tone bulk dump from the MKS-50";
const char chord_load_prompt[]=
	"Chords have not been set!\n"
	"Please load a chords sysex file or send a chords bulk dump from the MKS-50";
const char tone_recall_msg[]=
	"Click on any tone to recall it to the edit buffer and send it to the MKS-50";
const char patch_recall_msg[]=
	"Click on any patch to recall it to the edit buffer and send it to the MKS-50";
const char chord_recall_msg[]=
	"Click on any chord to recall it to the edit buffer and send it to the MKS-50";

void set_chord_memory_valid(bool b){
	chord_memory_valid=b;
	chord_table_1->tooltip(b?chord_recall_msg:chord_load_prompt);
	chord_table_2->tooltip(b?chord_recall_msg:chord_load_prompt);
}

void set_tone_bank_valid(bool b, Tone_bank &tone_bank, Fl_Widget* tone_table){
	tone_bank.valid=b;
	tone_table->tooltip(b?tone_recall_msg:tone_load_prompt);
}

void set_patch_bank_valid(bool b, Patch_bank &patch_bank, Fl_Widget* patch_table){
	patch_bank.valid=b;
	patch_table->tooltip(b?patch_recall_msg:patch_load_prompt);
}

static void btn_testnote_callback(Fl_Widget* o, void*) {
#ifdef _DEBUG	
	printf("State %u\n", btn_testnote_state);
#endif
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, portid); 
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	ev.queue = SND_SEQ_QUEUE_DIRECT; // maybe redundant

	if (((Fl_Valuator*)o)->value())
	    ev.type=SND_SEQ_EVENT_NOTEON;
	else
	    ev.type=SND_SEQ_EVENT_NOTEOFF;
	ev.data.note.channel=midi_channel;
	ev.data.note.note=midi_test_note;
	ev.data.note.velocity=midi_test_velocity;
	
	snd_seq_event_output_direct(seq_handle, &ev);
}

static void btn_all_notes_off_callback(Fl_Widget* o, void*) {
	btn_testnote->clear(); // Reset test note state
	all_notes_off(midi_channel);
}


void write_sysex_file(const t_byte *buf, const int len, const char *filename, bool auto_ext ){
	char * filename2, *filename3;
	FILE *sysex_file;
	filename2=(char *)malloc(strlen(filename)+5);
	if (filename2){
		strcpy(filename2, filename);
		filename3=trimwhitespace(filename2);
		if (auto_ext && strchr(filename3,'.') == 0)
			strcat(filename3, ".syx");
		// Check existence and prompt before overwriting
		if( access( filename3, F_OK ) != -1 ) {
			if(fl_choice("File already exists. Overwrite?","Yes", "No", 0)){
				free(filename2);
				fprintf(stderr, "Write sysex file cancelled.\n");
				return;
			}
			printf("Overwriting %s\n", filename3);
		}
		sysex_file=fopen(filename3,"wb");
		if (sysex_file==0){
			fprintf(stderr,"ERROR: Can not open sysex file %s\n", filename3);
		}else{		
			printf("Writing %u bytes to %s\n", len, filename3);
			fwrite(buf, 1, len, sysex_file);
			fclose(sysex_file);
		}
		free(filename2);
	}else{
		fprintf(stderr, "ERROR: Cannot allocate memory");
	}
}

void fc_save_current_callback(Fl_File_Chooser *w, void *userdata){
	// ?? should remember directory across calls
	if(w->visible()) return; // Do nothing until user has pressed ok
#ifdef _DEBUG	
	printf("File: %s\n", w->value());
	printf("Directory: %s\n", w->directory());
	printf("Filter: %u\n", w->filter_value());
#endif	
	t_byte buf[TONE_APR_SIZE+PATCH_APR_SIZE+CHORD_APR_SIZE];
	t_byte *dest=buf;
	if(current_tone_valid){
		set_apr_from_tone_parameters(tone_parameters, tone_name, dest);
		dest+=TONE_APR_SIZE;
	}
	if(current_patch_valid){
		set_apr_from_patch_parameters(patch_parameters, patch_name, dest);
		dest+=PATCH_APR_SIZE;
	}
	if(current_chord_valid){
		set_apr_from_chord_notes(chord_notes, dest);
		dest+=CHORD_APR_SIZE;
	}
	if(dest>buf)
		write_sysex_file(buf, dest-buf, w->value(), w->filter_value() == 0);
	current_changed=false;
	current_chord_changed=false;
	current_patch_changed=false;
	current_tone_changed=false;
}

static void btn_save_current_callback(Fl_Widget* o, void*) {
	if(!current_patch_valid && !current_tone_valid && !current_chord_valid){
		show_err("No data available, cannot save!");
		return;
	}
	if(!current_patch_valid || !current_tone_valid || !current_chord_valid){
		if (fl_choice("Not all data is available, do you want a partial save?\n"
		  "Patch data included:%s\n"
		  "Tone data included:%s\n"
		  "Chord data included:%s\n",
		  "Yes", "No", 0,
		  current_patch_valid?"Yes":"No",  
		  current_tone_valid?"Yes":"No",  
		  current_chord_valid?"Yes":"No")) return;
	}
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","sysex files(*.syx)",Fl_File_Chooser::CREATE,"Output file");
    fc->value(tone_name);
    fc->preview(0);
    fc->callback(fc_save_current_callback);
    fc->show();
}
static void set_parm(t_byte parm_num, t_byte parm_val){
	// Set in memory and sends to MKS-50
	// Parameter number coded on 7 bits
	// Parameter type on bit 7, 0 for tone, 1 for patch
	t_byte sysex[]="\xF0\x41\x36\x00\x23\x20\x01\x00\x00\xF7";
	bool is_patch_parm=parm_num>0x7F;
	t_byte short_parm_num=parm_num & 0x7F;
	if(is_patch_parm){
		if (short_parm_num>PATCH_PARMS){
			fprintf(stderr, "ERROR: Illegal patch parameter number %u\n", short_parm_num);
			return;
		}
		patch_parameters[short_parm_num]=parm_val;
		printf("set patch parameter #%u to %u\n", short_parm_num, parm_val);
		current_patch_changed=true;
	}else{ // Tone parm
		if (short_parm_num>TONE_PARMS){
			fprintf(stderr, "ERROR: Illegal tone parameter number %u\n", short_parm_num);
			return;
		}
		tone_parameters[short_parm_num]=parm_val;
		printf("set tone parameter #%u to %u\n", short_parm_num, parm_val);
		current_tone_changed=true;
	}
	current_changed=true;

	sysex[3]=midi_channel;
	sysex[5]=is_patch_parm ? 0x30: 0x20;
	sysex[7]=short_parm_num;
	sysex[8]=parm_val;
	send_sysex(sysex, 10);
	// check_error(status, "snd_seq_event_output_direct");
}

static void parm_callback(Fl_Widget* o, void*) {
	if (o){
#ifdef _DEBUG
		printf("%s parameter #%u = %u\n", (unsigned int)o->argument() & 0x80 ? "Patch": "Tone", (unsigned int)o->argument() & 0x3F, (unsigned int)((Fl_Valuator*)o)->value());
#endif
		set_parm((t_byte)o->argument(), (t_byte)((Fl_Valuator*)o)->value());
	}
}

static void parm_keymode_callback(Fl_Widget* o, void*) {
	if (o){
#ifdef _DEBUG
		printf("%s keymode parameter #%u = %u\n", (unsigned int)o->argument() & 0x80 ? "Patch": "Tone", (unsigned int)o->argument() & 0x3F, (unsigned int)((Fl_Valuator*)o)->value());
#endif
		t_byte parm_val=(t_byte)((Fl_Valuator*)o)->value();
		if (parm_val>2){
			fprintf(stderr, "ERROR: illegal key mode %u\n", parm_val);
			return;
		}
		set_parm((t_byte)o->argument(), key_mode_map[parm_val]); // Send the correct sysex
		patch_parameters[12]=parm_val; // Fix the current value
		redraw_keymode_parameter();
#ifdef _DEBUG	
		printf("Key mode %u -> sysex 0x%02x\n", parm_val, key_mode_map[parm_val]);
#endif
	}
}

static void parm_chord_callback(Fl_Widget* o, void*) {
	if (o){
		t_byte chord = (t_byte)((Fl_Valuator*)o)->value();
		if(chord>=16){
			fprintf(stderr, "ERROR: illegal chord number %u\n", chord);
			return;
		}		
		set_parm((t_byte)o->argument(), chord);
		recall_chord(chord);
	}
}

static void parm_wave_callback(Fl_Widget* o, void*) {
	if (o){
		t_byte v=(t_byte)((Fl_Valuator*)o)->value();
		set_parm((t_byte)o->argument(), v);
		redraw_wave_parameter();
	}
}

static void parm_chorus_callback(Fl_Widget* o, void*) {
	if (o){
		t_byte v=(t_byte)((Fl_Valuator*)o)->value();
		set_parm((t_byte)o->argument(), v);
		redraw_chorus_parameter();
	}
}

static void parm_porta_switch_callback(Fl_Widget* o, void*) {
	if (o){
		t_byte v=(t_byte)((Fl_Valuator*)o)->value();
		set_parm((t_byte)o->argument(), v);
		redraw_porta_switch_parameter();
	}
}

static void parm_note_callback(Fl_Widget* o, void*) {
	const char *ptr=((Fl_Input *)o)->value();
	const char *ptr2;
	int n;
	n=strtonote(ptr, &ptr2);
	if(n<0 or n>127)
		show_err("Note must be between 0 and 127"); // show as notes
	// test actual range based on argument - This is a hack
	else if(o->argument()==128+1 && (n<12 || n>108)) // C0..C8
		show_err("Range low note must be between C0 and C8");
	else if(o->argument()==128+2 && (n<13 || n>109)) // C#0..C#8
		show_err("Range low note must be between C#0 and C#8");
	else
		set_parm((t_byte)o->argument(), (t_byte)n);
	if(patch_parameters[1]>patch_parameters[2]) // Hack, belongs elsewhere ??
		show_err("Range low note should not be above range high note");
}

static void parm_key_shift_callback(Fl_Widget* o, void*) {
	const char *ptr=((Fl_Input *)o)->value();
	char *ptr2;
	int n;
	n=strtoul(ptr, &ptr2, 10);
	if(n<-12 or n>12)
		show_err("Must be between -12 and 12");
	else
		set_parm((t_byte)o->argument(), (t_byte)n & 0x7F);
}

static void txt_chord_callback(Fl_Widget* o, void*) {
	// implicit destination is current chord (chord_notes)
	const char *ptr=((Fl_Input *)o)->value();
	const char *ptr2;
	const char **ptrptr2=&ptr2;
	char s[5];
	int note, errs=0;
	for (int i=0; i<CHORD_NOTES; i++){
		note=strtonote(ptr, ptrptr2);
		if (note>=0){ // Valid note
			if(note>=CHORD_MIN_NOTE && note<=CHORD_MAX_NOTE){ // MKS50 range for chords
				ptr=*ptrptr2;
				chord_notes[i]=note; // apr format
			}else{
				chord_notes[i]=127; // Special value for off
				sprint_note(s, note);
				show_err("Note out of chord range C2..C6 (36..84) %s (%u)", s, note);
				errs++;
			}
		}else{
			chord_notes[i]=127; // Special value for off
			// Discard leading blanks
			while(*ptr==' ') ptr++;
			// Check for off
			if(toupper(ptr[0])=='O' && toupper(ptr[1])=='F' && toupper(ptr[2])=='F'){
				ptr+=3;
			}else{
				if(*ptr){ // Did not reach end
					show_err("ERROR: Cannot convert \"%s\" to chord note", ptr);
					errs++;
					break;
				}
			}
		}
#ifdef _DEBUG		
		printf("Note # %u = %d -> %u\n", i, note, chord_notes[i]);
#endif		
	}
	// hex_dump(chord_notes, CHORD_NOTES);
	current_chord_valid = (errs==0);
	// Should undo changes to chord_notes if not valid ??
	current_chord_changed = true;
	if(current_chord_valid) send_current_chord();
}


static void txt_tone_name_callback(Fl_Widget* o, void*) {
	if(!current_tone_valid){
		fl_alert("Current tone parameters not available!\n"
			"Please load a tone parameters file or change tone on the MKS-50");
			return;
	}
	strncpy(tone_name, ((Fl_Input *)o)->value(), TONE_NAME_SIZE);
	current_tone_changed=true;
	current_changed=true;
	send_current_tone(); // No ipr available for name, send apr
}

static void txt_patch_name_callback(Fl_Widget* o, void*) {
	if(!current_patch_valid){
		fl_alert("Current patch parameters not available!\n"
			"Please load a patch parameters file or change patch on the MKS-50");
			return;
	}
	strncpy(patch_name, ((Fl_Input *)o)->value(), PATCH_NAME_SIZE);
	current_patch_changed=true;
	current_changed=true;
	// send_current_patch(); // No ipr available for name, send apr
	// However this sets the tone and chord numbers ??
	// hence need to resend tone and chord as well
	send_current();
}

static void txt_midi_channel_callback(Fl_Widget* o, void*) {
	const char *ptr=((Fl_Input *)o)->value();
	char *ptr2;
	long c;
	c=strtoul(ptr, &ptr2, 0);
	if(c<1 or c>16)
		show_err("Midi channel must be between 1 and 16");
	else
		midi_channel=c-1;
}

static void txt_test_vel_callback(Fl_Widget* o, void*) {
	const char *ptr=((Fl_Input *)o)->value();
	char *ptr2;
	long v;
	v=strtoul(ptr, &ptr2, 0);
	if(v<1 or v>127)
		show_err("Velocity must be between 1 and 127");
	else
		midi_test_velocity=v;
}

static void txt_test_note_callback(Fl_Widget* o, void*) {
	const char *ptr=((Fl_Input *)o)->value();
	const char *ptr2;
	int n;
	n=strtonote(ptr, &ptr2);
	if(n<0 or n>127)
		show_err("Note must be between 0 and 127"); // show as notes
	else
		midi_test_note=n;
}

static void btn_send_current_callback(Fl_Widget* o, void*) {
	if(!current_patch_valid && !current_tone_valid && !current_chord_valid){
		fl_alert("Edit buffer has no content, cannot send\n"
			"Please load a parameters sysex file or select a patch/tone on the MKS-50");
		return;
	}
	if(!current_patch_valid || !current_tone_valid || !current_chord_valid){
		fl_alert("WARNING: Edit buffer has partial content, only the following will be sent:\n %s %s %s",
			current_patch_valid?"Patch data\n":"",
			current_tone_valid?"Tone data\n":"",
			current_chord_valid?"Chord data\n":""
			);
	}
	send_current();
}

static void btn_store_patch_callback(Fl_Widget* o, void*) {
	if (btn_store_patch->value()){
		btn_rename_patch->clear();
		// We don't know yet what bank it will be, we can't specifically check bank validity here
		if (!patch_bank_a.valid && !patch_bank_b.valid){
			fl_alert(patch_load_prompt);
			btn_store_patch->clear();
			return;
		}
		if(!current_patch_valid){
			fl_alert("Patch parameters not set in current edit buffer, cannot store\n"
				"Please load a patch parameters sysex file or select a patch on the MKS-50");
			btn_store_patch->clear();
			return;
		}
	}
}

static void btn_rename_patch_callback(Fl_Widget* o, void*) {
	if (btn_rename_patch->value()){
		btn_store_patch->clear();
		// We don't know yet what bank it will be, we can't specifically check bank validity here
		if (!patch_bank_a.valid && !patch_bank_b.valid){
			fl_alert(patch_load_prompt);
			btn_rename_patch->clear();
			return;
		}
	}
}

static void btn_store_tone_callback(Fl_Widget* o, void*) {
	if (btn_store_tone->value()){
		btn_rename_tone->clear();
		if(!current_tone_valid){
			// We don't know yet what bank it will be, we can't specifically check bank validity here
			fl_alert("Tone parameters not set in current edit buffer, cannot store\n"
				"Please load a tone parameters sysex file or select a tone on the MKS-50");
			btn_store_tone->clear();
			return;
		}
		// We don't know yet what bank it will be, we can't specifically check bank validity here
		if (!tone_bank_a.valid && !tone_bank_b.valid){
			fl_alert(tone_load_prompt);
			btn_store_tone->clear();
			return;
		}
	}
}

static void btn_rename_tone_callback(Fl_Widget* o, void*) {
	if (btn_rename_tone->value()){
		btn_store_tone->clear();
		// We don't know yet what bank it will be, we can't specifically check bank validity here
		if (!tone_bank_a.valid && !tone_bank_b.valid){
			fl_alert(tone_load_prompt);
			btn_rename_tone->clear();
			return;
		}
	}
}

int load_sysex_file(const char *filename){
	t_byte buf[SYSEX_MAX_SIZE];
	int len, channel, op, level, program, errs=0;
	bool keep_running=true;

	FILE *sysex_file;
	sysex_file=fopen(filename,"rb");
	if (sysex_file==0){
		fprintf(stderr,"ERROR: Can not open sysex file %s\n", filename);
	}else{		
		printf("Opened %s\n", filename);
		while(keep_running && errs==0){
			len=fread(buf, 1, SYSEX_HEADER_SIZE, sysex_file); // Get header
			if(len==SYSEX_HEADER_SIZE){
				if(parse_MKS50_sysex_header(buf, channel, op, level)) errs++;
				if(errs==0){
					if(op==OP_APR){
						 // ?? what about apr without name
						switch (level){
							case TONE_LEVEL:
								len=fread(buf+SYSEX_HEADER_SIZE, 1, TONE_APR_SIZE-SYSEX_HEADER_SIZE, sysex_file);
								if(len == TONE_APR_SIZE-SYSEX_HEADER_SIZE && buf[TONE_APR_SIZE-1] == 0xF7){
									printf("Read tone parameters\n");
									set_tone_parameters_from_apr(buf+7, tone_parameters, tone_name);
									current_tone_valid=true;
									current_tone_changed=false;
								}else{
									fprintf(stderr,"ERROR: invalid tone parameters\n");
									errs++;
								}
								break;
							case PATCH_LEVEL:
								len=fread(buf+SYSEX_HEADER_SIZE, 1, PATCH_APR_SIZE-SYSEX_HEADER_SIZE, sysex_file);
								if(len == PATCH_APR_SIZE-SYSEX_HEADER_SIZE && buf[PATCH_APR_SIZE-1] == 0xF7){
									printf("Read patch parameters\n");
									set_patch_parameters_from_apr(buf+7, patch_parameters, patch_name);
									// printf("loaded current chord %u\n", patch_parameters[11]);
									current_patch_valid=true;
									current_patch_changed=false;
								}else{
									fprintf(stderr,"ERROR: invalid patch parameters\n");
									errs++;
								}
								break;
							case CHORD_LEVEL:
								len=fread(buf+SYSEX_HEADER_SIZE, 1, CHORD_APR_SIZE-SYSEX_HEADER_SIZE, sysex_file);
								if(len == CHORD_APR_SIZE-SYSEX_HEADER_SIZE && buf[CHORD_APR_SIZE-1] == 0xF7){
									printf("Read chord notes\n");
									set_chord_notes_from_apr(buf+7, chord_notes);
									current_chord_valid=true;
									current_chord_changed=false;
								}else{
									fprintf(stderr,"ERROR: invalid chord notes\n");
									errs++;
								}
								break;
							default:
								fprintf(stderr,"ERROR: unknown level value %u\n", level);
								errs++;
						} // End of level switch
						current_valid=current_patch_valid && current_tone_valid && current_chord_valid;
					}else if (op==OP_BLD){
						switch (level){
							case TONE_LEVEL:
								len=fread(buf+SYSEX_HEADER_SIZE, 1, TONE_BLD_SIZE-SYSEX_HEADER_SIZE, sysex_file);
								if(len == TONE_BLD_SIZE-SYSEX_HEADER_SIZE && buf[TONE_BLD_SIZE-1] == 0xF7){
									program=buf[8];
									printf("Tones bulk dump from #%u, assuming tone bank %c\n", program, is_tone_bank_b?'B':'A');
									set_tones_from_chunk(buf, is_tone_bank_b? &tone_bank_b:&tone_bank_a);
									if(program==60){ // hack - if last chunk, tone number is 60
										printf("Tone bank %c complete\n", is_tone_bank_b?'B':'A');
										if (is_tone_bank_b){
											set_tone_bank_valid(true, tone_bank_b, tone_table_b);
											tone_bank_b.changed=false;
										}else{
											set_tone_bank_valid(true, tone_bank_a, tone_table_a);
											tone_bank_a.changed=false;
										}
										is_tone_bank_b=!is_tone_bank_b;
									}
								}else{
									fprintf(stderr,"ERROR: invalid tone bulk dump\n");
									errs++;
								}
								break;
							case PATCH_LEVEL:
								len=fread(buf+SYSEX_HEADER_SIZE, 1, PATCH_BLD_SIZE-SYSEX_HEADER_SIZE, sysex_file);
								if(len == PATCH_BLD_SIZE-SYSEX_HEADER_SIZE && buf[PATCH_BLD_SIZE-1] == 0xF7){
									program=buf[8];
									printf("Patches bulk dump from #%u, assuming patch bank %c\n", program, is_patch_bank_b?'B':'A');
									set_patches_from_chunk(buf, is_patch_bank_b? &patch_bank_b:&patch_bank_a);
									if(program==60){ // hack - if last chunk, patch number is 60
										printf("Patch bank %c complete\n", is_patch_bank_b?'B':'A');
										if (is_patch_bank_b){
											set_patch_bank_valid(true, patch_bank_b, patch_table_b);
											patch_bank_b.changed=false;
										}else{
											set_patch_bank_valid(true, patch_bank_a, patch_table_a);
											patch_bank_a.changed=false;
										}
										is_patch_bank_b=!is_patch_bank_b;
									}
								}else{
									fprintf(stderr,"ERROR: invalid patch  bulk dump\n");
									errs++;
								}
								break;
							case CHORD_LEVEL:
								len=fread(buf+SYSEX_HEADER_SIZE, 1, CHORD_BLD_SIZE-SYSEX_HEADER_SIZE, sysex_file);
								if(len == CHORD_BLD_SIZE-SYSEX_HEADER_SIZE && buf[CHORD_BLD_SIZE-1] == 0xF7){
									printf("Read chords bulk dump, length %u\n", len+SYSEX_HEADER_SIZE);
									set_chord_memory_from_bld(buf, chord_memory);
									set_chord_memory_valid(true);
									chord_memory_changed=false;
								}else{
									fprintf(stderr,"ERROR: invalid chord  bulk dump\n");
									errs++;
								}
								break;
							default:
								fprintf(stderr,"ERROR: unknown level value %u\n", level);
								errs++;
						} // End of level switch
					}else{
						fprintf(stderr,"ERROR: unknown operation %u\n", op);
						errs++;
					}
				} // End of if errs
			}else{
				fprintf(stderr,"End of file\n");
				keep_running=false;
			}
		} // end of while
		fclose(sysex_file);
	}
	return(errs);
}

void fc_load_sysex_callback(Fl_File_Chooser *w, void *userdata){
	// This is called on any user action, not just ok depressed!
	// ?? should remember directory across calls
	printf("File: %s\n", w->value());
	printf("Directory: %s\n", w->directory());
	printf("Filter: %u\n", w->filter_value());
	if(w->visible()) return; // Do nothing until user has pressed ok
	int errs=load_sysex_file(w->value());
	
	// refresh tables display just in case
	// ?? use flags to check which really needs a redraw
	patches_group->redraw();
	tones_group->redraw();
	chords_group->redraw();
	controls_group->redraw();
	
	if(errs) fl_alert("Error(s) loading file %s", w->value());
	if (auto_send && errs==0 && current_valid) send_current();
}

static void btn_load_current_callback(Fl_Widget* o, void*) {
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","sysex files(*.syx)",Fl_File_Chooser::SINGLE,"Input file");
    fc->preview(0);
    fc->callback(fc_load_sysex_callback);
    fc->show();
}

void set_bld_from_patch_bank(t_byte *buf, const Patch_bank *patch_bank){
	if(!patch_bank->valid){
		fprintf(stderr, "ERROR: Patch bank not set\n");
		return;
	}
	int program=0;
	for(int i=0; i<16; i++){ // 16 times 4 patches
		set_chunk_from_patches(patch_bank, buf+PATCH_BLD_SIZE*i, program);
		program+=4;
	}
}

void fc_save_patch_bank_a_callback(Fl_File_Chooser *w, void *userdata){
	// This is called on any user action, not just ok depressed!
	if(w->visible()) return; // Do nothing until user has pressed ok
	if(!patch_bank_a.valid){
		fl_alert(patch_load_prompt);
		return;
	}
	t_byte buf[16*PATCH_BLD_SIZE];
	set_bld_from_patch_bank(buf, &patch_bank_a);
	write_sysex_file(buf, 16*PATCH_BLD_SIZE, w->value(), w->filter_value() == 0);
}

void fc_save_patch_bank_b_callback(Fl_File_Chooser *w, void *userdata){
	// This is called on any user action, not just ok depressed!
	if(w->visible()) return; // Do nothing until user has pressed ok
	if(!patch_bank_b.valid){
		fl_alert(patch_load_prompt);
		return;
	}
	t_byte buf[16*PATCH_BLD_SIZE];
	set_bld_from_patch_bank(buf, &patch_bank_b);
	write_sysex_file(buf, 16*PATCH_BLD_SIZE, w->value(), w->filter_value() == 0);
}

void btn_send_patch_bank_a_callback(Fl_File_Chooser *w, void *userdata){
	if(!patch_bank_a.valid){
		fl_alert(patch_load_prompt);
		return;
	}
	fl_alert(patch_send_prompt);
	t_byte buf[16*PATCH_BLD_SIZE];
	set_bld_from_patch_bank(buf, &patch_bank_a);
	send_sysex(buf, 16*PATCH_BLD_SIZE);
}

void btn_send_patch_bank_b_callback(Fl_File_Chooser *w, void *userdata){
	if(!patch_bank_b.valid){
		fl_alert(patch_load_prompt);
		return;
	}
	fl_alert(patch_send_prompt);
	t_byte buf[16*PATCH_BLD_SIZE];
	set_bld_from_patch_bank(buf, &patch_bank_b);
	send_sysex(buf, 16*PATCH_BLD_SIZE);
}

void set_bld_from_tone_bank(t_byte *buf, const Tone_bank *tone_bank){
	if(!tone_bank->valid){
		fprintf(stderr, "ERROR: Tone bank not set\n");
		return;
	}
	int program=0;
	for(int i=0; i<16; i++){ // 16 times 4 tones
		set_chunk_from_tones(tone_bank, buf+TONE_BLD_SIZE*i, program);
		program+=4;
	}
}

void fc_save_tone_bank_a_callback(Fl_File_Chooser *w, void *userdata){
	// This is called on any user action, not just ok depressed!
	if(w->visible()) return; // Do nothing until user has pressed ok
	if(!tone_bank_a.valid){
		fl_alert(tone_load_prompt);
		return;
	}
	t_byte buf[16*TONE_BLD_SIZE];
	set_bld_from_tone_bank(buf, &tone_bank_a);
	write_sysex_file(buf, 16*TONE_BLD_SIZE, w->value(), w->filter_value() == 0);
}

void fc_save_tone_bank_b_callback(Fl_File_Chooser *w, void *userdata){
	// This is called on any user action, not just ok depressed!
	if(w->visible()) return; // Do nothing until user has pressed ok
	if(!tone_bank_b.valid){
		fl_alert(tone_load_prompt);
		return;
	}
	t_byte buf[16*TONE_BLD_SIZE];
	set_bld_from_tone_bank(buf, &tone_bank_b);
	write_sysex_file(buf, 16*TONE_BLD_SIZE, w->value(), w->filter_value() == 0);
}

void btn_send_tone_bank_a_callback(Fl_File_Chooser *w, void *userdata){
	if(!tone_bank_a.valid){
		fl_alert(tone_load_prompt);
		return;
	}
	fl_alert(tone_send_prompt);
	t_byte buf[16*TONE_BLD_SIZE];
	set_bld_from_tone_bank(buf, &tone_bank_a);
	send_sysex(buf, 16*TONE_BLD_SIZE);
}

void btn_send_tone_bank_b_callback(Fl_File_Chooser *w, void *userdata){
	if(!tone_bank_b.valid){
		fl_alert(tone_load_prompt);
		return;
	}
	fl_alert(tone_send_prompt);
	t_byte buf[16*TONE_BLD_SIZE];
	set_bld_from_tone_bank(buf, &tone_bank_b);
	send_sysex(buf, 16*TONE_BLD_SIZE);
}

void set_bld_from_chord_memory(t_byte *buf){
	// buf must be at least CHORD_BLD_SIZE bytes
	t_byte temp_notes[CHORD_NOTES];
	if(!chord_memory_valid){
		fprintf(stderr, "ERROR: Chord memory not set\n");
		return;
	}
	make_MKS50_sysex_header(buf, midi_channel, OP_BLD, CHORD_LEVEL);
	buf[SYSEX_HEADER_SIZE]=0x01; // group
	buf[SYSEX_HEADER_SIZE+1]=0x00; // extension of program number
	buf[SYSEX_HEADER_SIZE+2]=0x00; // program number
	for(int i=0; i<CHORDS; i++){
		memcpy(temp_notes, chord_memory[i], CHORD_NOTES);
		for(int j=0; j<CHORD_NOTES; j++)
			// For some reason, chord note 7F (no note) appears as FF in chords bulk dump
			if (temp_notes[j]==0x7F){
				temp_notes[j]=0xFF;
			}else{
				// Translate apr format to bld format
				temp_notes[j]=temp_notes[j]-CHORD_BASE_NOTE+(temp_notes[j]<CHORD_BASE_NOTE?128:0);
			}
		bytes_to_nibbles(temp_notes, buf+SYSEX_HEADER_SIZE+3+2*CHORD_NOTES*i, CHORD_NOTES);
	}
	buf[CHORD_BLD_SIZE-1]=0xF7;
}

void fc_save_chords_callback(Fl_File_Chooser *w, void *userdata){
	// This is called on any user action, not just ok depressed!
	if(w->visible()) return; // Do nothing until user has pressed ok
	if(!chord_memory_valid){
		fl_alert(chord_load_prompt);
		return;
	}
 	t_byte buf[CHORD_BLD_SIZE];
	set_bld_from_chord_memory(buf);
	write_sysex_file(buf, CHORD_BLD_SIZE, w->value(), w->filter_value() == 0);
	chord_memory_changed=false;
}

void btn_send_chords_callback(Fl_Widget* o, void*) {
	if(!chord_memory_valid){
		fl_alert(chord_load_prompt);
		return;
	}
	fl_alert("Please start the chord bulk load on the MKS-50 before clicking on Close");
 	t_byte buf[CHORD_BLD_SIZE];
	set_bld_from_chord_memory(buf);
	send_sysex(buf, CHORD_BLD_SIZE);
}

static void btn_save_patch_bank_a_callback(Fl_Widget* o, void*) {
	if(!patch_bank_a.valid){
		fl_alert(patch_load_prompt);
		return;
	}
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","sysex files(*.syx)",Fl_File_Chooser::CREATE,"Output file");
    fc->value("MKS50_patches_A.syx");
    fc->preview(0);
    fc->callback(fc_save_patch_bank_a_callback);
    fc->show();
    patch_bank_a.changed=false;
}

static void btn_save_patch_bank_b_callback(Fl_Widget* o, void*) {
	if(!patch_bank_b.valid){
		fl_alert(patch_load_prompt);
		return;
	}
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","sysex files(*.syx)",Fl_File_Chooser::CREATE,"Output file");
    fc->value("MKS50_patches_B.syx");
    fc->preview(0);
    fc->callback(fc_save_patch_bank_b_callback);
    fc->show();
    patch_bank_b.changed=false;
}

static void btn_save_tone_bank_a_callback(Fl_Widget* o, void*) {
	if(!tone_bank_a.valid){
		fl_alert(tone_load_prompt);
		return;
	}
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","sysex files(*.syx)",Fl_File_Chooser::CREATE,"Output file");
    fc->value("MKS50_tones_A.syx");
    fc->preview(0);
    fc->callback(fc_save_tone_bank_a_callback);
    fc->show();
    tone_bank_a.changed=false;
}

static void btn_save_tone_bank_b_callback(Fl_Widget* o, void*) {
	if(!tone_bank_b.valid){
		fl_alert(tone_load_prompt);
		return;
	}
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","sysex files(*.syx)",Fl_File_Chooser::CREATE,"Output file");
    fc->value("MKS50_tones_B.syx");
    fc->preview(0);
    fc->callback(fc_save_tone_bank_b_callback);
    fc->show();
    tone_bank_b.changed=false;
}

static void btn_save_chords_callback(Fl_Widget* o, void*) {
	if(!chord_memory_valid){
		fl_alert(chord_load_prompt);
		return;
	}
    Fl_File_Chooser *fc = new Fl_File_Chooser(".","sysex files(*.syx)",Fl_File_Chooser::CREATE,"Output file");
    fc->value("MKS50_chords.syx");
    fc->preview(0);
    fc->callback(fc_save_chords_callback);
    fc->show();
}

static void window_callback (Fl_Widget *widget, void *) 
{
// Adapted from https://stackoverflow.com/questions/8131510/fltk-closing-window
    Fl_Window *window = (Fl_Window *)widget;
    bool changed=patch_bank_a.changed || patch_bank_b.changed
		|| tone_bank_a.changed || tone_bank_b.changed
		|| chord_memory_changed
		|| current_changed;
	
    if(!changed){ window->hide(); return; }
    
	// Tell the user exactly what needs to be saved
    char s[128];
	char *ptr=s;
	bool need_comma=false;
	ptr+=sprintf(ptr, "Changed: ");
	if (patch_bank_a.changed){
		ptr+=sprintf(ptr,"patch bank a");
		need_comma=true;
	}
	if (patch_bank_b.changed){
		if (need_comma) ptr+=sprintf(ptr,", ");
		ptr+=sprintf(ptr,"patch bank b");
		need_comma=true;
	}
	if (tone_bank_a.changed){
		if (need_comma) ptr+=sprintf(ptr,", ");
		ptr+=sprintf(ptr,"tone bank a");
		need_comma=true;
	}
	if (tone_bank_b.changed){
		if (need_comma) ptr+=sprintf(ptr,", ");
		ptr+=sprintf(ptr,"tone bank b");
		need_comma=true;
	}
	if (chord_memory_changed){
		if (need_comma) ptr+=sprintf(ptr,", ");
		ptr+=sprintf(ptr,"chords");
		need_comma=true;
	}
	if (current_changed){
		if (need_comma) ptr+=sprintf(ptr,", ");
		ptr+=sprintf(ptr,"current edit buffer");
		need_comma=true;
	}


    int result = fl_choice("Quit without saving? All changes will be lost!\n%s", 
        "Quit anyway",  // 0
        "Cancel",       // 1
        0, // No 3rd button
        s);
    if (result == 0) // Close without saving
        window->hide(); // Already hidden ??
    else
		window->show();
    // else don't do anything
}

// GUI creation helper functions
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
	return(o);
}

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
	return(o);
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
	return(o);
}

// GUI creation
Fl_Double_Window* make_window() {

	// Prepare images and image lists
	// See also MKS50_images.h
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

	// ascii_to_rgb(off_image_data_ascii, off_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	// see https://www.fltk.org/doc-1.3/classFl__Image__Surface.html
	Fl_Image_Surface *img_surf = new Fl_Image_Surface(CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH);
	img_surf->set_current(); // direct graphics requests to the image
	fl_color(FL_BACKGROUND_COLOR); fl_rectf(0, 0, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH); // draw background
	fl_color(FL_BLACK); fl_draw("Off", -12, 5, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, FL_ALIGN_CENTER); // Should compute text size ??
	Fl_RGB_Image* off_image_rgb_ptr = img_surf->image();
	// delete img_surf; // delete the img_surf object -> black image!
	Fl_Display_Device::display_device()->set_current();  // direct graphics requests back to the display

	ascii_to_rgb(narrow_square_image_data_ascii, narrow_square_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(narrow_pulse_image_data_ascii, narrow_pulse_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(double_pulse_image_data_ascii, double_pulse_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	ascii_to_rgb(quad_pulse_image_data_ascii, quad_pulse_image_data_rgb, CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH);
	pulse_images_rgb[0]=off_image_rgb_ptr;// pulse_images_rgb[0]=&off_image_rgb;
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
	saw_images_rgb[0]=off_image_rgb_ptr;
	saw_images_rgb[1]=&saw1_image_rgb;
	saw_images_rgb[2]=&saw2_image_rgb;
	saw_images_rgb[3]=&saw3_image_rgb;
	saw_images_rgb[4]=&saw4_image_rgb;
	saw_images_rgb[5]=&saw5_image_rgb;

	// Create controls
	int w=50, h=25, spacing=5;
	int x=spacing, y=spacing;
	int x0, y0;
    int ww=3*spacing+17*(w+spacing);
    int hh=800;
    int tw=ww-2*spacing;
    int th=ww-2*spacing-h;
    Fl_Widget* o;

	main_window = new Fl_Double_Window(ww, hh, "Yet another MKS-50 patch editor");
	Fl_Tabs* tabs = new Fl_Tabs(0, 0, tw, th, "Parameters");
	controls_group = new Fl_Group(spacing, spacing+h, tw, th, "Parameters");
	x=11*w+12*spacing;
	x0=x; y0=y;
	y+=h+spacing;
	x += w+spacing; // There will be labels below
	btn_save_current = new Fl_Button(x, y, 2*w+spacing, h, "Save");
	btn_save_current->callback((Fl_Callback*)btn_save_current_callback);
	 btn_save_current->tooltip("Save all parameters from current edit buffer to file");
	x += 2*w + 2*spacing;
	btn_send_current = new Fl_Button(x, y, 2*w+spacing, h, "Send");
	btn_send_current->tooltip("Send all parameters from current edit buffer over midi");
	btn_send_current->callback((Fl_Callback*)btn_send_current_callback);
	x += 2*w + 2*spacing;
	x=x0; y+=h+spacing;
	y=y0; x=x0;
	// All sliders are for tone parameters unless otherwise noted
	x=spacing; y += 3*h + spacing; // New row
	make_value_slider(x, y, w, 8*h, "LFO\nrate", 24, 127); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "LFO\ndelay", 25, 127); x += w + spacing;
	x+=w+spacing;
	make_list_slider(x, y, w, 4*h, "DCO\nrange", 6, 3, dco_range_names); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "DCO\nlfo", 11, 127); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "DCO\nenv", 12, 127); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "DCO\nafter", 13, 127); x += w + spacing;
	make_image_list_slider(x, y, w, 4*h, "DCO\nenv\nmode", 0, 3, env_images_rgb); x += w + spacing;
	x+=w+spacing;
	make_list_slider(x, y, w, 4*h, "Key\nmode", 128 + 12, 2, key_mode_names); x += w + spacing; // Patch parameter
	patch_controls[12]->callback((Fl_Callback*)parm_keymode_callback); // Requires special mapping
	make_value_slider(x, y, w, 4*h, "Chord\nmemory", 128 + 11, 15); x += w + spacing; // Patch parameter
	make_value_slider(x, y, w, 8*h, "Mod.\nsens.", 128 + 5, 127); x += w + spacing; // Patch parameter
	patch_controls[11]->callback((Fl_Callback*)parm_chord_callback); // Requires special actions
	x += w+spacing; // room for label
	txt_patch_name = new Fl_Input(x, y, 2*w + spacing, h, "Patch:");
	txt_patch_name->callback((Fl_Callback*)txt_patch_name_callback);
	txt_patch_name->tooltip("Up to 10 letters or digits or space or dash");
	y0=y; x0=x;
	y+=h+spacing;
	txt_tone_name = new Fl_Input(x, y, 2*w + spacing, h, "Tone:");
	txt_tone_name->callback((Fl_Callback*)txt_tone_name_callback);
	txt_tone_name->tooltip("Up to 10 letters or digits or space or dash");
	x += 2*w + spacing;
	txt_tone_num = new Fl_Output(x, y, w, h); // ?? make it an input, same as tone recall??
	y+=h+spacing; x=x0;
	txt_chord=new Fl_Input(x, y, 4*w+3*spacing, h, "Chord:");
	txt_chord->callback((Fl_Callback*)txt_chord_callback);
	txt_chord->tooltip("Up to 6 notes from C2 to C6");
	y+=h+spacing; x=x0;
	txt_key_low=new Fl_Input(x, y, w, h, "Range:");
	txt_key_low->callback((Fl_Callback*)parm_note_callback);
	txt_key_low->argument(128+1);
	txt_key_low->tooltip("Lowest note the MKS-50 will play");
	x+=w+spacing;
	txt_key_high=new Fl_Input(x, y, w, h);
	txt_key_high->callback((Fl_Callback*)parm_note_callback);
	txt_key_high->argument(128+2);
	txt_key_high->tooltip("Highest note the MKS-50 will play");
	x+=w+spacing;
	x+=w+spacing; // Room for label
	txt_key_shift=new Fl_Int_Input(x, y, w, h, "Shift:");
	txt_key_shift->callback((Fl_Callback*)parm_key_shift_callback);
	txt_key_shift->argument(128+6);
	txt_key_shift->tooltip("Transpose by -12..+12 semitones");
	y=y0;
		
	x=spacing; y += 9*h + 2*spacing; // New row
	o=make_image_list_slider(x, y, w, 4*h, "DCO\npulse", 3, 3, pulse_images_rgb); x += w + spacing;
	o->callback((Fl_Callback*)parm_wave_callback);
	o=make_image_list_slider(x, y, w, 5*h, "DCO\nsaw", 4, 5, saw_images_rgb); x += w + spacing;
	o->callback((Fl_Callback*)parm_wave_callback);
	make_image_list_slider(x, y, w, 5*h, "DCO\nsub", 5, 5, sub_images_rgb); x += w + spacing;
	make_value_slider(x, y, w, 5*h, "Sub\nlevel", 7, 3); x += w + spacing;
	make_value_slider(x, y, w, 5*h, "Noise\nlevel", 8, 3); x += w + spacing;
	vs_pulse_width = make_value_slider(x, y, w, 8*h, "Pulse\nwidth", 14, 127); x += w + spacing;
	vs_pulse_width->tooltip("Applies only to some waveforms");
	vs_pulse_mod_rate=make_value_slider(x, y, w, 8*h, "P.mod\nrate", 15, 127); x += w + spacing;
	vs_pulse_mod_rate->tooltip("Pulse width modulation rate\n0 for fixed pulse width");
	make_value_slider(x, y, w, 5*h, "HPF\nfreq", 9, 3); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "VCF\nfreq", 16, 127); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "VCF\nreso", 17, 127); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "VCF\nlfo", 18, 127); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "VCF\nenv", 19, 127); x += w + spacing;
	make_image_list_slider(x, y, w, 4*h, "VCF\nenv\nmode", 1, 3, env_images_rgb); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "VCF\nkey\nfollow", 20, 127); x += w + spacing;
	make_value_slider(x, y, w, 8*h, "VCF\nafter", 21, 127); x += w + spacing;
	o=make_list_slider(x, y, w, 3*h, "Chorus", 10, 1, on_off_names); x += w + spacing;
	o->callback((Fl_Callback*)parm_chorus_callback);
	vs_chorus_rate=make_value_slider(x, y, w, 8*h, "Chorus\nrate", 34, 127); x += w + spacing;

	x=spacing; y += 9*h + 2*spacing; // New row
	make_value_slider(x, y, w, 8*h, "VCA\nlevel", 22, 127); x += w + spacing;
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
	o=make_list_slider(x, y, w, 3*h, "Porta.", 128 + 4, 1, on_off_names); x += w + spacing; // Patch parameter
	o->callback((Fl_Callback*)parm_porta_switch_callback);
	vs_porta_time=make_value_slider(x, y, w, 8*h, "Porta.\ntime", 128 + 3, 127); x += w + spacing; // Patch parameter
	x+=w+spacing;
	controls_group->end();

	patches_group = new Fl_Group(spacing, spacing+2*h, tw, th, "Patches");
	x=spacing; y=h+spacing; // Room for tabs
	y+=h+spacing; // Room for label
	patch_table_a=new PatchTable(x, y, 2*w*8+2, h*8+2, "Patch bank A");
	patch_table_a->col_width_all(2*w);
	patch_table_a->row_height_all(h);
	patch_table_a->patches(&patch_bank_a);
	patch_table_a->tooltip(patch_load_prompt);
	// patch_table_a->align(FL_ALIGN_LEFT | FL_ALIGN_TOP);
	// Buttons do not work for some y positions, interfering with label??
	y+=h*8+2;
	y+=h+spacing; // Room for label
	patch_table_b=new PatchTable(x, y, 2*w*8+2, h*8+2, "Patch bank B");
	patch_table_b->col_width_all(2*w);
	patch_table_b->row_height_all(h);
	patch_table_b->patches(&patch_bank_b);
	patch_table_b->tooltip(patch_load_prompt);
	y+=h*8+2+spacing;
	btn_store_patch = new Fl_Toggle_Button(x, y, 2*w+spacing-5, h, "Store");
	btn_store_patch->callback((Fl_Callback*)btn_store_patch_callback);
	btn_store_patch->tooltip("Will store patch parameters from edit buffer to next activated patch");
	x += 2*w + spacing - 5;
	btn_rename_patch = new Fl_Toggle_Button(x, y, 2*w+spacing-5, h, "Rename");
	btn_rename_patch->callback((Fl_Callback*)btn_rename_patch_callback);
	btn_rename_patch->tooltip("Will rename next activated patch");
	x += 2*w + spacing - 5;
	btn_save_patch_bank_a = new Fl_Button(x, y, 2*w+spacing-5, h, "Save A");
	btn_save_patch_bank_a->callback((Fl_Callback*)btn_save_patch_bank_a_callback);
	btn_save_patch_bank_a->tooltip("Save parameters for all patches from patch bank A to a file");
	x += 2*w + spacing - 5;
	btn_save_patch_bank_b = new Fl_Button(x, y, 2*w+spacing-5, h, "Save B");
	btn_save_patch_bank_b->callback((Fl_Callback*)btn_save_patch_bank_b_callback);
	btn_save_patch_bank_b->tooltip("Save parameters for all patches from patch bank B to a file");
	x += 2*w + spacing - 5;
	btn_send_patch_bank_a = new Fl_Button(x, y, 2*w+spacing-5, h, "Send A");
	btn_send_patch_bank_a->callback((Fl_Callback*)btn_send_patch_bank_a_callback);
	btn_send_patch_bank_a->tooltip("Send parameters for all patches from patch bank A over midi");
	x += 2*w + spacing - 5;
	btn_send_patch_bank_b = new Fl_Button(x, y, 2*w+spacing-5, h, "Send B");
	btn_send_patch_bank_b->callback((Fl_Callback*)btn_send_patch_bank_b_callback);
	btn_send_patch_bank_b->tooltip("Send parameters for all patches from patch bank B over midi");
	patches_group->end();

	tones_group = new Fl_Group(spacing, spacing+h, tw, th, "Tones");
	x=spacing; y=h+spacing; // Room for tabs
	y+=h+spacing; // Room for label
	tone_table_a=new ToneTable(x, y, 2*w*8+2, h*8+2, "Tone bank A");
	tone_table_a->col_width_all(2*w);
	tone_table_a->row_height_all(h);
	tone_table_a->tones(&tone_bank_a);
	tone_table_a->tooltip(tone_load_prompt);
	y+=h*8+2;
	y+=h+spacing; // Room for label
	tone_table_b=new ToneTable(x, y, 2*w*8+2, h*8+2, "Tone bank B");
	tone_table_b->col_width_all(2*w);
	tone_table_b->row_height_all(h);
	tone_table_b->tones(&tone_bank_b);
	tone_table_b->tooltip(tone_load_prompt);
	y+=h*8+2+spacing;
	btn_store_tone = new Fl_Toggle_Button(x, y, 2*w+spacing-5, h, "Store");
	btn_store_tone->callback((Fl_Callback*)btn_store_tone_callback);
	btn_store_tone->tooltip("Will store tone parameters from edit buffer to next activated tone");
	x += 2*w + spacing - 5;
	btn_rename_tone = new Fl_Toggle_Button(x, y, 2*w+spacing-5, h, "Rename");
	btn_rename_tone->callback((Fl_Callback*)btn_rename_tone_callback);
	btn_rename_tone->tooltip("Will rename next activated tone");
	x += 2*w + spacing - 5;
	btn_save_tone_bank_a = new Fl_Button(x, y, 2*w+spacing-5, h, "Save A");
	btn_save_tone_bank_a->callback((Fl_Callback*)btn_save_tone_bank_a_callback);
	btn_save_tone_bank_a->tooltip("Save parameters for all tones from tone bank A to a file");
	x += 2*w + spacing - 5;
	btn_save_tone_bank_b = new Fl_Button(x, y, 2*w+spacing-5, h, "Save B");
	btn_save_tone_bank_b->callback((Fl_Callback*)btn_save_tone_bank_b_callback);
	btn_save_tone_bank_b->tooltip("Save parameters for all tones from tone bank B to a file");
	x += 2*w + spacing - 5;
	btn_send_tone_bank_a = new Fl_Button(x, y, 2*w+spacing-5, h, "Send A");
	btn_send_tone_bank_a->callback((Fl_Callback*)btn_send_tone_bank_a_callback);
	btn_send_tone_bank_a->tooltip("Send parameters for all tones from tone bank A over midi");
	x += 2*w + spacing - 5;
	btn_send_tone_bank_b = new Fl_Button(x, y, 2*w+spacing-5, h, "Send B");
	btn_send_tone_bank_b->callback((Fl_Callback*)btn_send_tone_bank_b_callback);
	btn_send_tone_bank_b->tooltip("Send parameters for all tones from tone bank B over midi");
	tones_group->end();

	chords_group = new Fl_Group(spacing, spacing+h, tw, th, "Chords");
	x=spacing; y=h+spacing; // Room for tabs
	y+=h+spacing; // Room for label
	chord_table_1=new ChordTable(x, y, w*5+2, h*8+2, "Chords");
	chord_table_1->row_header_width(w);
	chord_table_1->col_width(0, 4*w);
	chord_table_1->row_height_all(h);
	chord_table_1->offset(0);
	chord_table_1->tooltip(chord_load_prompt);
	x+=w*5+2+spacing;
	chord_table_2=new ChordTable(x, y, w*5+2, h*8+2);
	chord_table_2->row_header_width(w);
	chord_table_2->col_width(0, 4*w);
	chord_table_2->row_height_all(h);
	chord_table_2->offset(8);
	chord_table_2->tooltip(chord_load_prompt);
	y+=h*8+2+spacing; x=spacing; // New row
	btn_store_chord = new Fl_Toggle_Button(x, y, 2*w+spacing, h, "Store");
	btn_store_chord->tooltip("Will store chord from current edit buffer into next activated chord");
	// btn_store_chord->callback((Fl_Callback*)btn_store_chord_callback);
	x += 2*w + spacing;
	btn_save_chords = new Fl_Button(x, y, 2*w+spacing, h, "Save");
	btn_save_chords->callback((Fl_Callback*)btn_save_chords_callback);
	btn_save_chords->tooltip("Save all chords from chord memory to a file");
	x += 2*w + spacing;
	btn_send_chords = new Fl_Button(x, y, 2*w+spacing, h, "Send");
	btn_send_chords->callback((Fl_Callback*)btn_send_chords_callback);
	btn_send_chords->tooltip("Send all chords from chord memory over midi");
	x += 2*w + spacing;

	chords_group->end();

	tabs->end();
	
	// The following will appear on all tabs
	x=spacing; y=hh-h-spacing;
	btn_testnote = new Fl_Toggle_Button(x, y, 2*w+spacing, h, "Test note");
	btn_testnote->callback((Fl_Callback*)btn_testnote_callback);
	btn_testnote->tooltip("Send a test note over midi");
	x += 2*w + 2*spacing;
	txt_midi_channel=new Fl_Input(x, y, w, h);
	txt_midi_channel->callback((Fl_Callback*)txt_midi_channel_callback);
	txt_midi_channel->tooltip("Set the midi channel (1..16)");
	char str_midi_channel[5];
	sprintf(str_midi_channel, "%u", midi_channel+1);
	txt_midi_channel->value(str_midi_channel);
	x += w + spacing;
	txt_test_note=new Fl_Input(x, y, w, h);
	txt_test_note->callback((Fl_Callback*)txt_test_note_callback);
	txt_test_note->tooltip("Set the test note, for example C4");
	char str_test_note[5];
	sprint_note(str_test_note, midi_test_note);
	txt_test_note->value(str_test_note);
	x += w + spacing;
	txt_test_vel=new Fl_Input(x, y, w, h);
	txt_test_vel->callback((Fl_Callback*)txt_test_vel_callback);
	txt_test_vel->tooltip("Set the test note velocity, between 1 and 127");
	char str_test_vel[4];
	snprintf(str_test_vel, 4, "%u", midi_test_velocity);
	txt_test_vel->value(str_test_vel);
	x += w + spacing;
	btn_all_notes_off = new Fl_Button(x, y, 2*w+spacing, h, "Panic");
	btn_all_notes_off->callback((Fl_Callback*)btn_all_notes_off_callback);
	btn_all_notes_off->tooltip("Send an \"all notes off\" message");
	x += 2*w + 2*spacing;
	btn_load_current = new Fl_Button(x, y, 2*w+spacing, h, "Load");
	btn_load_current->callback((Fl_Callback*)btn_load_current_callback);
	btn_load_current->tooltip("Load any MKS-50 sysex file");
	x += 2*w + 2*spacing;

	main_window->end();
	main_window->resizable(main_window);
	return main_window;
}

int main(int argc, char **argv) {

	Fl_Window* win = make_window();
	win->callback(window_callback);
	patch_bank_b.program_offset=64;
	tone_bank_b.program_offset=64;
	char *sender_id=0, *dest_id=0;
	char *sender_name=0, *dest_name=0;
	bool dest_ok=false;
	bool replicate=false;
	// chord_table_2->offset(8);

	// Handle command line arguments
	int arg_errs=0;
	long c;
	if (argc > 1){
		for(int i=1; i<argc;i++){
			if(argv[i][0]!='-' || strlen(argv[i])!=2){
				// Got a file
				arg_errs+=load_sysex_file(argv[i]);
			}else{
				// Got an option
				if (i+1>=argc && argv[i][1]!='r'){
					fprintf(stderr, "Option %s requires an argument\n", argv[i]);
					exit(-1);
				}
				switch(argv[i][1]){
					case 'c':
						char *endptr;
						c=strtoul(argv[i+1], &endptr, 0);
						if(c<1 || c>16){
							fprintf(stderr, "Illegal channel number %s\n", argv[i+1]);
							exit(-1);
						}
						midi_channel=c-1;
						i++;
						break;
					case 'i':
						sender_id=argv[i+1];
						i++;
						break;
					case 'I':
						sender_name=argv[i+1];
						i++;
						break;
					case 'o':
						dest_id=argv[i+1];
						i++;
						break;
					case 'O':
						dest_name=argv[i+1];
						i++;
						break;
					case 'r':
						replicate=true;
						break;
					default:
						fprintf(stderr, "Illegal option %s\n", argv[i]);
						exit(-1);
				}
			}	
		}
	}
	
	// printf("current chord %u\n", patch_parameters[11]);
	if(replicate){
		if(!current_valid){
			show_err("Nothing to replicate - please specify patch file");
		}else{
			for(int i=0; i<CHORDS; i++) store_chord(i, true);
			set_chord_memory_valid(true);
			for(int i=0; i<PATCHES; i++) store_patch(i, true);
			set_patch_bank_valid(true, patch_bank_a, patch_table_a);
			set_patch_bank_valid(true, patch_bank_b, patch_table_b);
			for(int i=0; i<TONES; i++) store_tone(i, true);
			set_tone_bank_valid(true, tone_bank_a, tone_table_a);
			set_tone_bank_valid(true, tone_bank_b, tone_table_b);
		}
	}
	
	// midi init
	pthread_t midithread;
	seq_handle = open_seq();
	npfd = snd_seq_poll_descriptors_count(seq_handle, POLLIN);
	pfd = (struct pollfd *)alloca(npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seq_handle, pfd, npfd, POLLIN);
	
	// midi connect
	char dest_port_name[10];
	char sender_port_name[10];
	if (dest_name){
		if(get_port_by_name(seq_handle, dest_name, dest_port_name, 10)){
			dest_id=0;
			show_err("ALSA midi port %s does not exist.", dest_name);
		}else{
			dest_id=dest_port_name;
		}
	}
	if (sender_name){
		if(get_port_by_name(seq_handle, sender_name, sender_port_name, 10)){
			sender_id=0;
			show_err("ALSA midi port %s does not exist.", sender_name);
		}else{
			sender_id=sender_port_name;
		}
	}
	
	int client = snd_seq_client_id(seq_handle);
	char s_client[6];
	snprintf(s_client, 6, "%u:0", client);
	if(dest_id){
		if(aconnect(s_client, dest_id))
			show_err("Connection to midi destination %s failed\n"
			    "Use aconnect -o to show available destinations", dest_id);
		else
			dest_ok=true;
	}
	if(sender_id){
		if(aconnect(sender_id, s_client))
			show_err("Connection from midi source %s failed\n"
			    "Use aconnect -i to show available sources", sender_id);
	}

	// create the thread and tell it to use Midi::work as thread function
	int err = pthread_create(&midithread, NULL, alsa_midi_process, seq_handle);
	if (err) {
		fprintf(stderr, "Error %u creating MIDI thread\n", err);
		// should exit? This is non-blocking for one-way use from editor to MKS-50.
	}

	// Now that midi is ok, try to sync the MKS-50
	// For this to work, we need to connect to midi output using command line option -o
	// and possibly set channel
	if (dest_ok && auto_send && arg_errs==0 && current_valid) send_current();

	// FLTK main loop
	win->show();
	Fl::run();
	// wait for the midi thread to shutdown carefully
	pthread_cancel(midithread);
	// release Alsa Midi connection
	snd_seq_close(seq_handle);
	
	printf("Bye!\n");
}
