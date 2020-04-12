## Summary

## Installation

No installer provided at the moment

The only prerequisites are alsa and fltk

The following commands were tested under Linux Mint 19.1

- Installing ALSA development files:
```
sudo apt install libasound2-dev
```
- Installing FLTK and its development files:
```
sudo apt install libfltk1.3-dev
```
- Compiling:
```
gcc fluid_test2.cxx Fl_List_Slider.cxx Fl_Image_List_Slider.cxx -o fluid_test2 -lfltk -lstdc++ -lasound -lpthread
```
or
```
gcc MKS50_editor.cxx -D _DEBUG ...
```
- Installing:
```
sudo cp MKS50_editor /usr/local/bin/
```
## Usage
Currently there isn't any command line option

Notes:

The MKS-50 has an early MIDI sysex implementation, with some shortcomings.

There is no provision for requesting MKS-50 data over midi.

All midi transmission requires manual action on the MKS-50 front panel.

To get data into the editor, you need to either

- open a syx file (as created by the editor using save)

or

- select a patch (or tone or chord) on the MKS-50 front panel

  For this to work, you have set the three "Tune/Midi" parameters "TX **** APR" to on
  
  where **** is "TONE", "PATCH" or "C.M."
  
This program currently supports "APR" (All parameters) and "IPR" (Individual parameter).

It doesn't support bulk dumps (yet...).
