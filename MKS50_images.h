// Bitmaps for envelope, box is 50 wide and 25 high
// image is 48*23, should be 46*21 or 44*20 for border?
#ifndef MKS50_images_h
#define MKS50_images_h

#define CTRL_IMAGE_WIDTH 46
#define CTRL_IMAGE_HEIGTH 21
/* Template
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "                                              " // 03
    "                                              " // 04
    "                                              " // 05
    "                                              " // 06
    "                                              " // 07
    "                                              " // 08
    "                                              " // 09
    "                                              " // 10
    "                                              " // 11
    "                                              " // 12
    "                                              " // 13
    "                                              " // 14
    "                                              " // 15
    "                                              " // 16
    "                                              " // 17
    "                                              " // 18
    "                                              " // 19
    "                                              " // 20
    "                                              " // 21
    "                                              " // 22
;
*/

unsigned char env_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
    "              .                               " // 01
    "              .                               " // 02
    "            .   .                             " // 03
    "            .   .                             " // 04
    "           .     .                            " // 05
    "           .     .                            " // 06
    "          .       .                           " // 07
    "          .       .                           " // 08
    "         .         .                          " // 09
    "         .         .                          " // 10
    "        .           .....................     " // 11
    "        .                               .     " // 12
    "       .                                 .    " // 13
    "       .                                 .    " // 14
    "      .                                   .   " // 15
    "      .                                   .   " // 16
    "     .                                     .  " // 17
    "     .                                     .  " // 18
    "    .                                       . " // 19
    "    .                                       . " // 20
    "   .                                         ." // 21
    "   .                                         ." // 22
;
unsigned char env_dyn_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
    "              .             .....             " // 01
    "              .             .    .            " // 02
    "            .   .           .     .           " // 03
    "            .   .           .     .           " // 04
    "           .     .          .     .           " // 05
    "           .     .          .     .           " // 06
    "          .       .         .    .            " // 07
    "          .       .         .....             " // 08
    "         .         .                          " // 09
    "         .         .                          " // 10
    "        .           .....................     " // 11
    "        .                               .     " // 12
    "       .                                 .    " // 13
    "       .                                 .    " // 14
    "      .                                   .   " // 15
    "      .                                   .   " // 16
    "     .                                     .  " // 17
    "     .                                     .  " // 18
    "    .                                       . " // 19
    "    .                                       . " // 20
    "   .                                         ." // 21
    "   .                                         ." // 22
;
unsigned char env_gate_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "      .....................................   " // 03
    "      .                                   .   " // 04
    "      .                                   .   " // 05
    "      .                                   .   " // 06
    "      .                                   .   " // 07
    "      .                                   .   " // 08
    "      .                                   .   " // 09
    "      .                                   .   " // 10
    "      .                                   .   " // 11
    "      .                                   .   " // 12
    "      .                                   .   " // 13
    "      .                                   .   " // 14
    "      .                                   .   " // 15
    "      .                                   .   " // 16
    "      .                                   .   " // 17
    "      .                                   .   " // 18
    "      .                                   .   " // 19
    "      .                                   .   " // 20
    "      .                                   .   " // 21
    " ......                                   ......" // 22
;

unsigned char env_dyn_gate_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "      .....................................   " // 03
    "      .                                   .   " // 04
    "      .                                   .   " // 05
    "      .                                   .   " // 06
    "      .                                   .   " // 07
    "      .              .....                .   " // 08
    "      .              .    .               .   " // 09
    "      .              .     .              .   " // 10
    "      .              .     .              .   " // 11
    "      .              .     .              .   " // 12
    "      .              .     .              .   " // 13
    "      .              .    .               .   " // 14
    "      .              .....                .   " // 15
    "      .                                   .   " // 16
    "      .                                   .   " // 17
    "      .                                   .   " // 18
    "      .                                   .   " // 19
    "      .                                   .   " // 20
    "      .                                   .   " // 21
    " ......                                   ...." // 22
;

unsigned char env_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char env_rev_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char env_dyn_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char env_dyn_rev_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char env_gate_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char env_dyn_gate_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
static Fl_RGB_Image env_image_rgb(env_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image env_rev_image_rgb(env_rev_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image env_dyn_image_rgb(env_dyn_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image env_dyn_rev_image_rgb(env_dyn_rev_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image env_gate_image_rgb(env_gate_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image env_dyn_gate_image_rgb(env_dyn_gate_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
const Fl_RGB_Image *env_images_rgb[4];
const Fl_RGB_Image *env_vca_images_rgb[4];

unsigned char square_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    " .                     ...................... " // 03
    " .                     .                    . " // 04
    " .                     .                    . " // 05
    " .                     .                    . " // 06
    " .                     .                    . " // 07
    " .                     .                    . " // 08
    " .                     .                    . " // 09
    " .                     .                    . " // 10
    " .                     .                    . " // 11
    " .                     .                    . " // 12
    " .                     .                    . " // 13
    " .                     .                    . " // 14
    " .                     .                    . " // 15
    " .                     .                    . " // 16
    " .                     .                    . " // 17
    " .                     .                    . " // 18
    " .                     .                    . " // 19
    " .                     .                    . " // 20
    " .                     .                    . " // 21
    " .......................                    .." // 22
;
unsigned char pulse_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    " .                                ........... " // 03
    " .                                .         . " // 04
    " .                                .         . " // 05
    " .                                .         . " // 06
    " .                                .         . " // 07
    " .                                .         . " // 08
    " .                                .         . " // 09
    " .                                .         . " // 10
    " .                                .         . " // 11
    " .                                .         . " // 12
    " .                                .         . " // 13
    " .                                .         . " // 14
    " .                                .         . " // 15
    " .                                .         . " // 16
    " .                                .         . " // 17
    " .                                .         . " // 18
    " .                                .         . " // 19
    " .                                .         . " // 20
    " .                                .         . " // 21
    " ..................................         . " // 22
;
unsigned char pulse_mod_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    " .           ..  ..  ........................ " // 03
    " .           .         .                    . " // 04
    " .           .         .                    . " // 05
    " .                     .                    . " // 06
    " .           .         .                    . " // 07
    " .           .         .                    . " // 08
    " .                     .                    . " // 09
    " .           .         .                    . " // 10
    " .           .         .                    . " // 11
    " .                     .                    . " // 12
    " .           .         .                    . " // 13
    " .           .         .                    . " // 14
    " .                     .                    . " // 15
    " .           .         .                    . " // 16
    " .           .         .                    . " // 17
    " .                     .                    . " // 18
    " .                     .                    . " // 19
    " .           .         .                    . " // 20
    " .           .         .                    . " // 21
    " .......................                    .." // 22
;

unsigned char off_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "                                              " // 03
    "                                              " // 04
    "                                              " // 05
    "                                              " // 06
    "          .....     ........   ........       " // 07
    "         .     .    .          .              " // 08
    "        .       .   .          .              " // 09
    "        .       .   .....      .....          " // 10
    "        .       .   .          .              " // 11
    "        .       .   .          .              " // 12
    "         .     .    .          .              " // 13
    "          .....     .          .              " // 14
    "                                              " // 15
    "                                              " // 16
    "                                              " // 17
    "                                              " // 18
    "                                              " // 19
    "                                              " // 20
    "                                              " // 21
    "                                              " // 22
;
unsigned char square_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char pulse_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char pulse_mod_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char off_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
static Fl_RGB_Image square_image_rgb(square_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image pulse_image_rgb(pulse_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image pulse_mod_image_rgb(pulse_mod_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image off_image_rgb(off_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
const Fl_RGB_Image *pulse_images_rgb[4];

unsigned char narrow_square_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "        .              ................       " // 03
    "        .              .              .       " // 04
    "        .              .              .       " // 05
    "        .              .              .       " // 06
    "        .              .              .       " // 07
    "        .              .              .       " // 08
    "        .              .              .       " // 09
    "        .              .              .       " // 10
    "        .              .              .       " // 11
    "        .              .              .       " // 12
    "        .              .              .       " // 13
    "        .              .              .       " // 14
    "        .              .              .       " // 15
    "        .              .              .       " // 16
    "        .              .              .       " // 17
    "        .              .              .       " // 18
    "        .              .              .       " // 19
    "        .              .              .       " // 20
    "        .              .              .       " // 21
    "        ................              .       " // 22
;
unsigned char narrow_pulse_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "        .                    ..........       " // 03
    "        .                    .        .       " // 04
    "        .                    .        .       " // 05
    "        .                    .        .       " // 06
    "        .                    .        .       " // 07
    "        .                    .        .       " // 08
    "        .                    .        .       " // 09
    "        .                    .        .       " // 10
    "        .                    .        .       " // 11
    "        .                    .        .       " // 12
    "        .                    .        .       " // 13
    "        .                    .        .       " // 14
    "        .                    .        .       " // 15
    "        .                    .        .       " // 16
    "        .                    .        .       " // 17
    "        .                    .        .       " // 18
    "        .                    .        .       " // 19
    "        .                    .        .       " // 20
    "        .                    .        .       " // 21
    "        ......................        .       " // 22
;
unsigned char double_pulse_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "       .              ......  ......          " // 03
    "       .              .    .  .    .          " // 04
    "       .              .    .  .    .          " // 05
    "       .              .    .  .    .          " // 06
    "       .              .    .  .    .          " // 07
    "       .              .    .  .    .          " // 08
    "       .              .    .  .    .          " // 09
    "       .              .    .  .    .          " // 10
    "       .              .    .  .    .          " // 11
    "       .              .    .  .    .          " // 12
    "       .              .    .  .    .          " // 13
    "       .              .    .  .    .          " // 14
    "       .              .    .  .    .          " // 15
    "       .              .    .  .    .          " // 16
    "       .              .    .  .    .          " // 17
    "       .              .    .  .    .          " // 18
    "       .              .    .  .    .          " // 19
    "       .              .    .  .    .          " // 20
    "       .              .    .  .    .          " // 21
    "       ................    ....    .          " // 22
;
unsigned char quad_pulse_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "       .              ... ... ... .           " // 03
    "       .              . . . . . . .           " // 04
    "       .              . . . . . . .           " // 05
    "       .              . . . . . . .           " // 06
    "       .              . . . . . . .           " // 07
    "       .              . . . . . . .           " // 08
    "       .              . . . . . . .           " // 09
    "       .              . . . . . . .           " // 10
    "       .              . . . . . . .           " // 11
    "       .              . . . . . . .           " // 12
    "       .              . . . . . . .           " // 13
    "       .              . . . . . . .           " // 14
    "       .              . . . . . . .           " // 15
    "       .              . . . . . . .           " // 16
    "       .              . . . . . . .           " // 17
    "       .              . . . . . . .           " // 18
    "       .              . . . . . . .           " // 19
    "       .              . . . . . . .           " // 20
    "       .              . . . . . . .           " // 21
    "       ................ ... ... ...           " // 22
;
unsigned char narrow_square_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char narrow_pulse_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char double_pulse_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char quad_pulse_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
static Fl_RGB_Image narrow_square_image_rgb(narrow_square_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image narrow_pulse_image_rgb(narrow_pulse_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image double_pulse_image_rgb(double_pulse_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image quad_pulse_image_rgb(quad_pulse_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
const Fl_RGB_Image *sub_images_rgb[6];

unsigned char saw1_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "                                      ....    " // 03
    "                                    ..   .    " // 04
    "                                  ..     .    " // 05
    "                                ..       .    " // 06
    "                              ..         .    " // 07
    "                            ..           .    " // 08
    "                          ..             .    " // 09
    "                        ..               .    " // 10
    "                      ..                 .    " // 11
    "                    ..                   .    " // 12
    "                  ..                     .    " // 13
    "                ..                       .    " // 14
    "              ..                         .    " // 15
    "            ..                           .    " // 16
    "          ..                             .    " // 17
    "        ..                               .    " // 18
    "      ..                                 .    " // 19
    "    ..                                   .    " // 20
    "  ..                                     .    " // 21
    " ..                                      .    " // 22
;

unsigned char saw2_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "                                     ....     " // 03
    "                                   ..   .     " // 04
    "                                 ..     .     " // 05
    "                               ..       .     " // 06
    "                             ..         .     " // 07
    "                           ..           .     " // 08
    "                         ..             .     " // 09
    "                       ..               .     " // 10
    "                       .                .     " // 11
    "                       .                .     " // 12
    "                       .                .     " // 13
    "                       .                .     " // 14
    "                       .                .     " // 15
    "           ..          .                .     " // 16
    "         .. .          .                .     " // 17
    "       ..   .          .                .     " // 18
    "     ..     .          .                .     " // 19
    "   ..       .          .                .     " // 20
    " ..         .          .                .     " // 21
    "..          ............                .     " // 22
;
unsigned char saw3_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "                                     ....     " // 03
    "                                   ..   .     " // 04
    "                                 ..     .     " // 05
    "                               ..       .     " // 06
    "                             ..         .     " // 07
    "                           ..           .     " // 08
    "                         ..             .     " // 09
    "                       ..               .     " // 10
    "                       .                .     " // 11
    "                  .    .                .     " // 12
    "               .. .    .                .     " // 13
    "             ..        .                .     " // 14
    "                  .    .                .     " // 15
    "           ..     .    .                .     " // 16
    "         .. .          .                .     " // 17
    "       ..   .     .    .                .     " // 18
    "     ..     .     .    .                .     " // 19
    "   ..       .          .                .     " // 20
    " ..         .     .    .                .     " // 21
    "..          ............                .     " // 22
;
unsigned char saw4_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "                                      ...     " // 03
    "                                    ..  .     " // 04
    "                                    .   .     " // 05
    "                               ..   .   .     " // 06
    "                             .. .   .   .     " // 07
    "                            .   .   .   .     " // 08
    "                       ..   .   .   .   .     " // 09
    "                     .. .   .   .   .   .     " // 10
    "                    .   .   .   .   .   .     " // 11
    "                    .   .   .   .   .   .     " // 12
    "                    .   .   .   .   .   .     " // 13
    "               ..   .   .   .   .   .   .     " // 14
    "             .. .   .   .   .   .   .   .     " // 15
    "            .   .   .   .   .   .   .   .     " // 16
    "            .   .   .   .   .   .   .   .     " // 17
    "        .   .   .   .   .   .   .   .   .     " // 18
    "      ...   .   .   .   .   .   .   .   .     " // 19
    "    ..  .   .   .   .   .   .   .   .   .     " // 20
    "    .   .   .   .   .   .   .   .   .   .     " // 21
    ".....   .....   .....   .....   .....   ..... " // 22
;
unsigned char saw5_image_data_ascii[]=
  // 012345678901234567890123456789012345678901234567
//  "                                              " // 00
//  "                                              " // 01
    "                                              " // 02
    "                                      ...     " // 03
    "                                    ..  .     " // 04
    "                                    .   .     " // 05
    "                               ..   .   .     " // 06
    "                             .. .   .   .     " // 07
    "                            .   .   .   .     " // 08
    "                            .   .   .   .     " // 09
    "                            .   .   .   .     " // 10
    "                            .   .   .   .     " // 11
    "                            .   .   .   .     " // 12
    "                            .   .   .   .     " // 13
    "               ..           .   .   .   .     " // 14
    "             .. .           .   .   .   .     " // 15
    "            .   .           .   .   .   .     " // 16
    "            .   .           .   .   .   .     " // 17
    "        .   .   .           .   .   .   .     " // 18
    "      ...   .   .           .   .   .   .     " // 19
    "    ..  .   .   .           .   .   .   .     " // 20
    "    .   .   .   .           .   .   .   .     " // 21
    ".....   .....   .............   .....   ..... " // 22
;

unsigned char saw1_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char saw2_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char saw3_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char saw4_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
unsigned char saw5_image_data_rgb[CTRL_IMAGE_WIDTH*CTRL_IMAGE_HEIGTH*3];
static Fl_RGB_Image saw1_image_rgb(saw1_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image saw2_image_rgb(saw2_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image saw3_image_rgb(saw3_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image saw4_image_rgb(saw4_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
static Fl_RGB_Image saw5_image_rgb(saw5_image_data_rgb, CTRL_IMAGE_WIDTH, CTRL_IMAGE_HEIGTH, 3, 0);
const Fl_RGB_Image *saw_images_rgb[6];

#endif
