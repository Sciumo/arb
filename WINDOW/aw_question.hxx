//  ==================================================================== //
//                                                                       //
//    File      : aw_question.hxx                                        //
//    Purpose   : Functions to ask questions to user                     //
//    Time-stamp: <Wed Jan/23/2002 14:52 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in January 2002          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AW_QUESTION_HXX
#define AW_QUESTION_HXX

#ifndef aw_root_hxx_included
#include <aw_root.hxx>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif

// for simple questions use :  int aw_message(const char *msg, const char *buttons)
//
// if you ask the same question in a loop, it is recommended to use AW_repeated_question
// to avoid asking the same question again and again.
//
// Usage : 1. Create a new instance of AW_repeated_question outside the loop
//         2. call get_answer() inside the loop

class AW_repeated_question {
private:
    int   answer;
    bool  dont_ask_again;
    char *buttons_used;

public:
    AW_repeated_question(bool dont_ask_again_ = false)
        : answer(0),
          dont_ask_again(dont_ask_again_),
          buttons_used(0)
    {}
    virtual ~AW_repeated_question() { free(buttons_used); }

    int get_answer(const char *question, const char *buttons, const char *to_all, bool add_abort);
    // return 0 for first button, 1 for second button, 2 for third button, ...
    // the single buttons are seperated by kommas (i.e. "YES,NO")
    // if add_abort is true an 'ABORT' button is added behind the last
};


#else
#error aw_question.hxx included twice
#endif // AW_QUESTION_HXX

