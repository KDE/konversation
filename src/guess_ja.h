/*
    SPDX-License-Identifier: BSD-3-Clause

    SPDX-FileCopyrightText: 2000-2003 Shiro Kawai <shirok@users.sourceforge.net>
*/

/*
 * original code is here.
 * http://cvs.sourceforge.net/viewcvs.py/gauche/Gauche/ext/charconv/guess.c?view=markup
 */

#ifndef GUESS_JA_H
#define GUESS_JA_H

class guess_arc {
 public:
  unsigned int next;          /* next state */
  double score;               /* score */
};


typedef signed char dfa_table[256];

/* DFA tables declared in guess_ja.cpp */
extern const dfa_table guess_eucj_st[];
extern guess_arc guess_eucj_ar[7];
extern const dfa_table guess_sjis_st[];
extern guess_arc guess_sjis_ar[6];
extern const dfa_table guess_utf8_st[];
extern guess_arc guess_utf8_ar[11];

class guess_dfa {
 public:
  const dfa_table *states;
  const guess_arc *arcs;
  int state;
  double score;
  
 guess_dfa (const dfa_table stable[], const guess_arc *atable) :
  states(stable), arcs(atable)
  {
    state = 0;
    score = 1.0;
  }
};

class JapaneseCode
{
 public:
  enum Type {K_ASCII, K_JIS, K_EUC, K_SJIS, K_UNICODE, K_UTF8 };
  enum Type guess_jp(const char* buf, int buflen);
  
  JapaneseCode () {
    eucj = new guess_dfa(guess_eucj_st, guess_eucj_ar);
    sjis = new guess_dfa(guess_sjis_st, guess_sjis_ar);
    utf8 = new guess_dfa(guess_utf8_st, guess_utf8_ar);
    last_JIS_escape = false;
  }
  
  ~JapaneseCode () {
    delete eucj;
    delete sjis;
    delete utf8;
  }
  
 protected:
  guess_dfa *eucj;
  guess_dfa *sjis;
  guess_dfa *utf8;
  
  bool last_JIS_escape;
};

#define DFA_NEXT(dfa, ch)                               \
    do {                                                \
        int arc__;                                      \
        if (dfa->state >= 0) {                          \
            arc__ = dfa->states[dfa->state][ch];        \
            if (arc__ < 0) {                            \
                dfa->state = -1;                        \
            } else {                                    \
                dfa->state = dfa->arcs[arc__].next;     \
                dfa->score *= dfa->arcs[arc__].score;   \
            }                                           \
        }                                               \
    } while (0)

#define DFA_ALIVE(dfa)  (dfa->state >= 0)

#endif  /* GUESS_JA_H */
