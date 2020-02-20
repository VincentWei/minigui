///////////////////////////////////////////////////////////////////////////////
//
//                          IMPORTANT NOTICE
//
// The following open source license statement does not apply to any
// entity in the Exception List published by FMSoft.
//
// For more information, please visit:
//
// https://www.fmsoft.cn/exception-list
//
//////////////////////////////////////////////////////////////////////////////
/*
 *   This file is part of MiniGUI, a mature cross-platform windowing
 *   and Graphics User Interface (GUI) support system for embedded systems
 *   and smart IoT devices.
 *
 *   Copyright (C) 2002~2018, Beijing FMSoft Technologies Co., Ltd.
 *   Copyright (C) 1998~2002, WEI Yongming
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Or,
 *
 *   As this program is a library, any link to this program must follow
 *   GNU General Public License version 3 (GPLv3). If you cannot accept
 *   GPLv3, you need to be licensed from FMSoft.
 *
 *   If you have got a commercial license of this program, please use it
 *   under the terms and conditions of the commercial license.
 *
 *   For more information about the commercial license, please refer to
 *   <http://www.minigui.com/blog/minigui-licensing-policy/>.
 */
/*
** standalone.c: low-level routines for MiniGUI-Lite standalone version.
**
** Current maintainer: Wei Yongming.
**
** Create date: 2003/08/15
*/

#include <string.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <signal.h>
#include <time.h>
#include <limits.h>
#ifndef WIN32
#include <sys/poll.h>
#endif

#include "common.h"
#include "minigui.h"
#include "gdi.h"
#include "window.h"
#include "cliprect.h"
#include "internals.h"
#include "gal.h"
#include "ial.h"
#include "cursor.h"
#include "event.h"
#include "menu.h"
#include "ourhdr.h"

extern DWORD __mg_timer_counter;
static DWORD old_timer_counter = 0;

#if 0
/* Since 5.0.0, use RegisterEventHookFunc to implement SetServerEventHook */
static SRVEVTHOOK srv_evt_hook = NULL;

SRVEVTHOOK GUIAPI SetServerEventHook (SRVEVTHOOK SrvEvtHook)
{
    SRVEVTHOOK old_hook = srv_evt_hook;

    srv_evt_hook = SrvEvtHook;

    return old_hook;
}
#endif /* deprecated code; moved to window.c */

static void ParseEvent (PMSGQUEUE msg_que, int event)
{
    LWEVENT lwe;
    PMOUSEEVENT me;
    PKEYEVENT ke;
    MSG Msg;

    ke = &(lwe.data.ke);
    me = &(lwe.data.me);
    me->x = 0; me->y = 0;
    Msg.hwnd = HWND_DESKTOP;
    Msg.wParam = 0;
    Msg.lParam = 0;

    lwe.status = 0L;

    if (!kernel_GetLWEvent (event, &lwe))
        return;

    Msg.time = __mg_timer_counter;
    if (lwe.type == LWETYPE_TIMEOUT) {
        Msg.message = MSG_TIMEOUT;
        Msg.wParam = (WPARAM)lwe.count;
        Msg.lParam = 0;

        kernel_QueueMessage (msg_que, &Msg);
    }
    else if (lwe.type == LWETYPE_KEY) {
        Msg.wParam = ke->scancode;
        Msg.lParam = ke->status;
        if (ke->event == KE_KEYDOWN){
            Msg.message = MSG_KEYDOWN;
        }
        else if (ke->event == KE_KEYUP) {
            Msg.message = MSG_KEYUP;
        }
        else if (ke->event == KE_KEYLONGPRESS) {
            Msg.message = MSG_KEYLONGPRESS;
        }
        else if (ke->event == KE_KEYALWAYSPRESS) {
            Msg.message = MSG_KEYALWAYSPRESS;
        }

#if 0   /* Since 5.0.0 */
        if (!(srv_evt_hook && srv_evt_hook (&Msg))) {
            kernel_QueueMessage (msg_que, &Msg);
        }
#else   /* deprecated code */
        if (__mg_check_hook_func (HOOK_EVENT_KEY, &Msg) == HOOK_GOON) {
            kernel_QueueMessage (msg_que, &Msg);
        }
#endif  /* use __mg_check_hook_func instead */
    }
    else if (lwe.type == LWETYPE_MOUSE) {

        switch (me->event) {
        case ME_MOVED:
            Msg.message = MSG_MOUSEMOVE;
            SetCursor (GetSystemCursor (IDC_ARROW));
            break;
        case ME_LEFTDOWN:
            Msg.message = MSG_LBUTTONDOWN;
            break;
        case ME_LEFTUP:
            Msg.message = MSG_LBUTTONUP;
            break;
        case ME_LEFTDBLCLICK:
            Msg.message = MSG_LBUTTONDBLCLK;
            break;
        case ME_RIGHTDOWN:
            Msg.message = MSG_RBUTTONDOWN;
            break;
        case ME_RIGHTUP:
            Msg.message = MSG_RBUTTONUP;
            break;
        case ME_RIGHTDBLCLICK:
            Msg.message = MSG_RBUTTONDBLCLK;
            break;
        }

        Msg.wParam = me->status;
        Msg.lParam = MAKELONG (me->x, me->y);
#if 0   /* Since 5.0.0 */
        if (!(srv_evt_hook && srv_evt_hook (&Msg))) {
            kernel_QueueMessage (msg_que, &Msg);
        }
#else   /* deprecated code */
        if (__mg_check_hook_func (HOOK_EVENT_MOUSE, &Msg) == HOOK_GOON) {
            kernel_QueueMessage (msg_que, &Msg);
        }
#endif  /* use __mg_check_hook_func instead */
    }
}

extern void  __mg_os_start_time_ms(void);
extern DWORD __mg_os_get_time_ms(void);

BOOL GUIAPI salone_StandAloneStartup (void)
{
#if 0
    mg_maxfd = 0;
    mg_fd_zero (&mg_rfdset);
#endif /* deprecated code since 5.0.0 */

#if 0 /* VW: do not use signal based interval timer; since 4.0 */
    mg_InstallIntervalTimer ();
#endif

    __mg_os_start_time_ms ();

    return TRUE;
}

void salone_StandAloneCleanup (void)
{
#if 0 /* VW: do not use signal based interval timer; since 4.0 */
    mg_UninstallIntervalTimer ();
#endif
}

#if 0
BOOL minigui_idle (void)
{
    return salone_IdleHandler4StandAlone (__mg_dsk_msg_queue, TRUE);
}
#endif /* deprecated code */

BOOL salone_IdleHandler4StandAlone (PMSGQUEUE msg_queue, BOOL wait)
{
    int n;
#ifdef __NOUNIX__
    struct timeval sel_timeout = {0, 10000};
#elif defined (_MGGAL_BF533)
    struct timeval sel_timeout = {0, 100000};
#else
    struct timeval sel_timeout = {0, 10000};
#endif
    struct timeval sel_timeout_nd = {0, 0};
    fd_set rset, wset, eset;
    fd_set* rsetptr = NULL;
    fd_set* wsetptr = NULL;
    fd_set* esetptr = NULL;
    EXTRA_INPUT_EVENT extra;    // Since 4.0.0; for extra input events

    if (old_timer_counter != __mg_timer_counter) {
        old_timer_counter = __mg_timer_counter;
        AlertDesktopTimerEvent ();
    }

    /* rset gets modified each time around */
    if (msg_queue->nr_rfds) {
        rset = msg_queue->rfdset;
        rsetptr = &rset;
    }
    if (msg_queue->nr_wfds) {
        wset = msg_queue->wfdset;
        wsetptr = &wset;
    }
    if (msg_queue->nr_efds) {
        eset = msg_queue->efdset;
        esetptr = &eset;
    }

    extra.params_mask = 0;
#ifdef __NOUNIX__
    n = IAL_WaitEvent (msg_queue->maxfd, rsetptr, wsetptr, esetptr,
                wait?&sel_timeout:&sel_timeout_nd, &extra);

    /* update __mg_timer_counter */
    if (wait) {
        //__mg_timer_counter += 10;
        /**
        10 is too fast, the "repeat_threshold" is 5, use ++, repeate 5 times
        **/
        __mg_timer_counter++;
    }
#elif defined (_MGGAL_BF533)
    n = IAL_WaitEvent (msg_queue->maxfd, rsetptr, wsetptr, esetptr,
                wait?&sel_timeout:&sel_timeout_nd, &extra);

    /* update __mg_timer_counter */
    if (wait) {
        __mg_timer_counter += 1;
    }
#else
    n = IAL_WaitEvent (msg_queue->maxfd, rsetptr, wsetptr, esetptr,
                wait?&sel_timeout:&sel_timeout_nd, &extra);
    /* update __mg_timer_counter */
    __mg_timer_counter = __mg_os_get_time_ms()/10;
#endif

    if (n < 0) {
        /* It is time to check event again. */
        if (errno == EINTR) {
            ParseEvent (msg_queue, 0);
        }
        return FALSE;
    }
    else if (!wait)
        return (n > 0);

    /* handle intput event (mouse/touch-screen or keyboard) */
    if (n & IAL_MOUSEEVENT) ParseEvent (msg_queue, IAL_MOUSEEVENT);
    if (n & IAL_KEYEVENT) ParseEvent (msg_queue, IAL_KEYEVENT);
    if (n & IAL_EVENT_EXTRA) {
        MSG msg;
        msg.hwnd = HWND_DESKTOP;
        msg.message = extra.event;
        msg.wParam = extra.wparam;
        msg.lParam = extra.lparam;
        msg.time = __mg_timer_counter;
        if (extra.params_mask) {
            // packed multiple sub events
            int i, n = 0;
            for (i = 0; i < NR_PACKED_SUB_EVENTS; i++) {
                if (extra.params_mask & (1 << i)) {
                    msg.wParam = extra.wparams[i];
                    msg.lParam = extra.lparams[i];
                    if (__mg_check_hook_func (HOOK_EVENT_EXTRA, &msg) ==
                            HOOK_GOON)
                        kernel_QueueMessage (msg_queue, &msg);
                    n++;
                }
            }

            if (n > 0) {
                msg.message = MSG_EXIN_END_CHANGES;
                msg.wParam = n;
                msg.lParam = 0;
                if (__mg_check_hook_func (HOOK_EVENT_EXTRA, &msg) == HOOK_GOON)
                    kernel_QueueMessage (msg_queue, &msg);
            }
        }
        else {
            if (__mg_check_hook_func (HOOK_EVENT_EXTRA, &msg) == HOOK_GOON)
                kernel_QueueMessage (msg_queue, &msg);
        }
    }
    else if (n == 0)
        ParseEvent (msg_queue, 0);

    /* go through registered listen fds */
    __mg_kernel_check_listen_fds (msg_queue, &rset, wsetptr, esetptr);

    return (n > 0);
}

