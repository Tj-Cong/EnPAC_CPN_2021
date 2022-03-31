//
// Created by hecong on 2020/11/14.
//

#ifndef ENPAC_CPN_CPN_H
#define ENPAC_CPN_CPN_H

#include <fstream>
#include "expression.h"
using namespace std;

#define INDEX_ERROR 0xffffffff
extern int stringToNum(const string& str);

/*========================Color_Petri_Net=======================*/
typedef struct CPN_Small_Arc
{
    index_t idx;
    hlinscription arc_exp;
} CSArc;

typedef struct CPN_Place
{
    bool significant = false;
    string id;                  /*place id*/
    type tid;                   /*type(place): dot|usersort|productsort|finiteintrange*/
    SORTID sid;                 /*the index of type(place) in sorttable*/
    SHORTIDX metacount=0;
    index_t project_idx;        /*库所映射到marking的idx，用于切片算法*/
    vector<CSArc> producer;
    vector<CSArc> consumer;
    hlinitialmarking *init_exp;    /*initial marking*/
    Multiset initM;             /*initial marking*/
    vector<unsigned char> atomicLinks; //库所关联原子命题序列
} CPlace;

typedef struct CPN_Transition
{
    string id;                   /*transition id*/
    condition_tree guard;        /*guard function*/
    bool hasguard;               /*hasguard=false => gurad is always evaluated to be true*/
    //bool significant = false;
    vector<CSArc> producer;
    vector<CSArc> consumer;
    vector<VARID> relvararray;   /*related variables*/

    void varinsert(VARID vid);
    void get_relvariables();
    void get_relvariables(arcnode *node);
} CTransition;

typedef struct CPN_Arc
{
    string id;
    bool isp2t;
    string source_id;
    string target_id;
    hlinscription arc_exp;

    ~CPN_Arc(){arc_exp.Tree_Destructor();}
} CArc;

class CPN
{
public:
    CPlace *place;                      /*place table*/
    CTransition *transition;            /*transition table*/
    CArc *arc;                          /*arc table*/
    NUM_t placecount;                   /*the number of places*/
    NUM_t transitioncount;              /*the number of transitions*/
    NUM_t arccount;                     /*the number of arcs*/
    NUM_t varcount;                     /*the number of variables*/
    map<string,index_t> mapPlace;       /*Quick index structure of places by place's id*/
    map<string,index_t> mapTransition;  /*Quick index structure of transitions bt transition's id*/

    //slice+
    vector<index_t> slice_p;             /*切片库所*/
    vector<index_t> slice_t;             /*切片变迁*/
    vector<index_t> t_order;            /*变迁序*/
    bool is_slice;                      /*标识是否切片*/
    //slice-
    CPN();

    /*This function is to first scan the pnml file and do these things:
     * 1. get the number of places,transitions,arcs and variables;
     * 2. alloc space for place table, transition table, arc table,
     * P_sorttable and variable table;
     * 3. parse declarations, get all sorts;
     * */
    void getSize(char *filename);

    /*This function is to second scan pnml file just after getSize();
     * It does these jobs:
     * 1.parse pnml file, get every place's information,especially initMarking,
     * get every transition's information,especially guard fucntion;
     * get every arc's information, especially arc expressiion
     * 2.get all variable's information, set up variable table;
     * 3.construct the CPN, complete every place and transition's producer and consumer
     * */
    void readPNML(char *filename);

    /*get initial marking (translate hlinitialmarking into a multiset)*/
    void getInitMarking(CPlace &pp, TiXmlElement *xmlnode);

    /*根据库所id得到他在库所表中的索引位置*/
    index_t getPPosition(string str);

    /*根据变迁id得到他在变迁表中的索引位置*/
    index_t getTPosition(string str);

    /*print CPN by dot language*/
    void printCPN();

    /*print all sorts information*/
    void printSort();

    /*print all variables' information*/
    void printVar();

    /*print every tranition's related variable*/
    void printTransVar();

    void SLICE(vector<string> criteria_p,vector<string> criteria_t);
//    void computeVis(set<index_t> &visItems,bool cardinality);
//
//    void VISpread(set<index_t> &visTransitions);
    ~CPN();

//    bool utilizeSlice();
};

#endif //ENPAC_CPN_CPN_H
