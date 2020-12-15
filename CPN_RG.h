//
// Created by hecong on 2020/11/26.
//

#ifndef ENPAC_CPN_CPN_RG_H
#define ENPAC_CPN_CPN_RG_H

#include "CPN.h"
#include <iomanip>

#define CPNRGTABLE_SIZE 1048576
#define random(x) rand()%(x)

extern CPN *cpn;
extern bool consistency;
class CPN_Product_Automata;

enum InsertStrategy{byorder,byhead};

class binding
{
public:
    COLORID *vararray;
    SHORTIDX tranid;
    binding *next;

    /*allocate memory and initiate to MAXCOLORID (0xffffffff)
     * */
    binding(SHORTIDX tid);

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
    bool completeness(vector<VARID> &unassignedvar) const;
    void printBinding(string &str);
    bool operator > (const binding &otherbind);
};

class BindingList
{
public:
    binding *listhead;
    InsertStrategy strategy;

    BindingList();
    ~BindingList();
    void insert(binding *p);
    void copy_insert(SHORTIDX tranid,const COLORID *vararry);
    binding *Delete(binding *outelem);
    bool empty();
};

class CPN_RGNode
{
public:
    Multiset *marking;
    CPN_RGNode *next;
    BindingList enbindings;
    index_t markingid;
    bool Binding_Available;
public:
    CPN_RGNode();
    ~CPN_RGNode();
    index_t Hash(SHORTNUM *weight);
    void all_FireableBindings();
    void tran_FireableBindings(SHORTIDX tranid);
    void complete(const vector<VARID> unassignedvar,int level,binding *inbind);
    void printMarking();
    bool isfirable(string transname);
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

    CPN_RG();
    ~CPN_RG();
    void addRGNode(CPN_RGNode *state);
    CPN_RGNode *CPNRGinitnode();
    bool NodeExist(CPN_RGNode *state,CPN_RGNode *&existstate);
    void Generate();
    void Generate(CPN_RGNode *state);
    CPN_RGNode *RGNode_Child(CPN_RGNode *curstate,binding *bind,bool &exist);
    friend class CPN_Product_Automata;
};
#endif //ENPAC_CPN_CPN_RG_H
