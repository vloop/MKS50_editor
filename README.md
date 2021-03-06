## Summary

"Possibly the ugliest MKS-50 patch editor in the world"

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
gcc MKS50_editor.cxx Fl_List_Slider.cxx Fl_Image_List_Slider.cxx -o MKS50_editor -lfltk -lstdc++ -lasound -lpthread
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
```
MKS50_editor.cxx [option arg...][file...]
```
will preload any specified MKS-50 sysex files (single sound or bulk dump).

Available options:
```
-c midi channel
-i alsa midi input id
-o alsa midi output id
-I alsa midi input name
-O alsa midi output name
```
If the -o option is used and a patch file is loaded from the command line,
the editor will automatically send that patch over midi.

Please use
```
aconnect -i -o
```
to show avalable inputs and outputs ids.

Depending on your configuration, these may change at reboot.

Examples:
```
MKS50_editor -o 20:2 -i 20:2 init.syx
MKS50_editor -O "mio10 MIDI 3" -I "mio10 MIDI 3" init.syx
MKS50_editor init.syx init_patches_A.syx init_patches_B.syx init_tones.syx init_tones.syx init_chords.syx

```
Don't forget the quotes around the port name if it has spaces!

Port names, unlike port ids, are not guaranteed to be unique; the first match will be used.

On startup, the editor has no data, and has no way of requesting data from the MKS-50 (that's a limitation of the MKS-50 sysex implementation).

It can read preset files, but factory presets sysex files are not included with this program (because of possible copyright issues).

Several init files are provided so you can start using the editor right away, however you'll probably want to create your own set of startup sysexe files.

The first step is to get data from the MKS-50, which involves manual operation on the MKS-50 front panel ("data transfer" button and patch/tone selection, always the version with "*", meaning no hadshake).

The editor is always ready to receive MKS-50 sysex data, no manual operation is needed on the editor side.

Remember that the editor expects first the a data, then the b data.

This only needs to be done once per bulk data set (patch bank a/b, tone bank a/b, chords). The data can then be saved to files from the editor.

After you have created your set of sysex files, they can be loaded on startup by specifying them on the command line (or manually from inside the editor), thus bypassing the tedious manual operations on the MKS-50.

However, if you do some manual editing from the MKS-50 front panel (which clearly indicates a masochistic tendency), it will obviously not be reflected automatically in the files.

To put things back in sync, you need to transmit bulk dumps either to or from the MKS-50.

Of course, all of this can only work if you set the correct alsa midi connections from and to the editor.

This can be done with aconnect (command line) or from qjackctl (gui) and others, or by command line options.

Sysex files:

There are two main types: parameter files and bulk dump files.

Parameter files match the MKS-50 edit buffer.

Parameter files are made up of 3 "apr" sysex messages: one for patch parameters, one for tone parameters, and one for the current chord.

The MKS-50 can receive them without manual intervention.

It sends them when a new patch or tone is selected from the front panel, provided the TX flags have been set to "ON" from the MKS-50 front panel.

Patches and tones usually have the same number and matching names, however this is not a requirement.

Patches have a tone number parameter, which can point to a different tone.

Bulk dump files match the MKS-50 memory, handling them always requires MKS-50 front panel operations.

Librarian functions:

The default action when a patch, chord or tone is clicked in their respective tabs is to recall it to the edit buffer, and transmit it to the MKS-50.

When a patch is recalled, the tone and chord it points to are also recalled.

To store the edit buffer contents to bank memory, first click "Store" on the relevant tab, then click on the target location.

Similarly, to rename a patch or tone, first click the rename button, then click on the patch or tone you want to rename.

Although the editor can handle any number of sysex messages in a sysex file, the MKS-50 cannot.

The editor only creates sysex files that can actually be loaded into the MKS-50, therefore separate files are needed for each tone or patch bank, and for the chord memory.

These 5 files can be created by clicking on the "Save A" and "Save B" buttons in the patch and tone tabs, and on the "Save" button in the chords tab.

You may also choose to use only the edit buffer features, and save each of your sounds as a separate sysex file, which will include patch and tone parameters and chord data.

The "Load" button can be used to load any type of MKS-50 sysex files, it recognizes both "apr" (all parameters, only for one sound) and "bld" (bulk dump, for an entire bank or chord memory), and it works the same from all the tabs.

The bulk dumps for banks a and b do not contain information about which bank it is (possibly to maintain compatibility with other Roland instruments).

For that reason, when loading (or receiving) bank data, the editor will assume bank a is loaded (or received) before bank b.

Important note:

The MKS-50 has an early MIDI sysex implementation, with some shortcomings.

There is no provision for requesting MKS-50 data over midi.

All midi transmission from the MKS-50 requires manual action on the MKS-50 front panel.

To get data into the editor, you need to either

- open a syx file (as created by the editor using save)

or

- select a patch (or tone or chord) on the MKS-50 front panel

  For this to work, you have set the three "Tune/Midi" parameters "TX **** APR" to on
  
  where **** is "TONE", "PATCH" or "C.M."
  
The editor supports "apr" (all parameters) and "ipr" (individual parameter).

It also supports bulk dumps (without handshake, "Bulk*Dump" on the MKS-50).

Implementation notes:

The MKS-50 sends data when a program change is made from the front panel, but unfortunately not when it receives a program change over midi.

Similarly, it does not send chord data when the chord number is changed from the editor.

This means the editor has no way of knowing for certain the notes actually used by the current chord, unless a chord memory bulk dump is launched manually from the MKS-50.

This prevents fully automated implementation of chord notes handling in the editor; the editor will usually rely on the assumption that previously loaded (or received) chord memory dump is still valid.

The MKS-50 has two sets of programs (A and B), but the bulk dump data does not identify which it is.

The editor has no way of knowing whether a bulk dump is A or B, because the information is not present in the MKS-50 bulk dump data.

The editor assumes set A is sent before set B, which may or not be the case.
