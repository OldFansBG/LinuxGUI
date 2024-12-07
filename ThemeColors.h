#ifndef THEME_COLORS_H
#define THEME_COLORS_H

#include <wx/wx.h>

namespace ThemeColors {
   // Dark Theme Colors
   const wxColour DARK_TITLEBAR(26, 26, 26);        // #1A1A1A
   const wxColour DARK_BACKGROUND(40, 44, 52);      // #282C34
   const wxColour DARK_TEXT(229, 229, 229);         // #E5E5E5
   const wxColour DARK_BUTTON_BG(68, 71, 90);       // #44475A
   const wxColour DARK_BUTTON_TEXT(255, 255, 255);  // #FFFFFF
   const wxColour DARK_BUTTON_HOVER(98, 114, 164);  // #6272A4
   const wxColour DARK_PRIMARY_BG(255, 121, 198);   // #FF79C6
   const wxColour DARK_PRIMARY_TEXT(40, 44, 52);    // #282C34
   const wxColour DARK_BORDER(98, 114, 164);        // #6272A4
   const wxColour DARK_HIGHLIGHT(139, 233, 253);    // #8BE9FD

   // Light Theme Colors
   const wxColour LIGHT_TITLEBAR(0, 120, 212);      // #0078D4
   const wxColour LIGHT_BACKGROUND(255, 255, 255);  // #FFFFFF
   const wxColour LIGHT_TEXT(0, 0, 0);              // #000000
   const wxColour LIGHT_BUTTON_BG(225, 225, 225);   // #E1E1E1
   const wxColour LIGHT_BUTTON_TEXT(0, 0, 0);       // #000000
   const wxColour LIGHT_BUTTON_HOVER(208, 208, 208);// #D0D0D0
   const wxColour LIGHT_PRIMARY_BG(0, 176, 255);    // #00B0FF
   const wxColour LIGHT_PRIMARY_TEXT(255, 255, 255);// #FFFFFF
   const wxColour LIGHT_BORDER(195, 195, 195);      // #C3C3C3

   // Special Colors
   const wxColour CLOSE_BUTTON_HOVER(232, 17, 35);  // #E81123
}

#endif // THEME_COLORS_H