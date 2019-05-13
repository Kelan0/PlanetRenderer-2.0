#pragma once

#include "core/Core.h"

#define KEYBOARD_SIZE 512
#define MOUSE_SIZE 16

#define KEY_UNKNOWN 0
#define KEY_A 4
#define KEY_B 5
#define KEY_C 6
#define KEY_D 7
#define KEY_E 8
#define KEY_F 9
#define KEY_G 10
#define KEY_H 11
#define KEY_I 12
#define KEY_J 13
#define KEY_K 14
#define KEY_L 15
#define KEY_M 16
#define KEY_N 17
#define KEY_O 18
#define KEY_P 19
#define KEY_Q 20
#define KEY_R 21
#define KEY_S 22
#define KEY_T 23
#define KEY_U 24
#define KEY_V 25
#define KEY_W 26
#define KEY_X 27
#define KEY_Y 28
#define KEY_Z 29
#define KEY_1 30
#define KEY_2 31
#define KEY_3 32
#define KEY_4 33
#define KEY_5 34
#define KEY_6 35
#define KEY_7 36
#define KEY_8 37
#define KEY_9 38
#define KEY_0 39
#define KEY_RETURN 40
#define KEY_ESCAPE 41
#define KEY_BACKSPACE 42
#define KEY_TAB 43
#define KEY_SPACE 44
#define KEY_MINUS 45
#define KEY_EQUALS 46
#define KEY_LEFTBRACKET 47
#define KEY_RIGHTBRACKET 48
#define KEY_BACKSLASH 49
#define KEY_NONUSHASH 50
#define KEY_SEMICOLON 51
#define KEY_APOSTROPHE 52
#define KEY_GRAVE 53
#define KEY_COMMA 54
#define KEY_PERIOD 55
#define KEY_SLASH 56
#define KEY_CAPSLOCK 57
#define KEY_F1 58
#define KEY_F2 59
#define KEY_F3 60
#define KEY_F4 61
#define KEY_F5 62
#define KEY_F6 63
#define KEY_F7 64
#define KEY_F8 65
#define KEY_F9 66
#define KEY_F10 67
#define KEY_F11 68
#define KEY_F12 69
#define KEY_PRINTSCREEN 70
#define KEY_SCROLLLOCK 71
#define KEY_PAUSE 72
#define KEY_INSERT 73
#define KEY_HOME 74
#define KEY_PAGEUP 75
#define KEY_DELETE 76
#define KEY_END 77
#define KEY_PAGEDOWN 78
#define KEY_RIGHT 79
#define KEY_LEFT 80
#define KEY_DOWN 81
#define KEY_UP 82
#define KEY_NUMLOCKCLEAR 83
#define KEY_KP_DIVIDE 84
#define KEY_KP_MULTIPLY 85
#define KEY_KP_MINUS 86
#define KEY_KP_PLUS 87
#define KEY_KP_ENTER 88
#define KEY_KP_1 89
#define KEY_KP_2 90
#define KEY_KP_3 91
#define KEY_KP_4 92
#define KEY_KP_5 93
#define KEY_KP_6 94
#define KEY_KP_7 95
#define KEY_KP_8 96
#define KEY_KP_9 97
#define KEY_KP_0 98
#define KEY_KP_PERIOD 99
#define KEY_NONUSBACKSLASH 100
#define KEY_APPLICATION 101
#define KEY_POWER 102
#define KEY_KP_EQUALS 103
#define KEY_F13 104
#define KEY_F14 105
#define KEY_F15 106
#define KEY_F16 107
#define KEY_F17 108
#define KEY_F18 109
#define KEY_F19 110
#define KEY_F20 111
#define KEY_F21 112
#define KEY_F22 113
#define KEY_F23 114
#define KEY_F24 115
#define KEY_EXECUTE 116
#define KEY_HELP 117
#define KEY_MENU 118
#define KEY_SELECT 119
#define KEY_STOP 120
#define KEY_AGAIN 121
#define KEY_UNDO 122
#define KEY_CUT 123
#define KEY_COPY 124
#define KEY_PASTE 125
#define KEY_FIND 126
#define KEY_MUTE 127
#define KEY_VOLUMEUP 128
#define KEY_VOLUMEDOWN 129
#define KEY_KP_COMMA 133
#define KEY_KP_EQUALSAS400 134
#define KEY_INTERNATIONAL1 135
#define KEY_INTERNATIONAL2 136
#define KEY_INTERNATIONAL3 137
#define KEY_INTERNATIONAL4 138
#define KEY_INTERNATIONAL5 139
#define KEY_INTERNATIONAL6 140
#define KEY_INTERNATIONAL7 141
#define KEY_INTERNATIONAL8 142
#define KEY_INTERNATIONAL9 143
#define KEY_LANG1 144
#define KEY_LANG2 145
#define KEY_LANG3 146
#define KEY_LANG4 147
#define KEY_LANG5 148
#define KEY_LANG6 149
#define KEY_LANG7 150
#define KEY_LANG8 151
#define KEY_LANG9 152
#define KEY_ALTERASE 153
#define KEY_SYSREQ 154
#define KEY_CANCEL 155
#define KEY_CLEAR 156
#define KEY_PRIOR 157
#define KEY_RETURN2 158
#define KEY_SEPARATOR 159
#define KEY_OUT 160
#define KEY_OPER 161
#define KEY_CLEARAGAIN 162
#define KEY_CRSEL 163
#define KEY_EXSEL 164
#define KEY_KP_00 176
#define KEY_KP_000 177
#define KEY_THOUSANDSSEPARATOR 178
#define KEY_DECIMALSEPARATOR 179
#define KEY_CURRENCYUNIT 180
#define KEY_CURRENCYSUBUNIT 181
#define KEY_KP_LEFTPAREN 182
#define KEY_KP_RIGHTPAREN 183
#define KEY_KP_LEFTBRACE 184
#define KEY_KP_RIGHTBRACE 185
#define KEY_KP_TAB 186
#define KEY_KP_BACKSPACE 187
#define KEY_KP_A 188
#define KEY_KP_B 189
#define KEY_KP_C 190
#define KEY_KP_D 191
#define KEY_KP_E 192
#define KEY_KP_F 193
#define KEY_KP_XOR 194
#define KEY_KP_POWER 195
#define KEY_KP_PERCENT 196
#define KEY_KP_LESS 197
#define KEY_KP_GREATER 198
#define KEY_KP_AMPERSAND 199
#define KEY_KP_DBLAMPERSAND 200
#define KEY_KP_VERTICALBAR 201
#define KEY_KP_DBLVERTICALBAR 202
#define KEY_KP_COLON 203
#define KEY_KP_HASH 204
#define KEY_KP_SPACE 205
#define KEY_KP_AT 206
#define KEY_KP_EXCLAM 207
#define KEY_KP_MEMSTORE 208
#define KEY_KP_MEMRECALL 209
#define KEY_KP_MEMCLEAR 210
#define KEY_KP_MEMADD 211
#define KEY_KP_MEMSUBTRACT 212
#define KEY_KP_MEMMULTIPLY 213
#define KEY_KP_MEMDIVIDE 214
#define KEY_KP_PLUSMINUS 215
#define KEY_KP_CLEAR 216
#define KEY_KP_CLEARENTRY 217
#define KEY_KP_BINARY 218
#define KEY_KP_OCTAL 219
#define KEY_KP_DECIMAL 220
#define KEY_KP_HEXADECIMAL 221
#define KEY_LCTRL 224
#define KEY_LSHIFT 225
#define KEY_LALT 226
#define KEY_LGUI 227
#define KEY_RCTRL 228
#define KEY_RSHIFT 229
#define KEY_RALT 230
#define KEY_RGUI 231
#define KEY_MODE 257
#define KEY_AUDIONEXT 258
#define KEY_AUDIOPREV 259
#define KEY_AUDIOSTOP 260
#define KEY_AUDIOPLAY 261
#define KEY_AUDIOMUTE 262
#define KEY_MEDIASELECT 263
#define KEY_WWW 264
#define KEY_MAIL 265
#define KEY_CALCULATOR 266
#define KEY_COMPUTER 267
#define KEY_AC_SEARCH 268
#define KEY_AC_HOME 269
#define KEY_AC_BACK 270
#define KEY_AC_FORWARD 271
#define KEY_AC_STOP 272
#define KEY_AC_REFRESH 273
#define KEY_AC_BOOKMARKS 274
#define KEY_BRIGHTNESSDOWN 275
#define KEY_BRIGHTNESSUP 276
#define KEY_DISPLAYSWITCH 277
#define KEY_KBDILLUMTOGGLE 278
#define KEY_KBDILLUMDOWN 279
#define KEY_KBDILLUMUP 280
#define KEY_EJECT 281
#define KEY_SLEEP 282
#define KEY_APP1 283
#define KEY_APP2 284
#define KEY_AUDIOREWIND 285
#define KEY_AUDIOFASTFORWARD 286

#define MOUSE_BUTTON1 1 // left button
#define MOUSE_BUTTON3 2 // middle button
#define MOUSE_BUTTON2 3 // right button
#define MOUSE_BUTTON4 4 // extra button 1
#define MOUSE_BUTTON5 5 // extra button 2

#define MOUSE_BUTTON_LEFT MOUSE_BUTTON1
#define MOUSE_BUTTON_MIDDLE MOUSE_BUTTON3
#define MOUSE_BUTTON_RIGHT MOUSE_BUTTON2
#define MOUSE_BUTTON_X1 MOUSE_BUTTON4
#define MOUSE_BUTTON_X2 MOUSE_BUTTON5

class InputHandler
{
public:
	InputHandler();
	~InputHandler();

	void init();

	void grabMouse(bool grabbed);

	bool isMouseGrabbed();

	void reset(double delta);

	void update();

	bool keyPressed(int32 key);

	bool keyDown(int32 key);

	bool keyReleased(int32 key);

	bool mouseButtonPressed(int32 button, uint8 count = 0, bool once = true);

	bool mouseButtonDown(int32 button);

	bool mouseButtonReleased(int32 button);

	bool mouseButtonDoubleClicked(int32 button);

	fvec2 getMousePosition() const;

	fvec2 getMouseVelocity() const;
};
