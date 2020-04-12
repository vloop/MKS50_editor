//
// Modified vloop 2020
//
// "$Id$"
//
// Text list slider widget for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include "Fl_List_Slider.H"
#include <FL/Fl_Hor_Value_Slider.H>
#include <FL/fl_draw.H>
#include <math.h>

/**
  Creates a new Fl_List_Slider widget using the given
  position, size, and label string. The default boxtype is FL_DOWN_BOX.
*/
Fl_List_Slider::Fl_List_Slider(int X, int Y, int W, int H, const char*l)
: Fl_Slider(X,Y,W,H,l) {
  step(1);
  textfont_ = FL_HELVETICA;
  textsize_ = 12;
  textcolor_ = FL_FOREGROUND_COLOR;
}

void Fl_List_Slider::draw() {
  int sxx = x(), syy = y(), sww = w(), shh = h();
  int bxx = x(), byy = y(), bww = w(), bhh = h();
  if (horizontal()) {
    bww = 35; sxx += 35; sww -= 35;
  } else {
    syy += 25; bhh = 25; shh -= 25;
  }
  if (damage()&FL_DAMAGE_ALL) draw_box(box(),sxx,syy,sww,shh,color());
  Fl_Slider::draw(sxx+Fl::box_dx(box()),
		  syy+Fl::box_dy(box()),
		  sww-Fl::box_dw(box()),
		  shh-Fl::box_dh(box()));
  draw_box(box(),bxx,byy,bww,bhh,color());
  // printf("w=%u h=%u\n", bww, bhh); // -> w=50 h=25
  char buf2[128];
  // format(buf);
  const char *buf;
  // printf("%s\n", list()[0]); // --> Poly
  // printf("%s\n", list()[1]); // --> Chord
  // printf("%s\n", *list()); // --> Poly
  // printf("%lu\n", sizeof(*list())); // --> 8 ??
  // printf("%u\n", list_count()); //
  // printf("%u\n", (int)value()); //
  // printf("%s\n", list()[(int)value()]);
  if ((int)value()<list_count() && (int)value()>=0){
	buf=list()[(int)value()];
  }else{
	format(buf2);
	buf=buf2;
  }
	  
  fl_font(textfont(), textsize());
  fl_color(active_r() ? textcolor() : fl_inactive(textcolor()));
  fl_draw(buf, bxx, byy, bww, bhh, FL_ALIGN_CLIP);
}

int Fl_List_Slider::handle(int event) {
  if (event == FL_PUSH && Fl::visible_focus()) {
    Fl::focus(this);
    redraw();
  }
  int sxx = x(), syy = y(), sww = w(), shh = h();
  if (horizontal()) {
    sxx += 35; sww -= 35;
  } else {
    syy += 25; shh -= 25;
  }
  return Fl_Slider::handle(event,
			   sxx+Fl::box_dx(box()),
			   syy+Fl::box_dy(box()),
			   sww-Fl::box_dw(box()),
			   shh-Fl::box_dh(box()));
}

/*
error: ‘Fl_Hor_List_Slider’ does not name a type

Fl_Hor_List_Slider::Fl_Hor_List_Slider(int X,int Y,int W,int H,const char *l)
: Fl_List_Slider(X,Y,W,H,l) {
  type(FL_HOR_SLIDER);
}
*/

//
// End of "$Id$".
//
