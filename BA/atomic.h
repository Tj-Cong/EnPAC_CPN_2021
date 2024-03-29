//
// Created by hecong on 2020/9/28.
//

#ifndef ENPAC_2021_ATOMIC_H
#define ENPAC_2021_ATOMIC_H

#include <iostream>
#include <string>
#include <set>
#include "CPN.h"

using namespace std;

#define OK 1
#define CONSISTENCY_ERROR 0
#define ATOMICTABLESIZE 50

extern CPN *cpn;

enum AtomicType{PT_CARDINALITY,PT_FIREABILITY};
enum Evaluation{UNKNOW,TRUE,FALSE};

typedef struct cardmeta {
    short coefficient = 1;
    unsigned int placeid;
    cardmeta *next = NULL;
} cardmeta;

class cardexp {
public:
    int constnum;
    cardmeta *expression;

    cardexp(){constnum=-1;expression = NULL;/*expression=new cardmeta;*/}
    ~cardexp();
    void DestroyExp();
    cardmeta *locate(unsigned int placeid);
    int placenum();
    void insert(cardmeta *meta);
    void MINUS (const cardexp &exp2);
};

class atomicmeta
{
public:
    AtomicType mytype;
    string mystr;
    cardexp leftexp;
    cardexp rightexp;
    vector<unsigned int> fires;
    Evaluation groundtruth;

    bool last_check;
    bool last_check_avaliable;

public:
    atomicmeta(){last_check_avaliable=false;groundtruth=UNKNOW;}

    int addPlace2Exp(bool left, const string &placeName);

    void evaluate();
    int parse();
    int parse_card();
    int parse_fire();
    void transform();
};

typedef struct atomic {
    bool negation;
    unsigned short atomicmeta_link;
} atomic, Atomic;

class atomictable
{
public:
    atomicmeta atomics[ATOMICTABLESIZE];
    int atomiccount;
public:
    atomictable(){atomiccount = 0;}//init, atomics[0] always empty

    void checkRepeat();//check the latest one. If repeated, delete and release memory.
    void linkPlace2atomictable();    //构建库所影响的原子命题序列
};
#endif //ENPAC_2021_ATOMIC_H
