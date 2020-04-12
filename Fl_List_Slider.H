//
// Modified vloop 2020
//
// "$Id$"
//
// Text list slider header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

/* \file
   Fl_List_Slider widget . */

#ifndef Fl_List_Slider_H
#define Fl_List_Slider_H

#include <FL/Fl_Slider.H>

/**
  The Fl_List_Slider widget is a Fl_Slider widget
  with a box displaying the current value.
  <P ALIGN=CENTER>\image html value_slider.png 
  \image latex  value_slider.png "Fl_List_Slider" width=4cm
*/
class FL_EXPORT Fl_List_Slider : public Fl_Slider {
    Fl_Font textfont_;
    Fl_Fontsize textsize_;
    Fl_Color textcolor_;
    const char **list_;
    unsigned int list_count_;
protected:
    void draw();
public:
    int handle(int);
    Fl_List_Slider(int x,int y,int w,int h, const char *l = 0);
    /**    Gets the typeface of the text in the value box.  */
    Fl_Font textfont() const {return textfont_;}
    /**    Sets the typeface of the text in the value box.  */
    void textfont(Fl_Font s) {textfont_ = s;}
    /**    Gets the size of the text in the value box.  */
    Fl_Fontsize textsize() const {return textsize_;}
    /**    Sets the size of the text in the value box.  */
    void textsize(Fl_Fontsize s) {textsize_ = s;}
    /**    Gets the color of the text in the value box.  */
    Fl_Color textcolor() const {return textcolor_;}
    /**    Sets the color of the text in the value box.  */
    void textcolor(Fl_Color s) {textcolor_ = s;}

    const char **list() const {return list_;}
    unsigned int list_count() const {return list_count_;}
    void list(const char **s, unsigned int count) {list_ = s; list_count_=count; range(count-1,0); }
};

#endif

//
// End of "$Id$".
//
