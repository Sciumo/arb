//  ==================================================================== //
//                                                                       //
//    File      : SQ_GroupData.h                                         //
//    Purpose   : We will see!                                           //
//    Time-stamp: <Mon Sep/29/2003 19:10 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Juergen Huber in July - October 2003                        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#ifndef SQ_GROUPADTA_H
#define SQ_GROUPDATA_H

#ifndef _CPP_CSTDDEF
#include <cstddef>
#endif

class SQ_GroupData {

public:
    SQ_GroupData();
    ~SQ_GroupData();
    int  SQ_get_avg_bases() const;
    void SQ_set_avg_bases(int bases);
    bool SQ_is_initialised();

protected:
    int    size;
    int    avg_bases;
    bool   initialised;
};

#else
#error SQ_GroupData.h included twice
#endif
