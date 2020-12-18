//
// Created by hecong on 2020/11/13.
//

#ifndef ENPAC_CPN_EXPRESSION_H
#define ENPAC_CPN_EXPRESSION_H

#include <iostream>
#include "BA/tinyxml.h"
#include <map>
#include <sstream>
#include "sort.h"

using namespace std;

class CPN;
class binding;
class BindingList;
int stringToNum(const string& str);
extern CPN *cpn;
extern bool ready2exit;
enum nodetype{Root,Boolean,Relation,variable,useroperator,finiteintrangeconstant};

/***********************************************************************/
/*=====Transition's guard function is an AST(Abstract Syntax Tree)=====*/
/*Here is an example                                                   *
 *                               and                                   *
 *                              /    \                                 *
 *                    greaterthan     equallity                        *
 *                    /     \           /    \                         *
 *                  var  color/var var/color var /color                */
/***********************************************************************/
typedef struct condition_tree_node
{
    //if this node is an operator, myname is the operator's name,
    //like and operator, myname is 'and';
    //if this node is a variable or useroperator,
    //myname is this variable or color's name.
    string myname;
    string myexp;
    COLORID cid;
    nodetype mytype;
    bool mytruth = false;
    condition_tree_node *left = NULL;
    condition_tree_node *right = NULL;
} CTN;

class condition_tree
{
private:
    CTN *root;
public:
    condition_tree();

    void constructor(TiXmlElement *condition);

    void build_step(TiXmlElement *elem,CTN *&curnode);

    void destructor(CTN *node);

    void computeEXP(CTN *node);

    void printEXP(string &str);

    bool judgeGuard(const COLORID *vararray);

    void judgeGuard(CTN *node,const COLORID *cid);

    ~condition_tree();
};

/*************************************************************************\
| *                                                                     * |
| *                            hlinscription                            * |
| *                                                                     * |
\*************************************************************************/
enum hlnodetype {color,numberof,tuple,add,all,predecessor,successor,subtract,var};
typedef struct arcnode
{
    hlnodetype mytype;
    string myexp;
    /*number has multiple explanations:
     * 1.as numberof node, it represents the coefficient
     * 2.as to color, represents the color index
     * 3.as to var node, it represents the sort index*/
    int number;
    int tokencount = 1;
    SHORTIDX position = MAXSHORT;
    type tid;                      /*used for all, color and var*/
    SORTID sid;                    /*used for all, color and var*/
    arcnode *leftnode = NULL;
    arcnode *rightnode = NULL;
} meta;

class hlinscription
{
private:
    arcnode *root;
    type placetid;                       /*place type*/
    SORTID placesid;                     /*place sort id*/
    SHORTIDX tranid;                     /*transition index*/
    NUM_t Tokensum;                      /*the number of tokens*/
    bool pure_multiset;                  /*whether this arc contains multiset operator subtract (pure_multiset=false->contains)*/
    vector<VARID> arcrelvar;                /*The variables involved in this arc*/
    vector<arcnode*> metaentrance;          /*every member is a root node of an arc pattern*/
public:
    /*constructor function
     * create an arcnode for root*/
    hlinscription();

    /*destructor function
     * 1.destroy the whole tree*/
    ~hlinscription();

    /*initiate placetid with @param tid and placesid with @param sid*/
    void initiate(type tid,SORTID sid);

    /*insert a variable into arcrelvar*/
    void insertvar(VARID vid);

    /*set this->tranid*/
    void set_tranid(SHORTIDX tid);

    /*construct the whole AST tree according to xml tree
     * @param hlinscription: the arc expression begins with XmlElement '<hlinscription>'*/
    void Tree_Constructor(TiXmlElement *hlinscription);

    /*construct a subtree (begins with xmlnode and curnode) recursively
     * @param xmlnode:
     * @param curnode: current arcnode;
     * @param tuplenum: used only for 'tuple' node to indicate one's position in the product sort
     * assign (tuplenum) to its leftnode's number, (tuplenum+1) to its rightnode's number;*/
    void Tree_Constructor(TiXmlElement *xmlnode,arcnode *&curnode,int tuplenum=0);

    /*destroy the subtree from @param node*/
    void Tree_Destructor(arcnode *node);

    /*destroy the whole tree*/
    void Tree_Destructor();

    /*print the arc expression into string @parm arcexp*/
    void printEXP(string &arcexp);

    /*this is a recursive function: calculate myexp*/
    void computeEXP(arcnode *node);

    /*calculate the multiset of this arc arccording to @parm binding
     * @parm ms (out): the multiset of arc
     * @parm binding (in): a binding (an variable array whose length is he number of ALL variables)*/
    void to_Multiset(Multiset &ms,const COLORID *binding);

    /*this is a recursive function called by to_Multiset(Multiset &ms,const COLORID *binding)
     * its function is to calculate the multiset of sub-arc-expression whose root node is @parm node
     * @parm node (in): current node
     * @parm ms (out): the calculated multiset
     * @parm binding (in): a binding
     * @paem tokennum (in): the cofficient of a color (default is 1, when nodetype is numberof, it is its number)*/
    void to_Multiset(arcnode *node,Multiset &ms,const COLORID *binding,SHORTNUM tokennum=1);

    /*This is a recursive function: get the product color according to a binding @parm vararray
     * @parm node (in): current node (initially, it is the top tuple node)
     * @parm colarray (out): the calculated product color (an array whose length is the sortnum of this product sort)
     * @parm vararay (in): a binding
     * Types of nodes contained in the tuple subtree：
     * {tuple,predecessor,successor,var,color}*/
    void getTupleColor(arcnode *node,BindingList &bindingList,const COLORID *vararray);

    /*calculate the number of tokens this arc expression contains*/
    void TokenSum(arcnode *node);

    /*get all possible enabled incomplete bindings of this arc under a state @parm marking
     * all the incomplete bindings will be stored in IBS.
     * if this arc is a pure multiset arc (doesn't contain subtract operator),
     * then it will adopt multiset match strategy,
     * otherwise it implements variable enumeration strategy.
     * */
    int get_IBS(const Multiset &marking,const BindingList &givenIBS,BindingList &retIBS);

    /*match the arc multiset with a given multiset to determine variables' value,
     * All possible values of variables will be recorded in a IBS in the form of
     * incomplete bindings.
     * @parm marking (in): the given multiset
     * @parm IBS (out): incomplete binding set
     * */
    int multiset_match(const Multiset &marking,BindingList &IBS);
    int multiset_match(const Multiset &marking,const BindingList &givenIBS,BindingList &retIBS);

    /*This is a recursive function only called by multiset_match
     * match an arc pattern with all the patterns in marking to get all possible enabled
     * incomplete bindings
     * @parm arcmeta (in): the (arcmeta)th member of metaentrance
     * @parm marking (in): the CPN state
     * @parm IBS (out): the set storing all possible enabled partial bindings
     * @parm vararray (in & out): the variable array which can be regarded as an incomplete binding*/
    void arcmeta_multiset_match(int arcmeta,BindingList &IBS,const COLORID *vararray,const Multiset &marking);

    /*A pattern is meta of a multiset.
     * It is composed of a coefficient $k and a color $c or a variable $v.
     * i.e., if a multiset is 1'1 ++ 1'2 ++ 2'(a++), then (1'1),(1'2) and (2'(a++)) are all patterns
     * This function is to match an arc pattern with an given pattern @parm pattern,
     * its goal is to determine the related variables' value involved in the arc pattern.
     * The variables' value will be recorded in the array @parm vararray.
     * @parm curmeta (in): arc pattern (represented by an arc tree)
     * @parm pattern (in): the given pattern
     * @parm vararry (in & out): the variable array which can be regarded as an incomplete binding
     * */
    int arcmeta_pattern_match(arcnode *curmeta,Tokens *pattern,COLORID *vararry);

    /*match a product sort arc pattern with a given pattern
     * and record involved variable's value in vararray.
     * @parm curmeta (in): product sort arc pattern
     * @parm token (in): given pattern
     * @parm vararry (out): the variable array which can be regarded as an incomplete binding
     * return: FAIL or SUCCESS
     * */
    int tuplemeta_pattern_match(arcnode *curmeta,Tokens *token,COLORID *vararry);

    /*get all possible enabled incomplete bindings under state @parm marking
     * by color space exploration strategy
     * @parm marking (in): the CPN state
     * @parm IBS (out): the set storing all possible enabled partial bindings
     * */
    int variable_enumeration(const Multiset &marking,const BindingList &givenIBS,BindingList &retIBS);

    /*this is recursive function: explore every variable's color space to get all
     * possible enabled partial bindings under a state @parm marking
     * @parm relvarpt (in): the (relvarpt)th member of arcrelvar
     * @parm vararray (in & out): current incomplete binding
     * @parm marking (in): a CPN state
     * @parm IBS (out): the set storing all possible enabled partial bindings*/
    void color_space_exploration(int relvarpt,COLORID *vararray,const Multiset &marking,BindingList &IBS);

    bool contain_All_operator(arcnode *curmeta);
    friend struct CPN_Transition;
};

/*************************************************************************\
| *                                                                     * |
| *                          hlinitialmarking                           * |
| *                                                                     * |
\*************************************************************************/

typedef struct initMarking_node
{
    hlnodetype mytype;             /*mytype indicates which operator this node is*/
    Multiset mm;                   /*this node's incomplete multiset*/
    int value;                     /*used when this node is numberof, indicates the number*/
    type tid;                      /*used when this node is all*/
    SORTID sid;                    /*used when this node is all*/
    COLORID cid;
    initMarking_node *leftnode = NULL;
    initMarking_node *rightnode = NULL;
} initMarking_node;

class hlinitialmarking
{
private:
    initMarking_node *root;
    type placetid;
    SORTID placesid;
public:
    hlinitialmarking(type tid,SORTID sid);
    ~hlinitialmarking();
    void Tree_Constructor(TiXmlElement *hlinitialMarking);
    void Tree_Constructor(TiXmlElement *xmlnode,initMarking_node *&initMnode,int tuplenum=0);
    void Tree_Destructor(initMarking_node *node);
    void get_Multiset(Multiset &ms);
    void get_Multiset(initMarking_node *node);
};
#endif //ENPAC_CPN_EXPRESSION_H
