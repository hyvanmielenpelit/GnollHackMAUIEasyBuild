/* GnollHack File Change Notice: This file has been changed from the original. Date of last change: 2024-08-11 */

/* GnollHack 4.0    nhdefkey.c    $NHDT-Date: 1432512793 2015/05/25 00:13:13 $  $NHDT-Branch: master $:$NHDT-Revision: 1.14 $ */
/* Copyright (c) GnollHack PC Development Team 2003                      */
/* GnollHack may be freely redistributed.  See license for details.      */

/*
 * This is the default GnollHack keystroke processing.
 * It can be built as a run-time loadable dll (nhdefkey.dll).
 * Alternative keystroke handlers can be built using the
 * entry points in this file as a template.
 *
 * Use the defaults.gnh "altkeyhandler" option to set a
 * different dll name (without the ".DLL" extension) to
 * get different processing. Ensure that the dll referenced
 * in defaults.gnh exists in the same directory as GnollHack in
 * order for it to load successfully.
 *
 */

static char where_to_get_source[] = "http://www.GnollHack.org/";
static char author[] = "The GnollHack Development Team";

#include "win32api.h"
#include "hack.h"
#include "wintty.h"

extern HANDLE hConIn;
extern INPUT_RECORD ir;
extern struct sinfo program_state;

char dllname[512];
char *shortdllname;

int FDECL(__declspec(dllexport) __stdcall ProcessKeystroke,
          (HANDLE hConIn, INPUT_RECORD *ir, boolean *valid,
           BOOLEAN_P numberpad, int portdebug));

int WINAPI
DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
    char dlltmpname[512];
    char *tmp = dlltmpname, *tmp2;
    *(tmp + GetModuleFileName(hInstance, tmp, 511)) = '\0';
    (void) strcpy(dllname, tmp);
    tmp2 = strrchr(dllname, '\\');
    if (tmp2) {
        tmp2++;
        shortdllname = tmp2;
    }
    return TRUE;
}

/*
 *  Keyboard translation tables.
 *  (Adopted from the MSDOS port)
 */

#define KEYPADLO 0x47
#define KEYPADHI 0x53

#define PADKEYS (KEYPADHI - KEYPADLO + 1)
#define iskeypad(x) (KEYPADLO <= (x) && (x) <= KEYPADHI)

/*
 * Keypad keys are translated to the normal values below.
 * Shifted keypad keys are translated to the
 *    shift values below.
 */

static const struct pad {
    uchar normal, shift, cntrl;
} keypad[PADKEYS] =
    {
      { 'y', 'Y', C('y') },    /* 7 */
      { 'k', 'K', C('k') },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { 'h', 'H', C('h') },    /* 4 */
      { 'g', 'G', 'g' },       /* 5 */
      { 'l', 'L', C('l') },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { 'b', 'B', C('b') },    /* 1 */
      { 'j', 'J', C('j') },    /* 2 */
      { 'n', 'N', C('n') },    /* 3 */
      { 'i', 'I', C('i') },    /* Ins */
      { '.', ':', ':' }        /* Del */
    },
 gnhpad[PADKEYS] =
    {
      { 'y', 'Y', C('y') },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { 'u', 'U', C('u') },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { '4', M('4'), '4' },    /* 4 */
      { '5', M('5'), '5' },    /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { 'h', 'H', C('h') },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { 'j', 'J', C('j') },    /* 3 */
      { '0', M('0'), '0' },    /* Ins */
      { '.', ':', ':' }        /* Del */
    },
 numpad[PADKEYS] = {
      { '7', M('7'), '7' },    /* 7 */
      { '8', M('8'), '8' },    /* 8 */
      { '9', M('9'), '9' },    /* 9 */
      { 'm', C('p'), C('p') }, /* - */
      { '4', M('4'), '4' },    /* 4 */
      { '5', M('5'), '5' },    /* 5 */
      { '6', M('6'), '6' },    /* 6 */
      { '+', 'P', C('p') },    /* + */
      { '1', M('1'), '1' },    /* 1 */
      { '2', M('2'), '2' },    /* 2 */
      { '3', M('3'), '3' },    /* 3 */
      { '0', M('0'), '0' },    /* Ins */
      { '.', ':', ':' }        /* Del */
  };

#define inmap(x, vk) (((x) > 'A' && (x) < 'Z') || (vk) == 0xBF || (x) == '2')

static BYTE KeyState[256];

int __declspec(dllexport) __stdcall ProcessKeystroke(hConIn, ir, valid,
                                                     numberpad, portdebug)
HANDLE hConIn;
INPUT_RECORD *ir;
boolean *valid;
boolean numberpad;
int portdebug;
{
    int metaflags = 0, k = 0;
    int keycode, vk;
    unsigned char ch, pre_ch, mk = 0;
    unsigned short int scan;
    unsigned long shiftstate;
    int altseq = 0;
    const struct pad *kpad;

    shiftstate = 0L;
    ch = pre_ch = ir->Event.KeyEvent.uChar.AsciiChar;
    scan = ir->Event.KeyEvent.wVirtualScanCode;
    vk = ir->Event.KeyEvent.wVirtualKeyCode;
    keycode = MapVirtualKey(vk, 2);
    shiftstate = ir->Event.KeyEvent.dwControlKeyState;
    KeyState[VK_SHIFT] = (shiftstate & SHIFT_PRESSED) ? 0x81 : 0;
    KeyState[VK_CONTROL] =
        (shiftstate & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) ? 0x81 : 0;
    KeyState[VK_CAPITAL] = (shiftstate & CAPSLOCK_ON) ? 0x81 : 0;

    if (shiftstate & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
        if (ch || inmap(keycode, vk))
            altseq = 1;
        else
            altseq = -1; /* invalid altseq */
    }
    if (ch || (iskeypad(scan)) || (altseq > 0))
        *valid = TRUE;
    /* if (!valid) return 0; */
    /*
     * shiftstate can be checked to see if various special
     * keys were pressed at the same time as the key.
     * Currently we are using the ALT & SHIFT & CONTROLS.
     *
     *           RIGHT_ALT_PRESSED, LEFT_ALT_PRESSED,
     *           RIGHT_CTRL_PRESSED, LEFT_CTRL_PRESSED,
     *           SHIFT_PRESSED,NUMLOCK_ON, SCROLLLOCK_ON,
     *           CAPSLOCK_ON, ENHANCED_KEY
     *
     * are all valid bit masks to use on shiftstate.
     * eg. (shiftstate & LEFT_CTRL_PRESSED) is true if the
     *      left control key was pressed with the keystroke.
     */
    if (iskeypad(scan)) {
        kpad = numberpad == 2 ? gnhpad : numberpad ? numpad : keypad;
        if (shiftstate & SHIFT_PRESSED) {
            ch = kpad[scan - KEYPADLO].shift;
        } else if (shiftstate & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
            ch = kpad[scan - KEYPADLO].cntrl;
        } else {
            ch = kpad[scan - KEYPADLO].normal;
        }
    } else if (altseq > 0) { /* ALT sequence */
        if (vk == 0xBF)
            ch = M('?');
        else
            ch = M(tolower((uchar) keycode));
    }
    /* Attempt to work better with international keyboards. */
    else {
        WORD chr[2];
        k = ToAscii(vk, scan, KeyState, chr, 0);
        if (k <= 2)
            switch (k) {
            case 2: /* two characters */
                ch = (unsigned char) chr[1];
                *valid = TRUE;
                break;
            case 1: /* one character */
                ch = (unsigned char) chr[0];
                *valid = TRUE;
                break;
            case 0:  /* no translation */
            default: /* negative */
                *valid = FALSE;
            }
    }
    if (ch == '\r')
        ch = '\n';
#ifdef PORT_DEBUG
    if (portdebug) {
        char buf[BUFSZ];
        Sprintf(buf, "PORTDEBUG (%s): ch=%u, sc=%u, vk=%d, pre=%d, sh=0x%X, "
                     "ta=%d (ESC to end)",
                shortdllname, ch, scan, vk, pre_ch, shiftstate, k);
        fprintf(stdout, "\n%s", buf);
    }
#endif
    return ch;
}

int __declspec(dllexport) __stdcall NHkbhit(hConIn, ir)
HANDLE hConIn;
INPUT_RECORD *ir;
{
    int done = 0; /* true =  "stop searching"        */
    int retval;   /* true =  "we had a match"        */
    DWORD count;
    unsigned short int scan;
    unsigned char ch;
    unsigned long shiftstate;
    int altseq = 0, keycode, vk;
    done = 0;
    retval = 0;
    while (!done) {
        count = 0;
        PeekConsoleInput(hConIn, ir, 1, &count);
        if (count > 0) {
            if (ir->EventType == KEY_EVENT && ir->Event.KeyEvent.bKeyDown) {
                ch = ir->Event.KeyEvent.uChar.AsciiChar;
                scan = ir->Event.KeyEvent.wVirtualScanCode;
                shiftstate = ir->Event.KeyEvent.dwControlKeyState;
                vk = ir->Event.KeyEvent.wVirtualKeyCode;
                keycode = MapVirtualKey(vk, 2);
                if (shiftstate & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
                    if (ch || inmap(keycode, vk))
                        altseq = 1;
                    else
                        altseq = -1; /* invalid altseq */
                }
                if (ch || iskeypad(scan) || altseq) {
                    done = 1;   /* Stop looking         */
                    retval = 1; /* Found what we sought */
                } else {
                    /* Strange Key event; let's purge it to avoid trouble */
                    ReadConsoleInput(hConIn, ir, 1, &count);
                }

            } else if ((ir->EventType == MOUSE_EVENT
                        && (ir->Event.MouseEvent.dwButtonState & MOUSEMASK))) {
                done = 1;
                retval = 1;
            }

            else /* Discard it, it's an insignificant event */
                ReadConsoleInput(hConIn, ir, 1, &count);
        } else /* There are no events in console event queue */ {
            done = 1; /* Stop looking               */
            retval = 0;
        }
    }
    return retval;
}

int __declspec(dllexport) __stdcall CheckInput(hConIn, ir, count, numpad,
                                               mode, mod, cc)
HANDLE hConIn;
INPUT_RECORD *ir;
DWORD *count;
boolean numpad;
int mode;
int *mod;
coord *cc;
{
#if defined(SAFERHANGUP)
    DWORD dwWait;
#endif
    int ch;
    boolean valid = 0, done = 0;
    while (!done) {
#if defined(SAFERHANGUP)
        dwWait = WaitForSingleObjectEx(hConIn,   // event object to wait for
                                       INFINITE, // waits indefinitely
                                       TRUE);    // alertable wait enabled
        if (dwWait == WAIT_FAILED)
            return '\033';
#endif
        ReadConsoleInput(hConIn, ir, 1, count);
        if (mode == 0) {
            if ((ir->EventType == KEY_EVENT) && ir->Event.KeyEvent.bKeyDown) {
                ch = ProcessKeystroke(hConIn, ir, &valid, numpad, 0);
                done = valid;
            }
        } else {
            if (count > 0) {
                if (ir->EventType == KEY_EVENT
                    && ir->Event.KeyEvent.bKeyDown) {
                    ch = ProcessKeystroke(hConIn, ir, &valid, numpad, 0);
                    if (valid)
                        return ch;
                } else if (ir->EventType == MOUSE_EVENT) {
                    if ((ir->Event.MouseEvent.dwEventFlags == 0)
                        && (ir->Event.MouseEvent.dwButtonState & MOUSEMASK)) {
                        cc->x = ir->Event.MouseEvent.dwMousePosition.X + 1;
                        cc->y = ir->Event.MouseEvent.dwMousePosition.Y - 1;

                        if (ir->Event.MouseEvent.dwButtonState & LEFTBUTTON)
                            *mod = CLICK_PRIMARY;
                        else if (ir->Event.MouseEvent.dwButtonState & RIGHTBUTTON)
                            *mod = CLICK_SECONDARY;
                    else if (ir->Event.MouseEvent.dwButtonState & MIDBUTTON)
                          *mod = CLICK_TERTIARY;
                        return 0;
                    }
                }
            } else
                done = 1;
        }
    }
    return mode ? 0 : ch;
}

int __declspec(dllexport) __stdcall SourceWhere(buf)
char **buf;
{
    if (!buf)
        return 0;
    *buf = where_to_get_source;
    return 1;
}

int __declspec(dllexport) __stdcall SourceAuthor(buf)
char **buf;
{
    if (!buf)
        return 0;
    *buf = author;
    return 1;
}

int __declspec(dllexport) __stdcall KeyHandlerName(buf, full)
char **buf;
int full;
{
    if (!buf)
        return 0;
    if (full)
        *buf = dllname;
    else
        *buf = shortdllname;
    return 1;
}
