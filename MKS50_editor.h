 /*
 * This file is part of MKS50_editor, which is free software:
 * you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
 
#ifndef MKS50_editor_h
#define MKS50_editor_h
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Toggle_Button.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Table.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/Fl_Int_Input.H>


#include "Fl_List_Slider.H"
#include "Fl_Image_List_Slider.H"

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>

int store_chord(unsigned int chord_num, bool forced);
int store_patch(unsigned int patch_num, bool forced);
int store_tone(unsigned int tone_num, bool forced);
int recall_chord(unsigned int chord_num);
int recall_patch(unsigned int patch_num);
int recall_tone(unsigned int tone_num);

#endif
