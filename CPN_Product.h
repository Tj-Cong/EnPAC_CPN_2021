//
// Created by hecong on 2020/12/4.
//

#ifndef ENPAC_CPN_CPN_PRODUCT_H
#define ENPAC_CPN_CPN_PRODUCT_H

#include <thread>
#include <unistd.h>
#include <signal.h>
#include "CPN_RG.h"
#include "BA/buchi.h"

#define UNREACHABLE 0xffffffff
#define CPSTACKSIZE 33554432  //2^25
#define CHASHSIZE 1048576  //2^20

extern CPN_RG *cpnRG;
extern bool ready2exit;
extern bool timeflag;
extern short int total_mem;
extern short int total_swap;
extern bool consistency;
extern pid_t mypid;

typedef struct last_firing_info {
    bool virgin = true;
    SHORTIDX tranid = 0;
    binding *fireBinding = NULL;
} LFI;

class ProductState
{
public:
    index_t id;
    CPN_RGNode *RGname_ptr;
    int BAname_id;
    index_t stacknext;
    ArcNode *pba;
    LFI fireinfo;
    CPN_RGNode *cur_RGchild;
    ProductState();
    void getNextRGChild(bool &exist);
    int NEXTTRANSITION();
    int NEXTBINDING();
};

class S_ProductState
{
public:
    CPN_RGNode *RGname_ptr;
    int BAname_id;
    S_ProductState *hashnext;
public:
    S_ProductState(ProductState *node){
        this->RGname_ptr = node->RGname_ptr;
        this->BAname_id = node->BAname_id;
        hashnext = NULL;
    }
};

class CHashtable
{
public:
    S_ProductState **table;
    NUM_t nodecount;
    NUM_t hash_conflict_times;
public:
    CHashtable();
    ~CHashtable();
    index_t hashfunction(ProductState *node);
    index_t hashfunction(S_ProductState *node);
    void insert(ProductState *node);
    S_ProductState *search(ProductState *node);
    void remove(ProductState *node);
    void resetHash();
};

class CPstack
{
public:
    ProductState **mydata;
    index_t *hashlink;
    index_t toppoint;

    CPstack();
    ~CPstack();
    index_t hashfunction(ProductState *node);
    ProductState *top();
    ProductState *pop();
    ProductState *search(ProductState *node);
    int push(ProductState *node);
    NUM_t size();
    bool empty();
    void clear();
};

class CPN_Product_Automata
{
private:
    vector<ProductState> initial_status;
    CHashtable h;
    CStack<index_t> astack;
    CStack<index_t> dstack;
    CPstack cstack;
    StateBuchi *ba;
    thread detect_mem_thread;
    int ret;
    bool result;
    bool memory_flag;
    bool stack_flag;
public:
    CPN_Product_Automata(StateBuchi *sba);
    ~CPN_Product_Automata();
    bool isLabel(CPN_RGNode *state,int sj);
    bool newisLabel(CPN_RGNode *state,int sj);
    bool judgeF(string s);
    bool checkLTLF(CPN_RGNode *state, atomicmeta &am);
    bool checkLTLC(CPN_RGNode *state, atomicmeta &am);
    bool handleLTLF(string s, CPN_RGNode *state);
    bool handleLTLC(string s, CPN_RGNode *state);
    void handleLTLCstep(NUM_t &front_sum, NUM_t &latter_sum, string s, CPN_RGNode *state);
    NUM_t sumtoken(string s,CPN_RGNode *state);
    ProductState *getNextChild(ProductState *curstate);
    void TCHECK(ProductState *p0);
    void UPDATE(ProductState *p0);
    int PUSH(ProductState *p0);
    void POP();
    void getinitial_status();
    void getProduct();
    unsigned short ModelChecker(string propertyid,unsigned short each_run_time);
    int getresult(){return ret;}
    void detect_memory();
};
#endif //ENPAC_CPN_CPN_PRODUCT_H
