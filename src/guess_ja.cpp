/*
 * This file is part of the KDE libraries
 *
 * Copyright (c) 2000-2003 Shiro Kawai, All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  3. Neither the name of the authors nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/* Edited by İsmail Dönmez <ismail@kde.org> (C) 2005 for Konversation */
/*
 * original code is here.
 * http://cvs.sourceforge.net/viewcvs.py/gauche/Gauche/ext/charconv/guess.c?view=markup
 */

#include <stdlib.h>
#include "guess_ja.h"

enum JapaneseCode::Type JapaneseCode::guess_jp(const char *buf, int buflen)
{
    int i;
    guess_dfa *top = NULL;

    for (i=0; i<buflen; i++)
    {
        int c = (unsigned char)buf[i];

        /* special treatment of jis escape sequence */
        if (c == 0x1b || last_JIS_escape)
        {
            if (i < buflen-1)
            {
                if (last_JIS_escape)
                    c = (unsigned char)buf[i];
                else
                    c = (unsigned char)buf[++i];
                last_JIS_escape = false;

                if (c == '$' || c == '(')
                {
                    return JapaneseCode::JIS;
                }
            }
            else
            {
                last_JIS_escape = true;
            }
        }

        if (DFA_ALIVE(eucj))
        {
            if (!DFA_ALIVE(sjis) && !DFA_ALIVE(utf8)) return JapaneseCode::EUC;
            DFA_NEXT(eucj, c);
        }
        if (DFA_ALIVE(sjis))
        {
            if (!DFA_ALIVE(eucj) && !DFA_ALIVE(utf8)) return JapaneseCode::SJIS;
            DFA_NEXT(sjis, c);
        }
        if (DFA_ALIVE(utf8))
        {
            if (!DFA_ALIVE(sjis) && !DFA_ALIVE(eucj)) return JapaneseCode::UTF8;
            DFA_NEXT(utf8, c);
        }

        if (!DFA_ALIVE(eucj) && !DFA_ALIVE(sjis) && !DFA_ALIVE(utf8))
        {
            /* we ran out the possibilities */
            return JapaneseCode::ASCII;
        }
    }

    /* ascii code check */
    if (eucj->score == 1.0 && sjis->score == 1.0 && utf8->score == 1.0)
        return JapaneseCode::ASCII;

    /* Now, we have ambigous code.  Pick the highest score.  If more than
       one candidate tie, pick the default encoding. */
    if (DFA_ALIVE(eucj)) top = eucj;
    if (DFA_ALIVE(utf8))
    {
        if (top)
        {
            if (top->score <  utf8->score) top = utf8;
        }
        else
        {
            top = utf8;
        }
    }
    if (DFA_ALIVE(sjis))
    {
        if (top)
        {
            if (top->score <= sjis->score) top = sjis;
        }
        else
        {
            top = sjis;
        }
    }

    if (top == eucj) return JapaneseCode::EUC;
    if (top == utf8) return JapaneseCode::UTF8;
    if (top == sjis) return JapaneseCode::SJIS;

    return JapaneseCode::ASCII;
}
