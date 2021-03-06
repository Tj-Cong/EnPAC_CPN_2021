//
// Created by hecong on 2020/11/26.
//

#ifndef ENPAC_CPN_CPN_RG_H
#define ENPAC_CPN_CPN_RG_H

#include "CPN.h"
#include <iomanip>
#include "BA/atomic.h"

#define CPNRGTABLE_SIZE 1048576
#define random(x) rand()%(x)
#define MAXVARNUM 30

extern CPN *cpn;
extern bool consistency;
extern bool ready2exit;
extern bool timeflag;
class CPN_Product_Automata;

enum InsertStrategy{byorder,byhead};

class binding
{
public:
    COLORID *vararray;
    binding *next;

    /*allocate memory and initiate to MAXCOLORID (0xffffffff)
     * */
    binding();

    binding(const binding &b);
    /*free memory
     * */
    ~binding();

    /*check the completeness of transition tt:
     * whether all related variables of @parm tt have been assigned a value;
     * if not, return the first unassigned variable's vid from @parm vid
     * @parm tt (in)
     * @parm vid (out)
     * */
    bool completeness(vector<VARID> &unassignedvar,SHORTIDX tranid) const;
//    void compress();
//    void decompress(COLORID *varvec);
    void printBinding(string &str);
    bool operator > (const binding &otherbind);
};

class BindingList
{
public:
    binding *listhead;
    NUM_t count;
    InsertStrategy strategy;

    BindingList();
    ~BindingList();
    void insert(binding *p);
    void copy_insert(const COLORID *vararry);
    binding *Delete(binding *outelem);
    bool empty();
};

class CPN_RGNode
{
public:
    Multiset *marking;
    CPN_RGNode *next;
    BindingList *enbindings;
    bool *Binding_Available;
    index_t markingid;
public:
    CPN_RGNode();
    ~CPN_RGNode();
    index_t Hash(SHORTNUM *weight);
    void all_FireableBindings();
    void tran_FireableBindings(SHORTIDX tranid);
    void complete(const vector<VARID> unassignedvar,int level,binding *inbind,SHORTIDX tranid);
//    void compress();
    void printMarking();
    bool isfirable(string transname);
    bool isfirable(index_t tranid);
    bool deadmark();
    NUM_t readPlace(index_t placeid);
    bool operator == (const CPN_RGNode &state);
    void operator = (const CPN_RGNode &state);
};

class CPN_RG
{
private:
    CPN_RGNode **markingtable;
    CPN_RGNode *initnode;
public:
    SHORTNUM *weight;
    NUM_t hash_conflict_times;
    NUM_t nodecount;
    atomictable &AT;

    CPN_RG(atomictable &argAT);
    ~CPN_RG();
    void addRGNode(CPN_RGNode *state);
    CPN_RGNode *CPNRGinitnode();
    bool NodeExist(CPN_RGNode *state,CPN_RGNode *&existstate);
    void Generate();
    void Generate(CPN_RGNode *state);
    CPN_RGNode *RGNode_Child(CPN_RGNode *curstate,binding *bind,SHORTIDX tranid,bool &exist);
    friend class CPN_Product_Automata;
};
#endif //ENPAC_CPN_CPN_RG_H
