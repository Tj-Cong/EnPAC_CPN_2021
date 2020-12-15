//
// Created by hecong on 2020/11/14.
//

/********************************************************************************\
|* Notice: usersort is cyclicenumeration sort                                   *|
|*         productsort can only be composed by usersort and finiteintrange sort *|
\********************************************************************************/

#ifndef ENPAC_CPN_SORT_H
#define ENPAC_CPN_SORT_H

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <cstring>
#include <gperftools/tcmalloc.h>
#include <gperftools/malloc_extension.h>
#include "base.h"
using namespace std;

#define MAXSHORT 0xffff
#define H1FACTOR 13
#define MAXCOLORID 0x7fff
#define SUCCESS 1
#define CONFLICT -1
#define FAIL 0

typedef short COLORID;
typedef unsigned short SORTID;
typedef unsigned short VARID;
typedef unsigned short SHORTIDX;
typedef unsigned short SHORTNUM;
typedef unsigned int index_t;    //索引数据类型
typedef unsigned int NUM_t;

enum type{dot,finiteintrange,productsort,usersort};
int pattern_merge(COLORID *ret, const COLORID *array1, const COLORID *array2, int length);
int pattern_merge(COLORID *array1, const COLORID *array2, int length);

typedef struct mapsort_info
{
    SORTID sid;       /*the index of this sort in sorttable*/
    type tid;         /*sort category: dot|finiteintrange|productsort|usersort*/
} MSI;

typedef struct mapcolor_info  //only used for members of usersort
{
    SORTID sid;       /*the index of the sort that the color belongs to*/
    COLORID cid;      /*the index of the color in its sort color table*/
} MCI;

/*==========================Sort=======================*/
class Sort
{
public:
    type mytype;
    virtual ~Sort(){};
};

/*
 * ProductSort can be composed by UserSort and FinteIntRange
 * */
class ProductSort:public Sort
{
public:
    string id;                   /*sort name*/
    SHORTNUM sortnum;            /*the number of sorts that consists of this product sort*/
    vector<string> sortname;     /*every sort members' name (id)*/
    vector<MSI> sortid;          /*every sort members' information*/
};

/*
 * UserSort is a user declared cyclic enumerable sort
 * */
class UserSort:public Sort
{
public:
    string id;                   /*sort name*/
    SHORTNUM feconstnum;         /*the number of colors*/
    vector<string> members;      /*every colors' name*/
    map<string,COLORID> mapValue;  /*fast index structure of colors*/
};

class FiniteIntRange:public Sort
{
public:
    string id;                   /*sort name*/
    int start;                   /*the start of the range*/
    int end;                     /*the end of the range*/
};

/*=======================SortValue=====================*/
class SortValue
{
public:
    virtual ~SortValue(){};
    virtual void setColor(COLORID cid)=0;
    virtual void setColor(COLORID *cid,int size)=0;
    virtual void setColor(COLORID cid,int position)=0;
    virtual void getColor(COLORID &cid)=0;
    virtual void getColor(COLORID *cid,int size)=0;
    virtual void getColor(COLORID &cid,int position)=0;
};

class ProductSortValue:public SortValue
{
private:
    COLORID *valueindex;
public:
    ProductSortValue(int sortnum) {
        valueindex = new COLORID[sortnum];
        for(int i=0;i<sortnum;++i) {
            valueindex[i] = MAXCOLORID;
        }
//        memset(valueindex,MAXCOLORID,sizeof(COLORID)*sortnum);
    }
    ~ProductSortValue() {
        delete [] valueindex;
    }
    void setColor(COLORID cid){};
    void setColor(COLORID cid,int position){valueindex[position] = cid;}
    void setColor(COLORID *cid,int size) {
        memcpy(valueindex,cid,sizeof(COLORID)*size);
    }
    void getColor(COLORID &cid){};
    void getColor(COLORID &cid,int position){cid = valueindex[position];}
    void getColor(COLORID *cid,int size) {
        memcpy(cid,valueindex,sizeof(COLORID)*size);
    }
};

class UserSortValue:public SortValue
{
private:
    COLORID colorindex;
public:
    UserSortValue() {colorindex = MAXCOLORID;}
    void setColor(COLORID *cid,int size){};
    void setColor(COLORID cid,int position){};
    void setColor(COLORID cid) {colorindex = cid;}
    void getColor(COLORID &cid) {cid = colorindex;}
    void getColor(COLORID *cid,int size){};
    void getColor(COLORID &cid,int position){};
    ~UserSortValue(){};
};

class FIRSortValue:public SortValue
{
private:
    COLORID value;
public:
    FIRSortValue() {value = MAXCOLORID;}
    void setColor(COLORID *cid,int size){};
    void setColor(COLORID cid,int position){};
    void setColor(COLORID cid){value = cid;}
    void getColor(COLORID &cid){value = cid;}
    void getColor(COLORID *cid,int size){};
    void getColor(COLORID &cid,int position){};
    ~FIRSortValue(){};
};

class SortTable
{
public:
    static vector<ProductSort> productsort;
    static vector<UserSort> usersort;
    static vector<FiniteIntRange> finitintrange;
    static map<string,MSI> mapSort;
    static map<string,MCI> mapColor;
};

class Tokens
{
public:
    SortValue *tokenvalue;
    SHORTNUM tokencount;
    Tokens *next;

    Tokens() {tokenvalue=NULL;tokencount=0;next=NULL;}
    ~Tokens() {
        if(tokenvalue!=NULL)
            delete tokenvalue;
    }
    void initiate(SHORTNUM tc, type sort, int PSnum=0);
};

struct Variable
{
    string id;          /*variable's name*/
    SHORTIDX upbound;   /*codomain*/
    type tid;           /*type category: dot,finiteintrange,productsort,usersort*/
    SORTID sid;         /*the index of this variable's sort in sorttable*/
    VARID vid;          /*its index in variable table*/

    bool operator == (const Variable &var) const {
        if(this->id == var.id)
            return true;
        else
            return false;
    }
    bool operator < (const Variable &var) const {
        if(this->vid < var.vid)
            return true;
        else
            return false;
    }
};

class VarTable
{
public:
    static Variable *vartable;
    static map<string,VARID> mapVariable;
    static SHORTNUM varcount;
};

class Multiset
{
public:
    Tokens *tokenQ;      //带有头结点的队列；
    SHORTIDX colorcount;
    type tid;
    SORTID sid;
    index_t hashvalue;

    Multiset();
    ~Multiset();
    void initiate(type t,SORTID s);
    void insert(Tokens *token);
    void clear();
    index_t Hash();
    NUM_t Tokensum() const;
    void unify_coefficient(SHORTNUM coefficient);
    void printToken() const;
    void printToken(string &str) const;
    bool operator>=(const Multiset &mm) const;
};

/*copy t2 into t1: t2=t1
 * @param t1 :out
 * @param t2 :in
 */
void Tokenscopy(Tokens &t1,const Tokens &t2,type tid,int PSnum=0);

/*copy mm2 into mm1: mm1=mm2
 * @param mm2: in;
 * @param mm1: out;
 */
void MultisetCopy(Multiset &mm1, const Multiset &mm2, type tid, SORTID sid);

/*multiset operator subtract
 * mm1 = mm1 -- mm2
 * */
int MINUS(Multiset &mm1, const Multiset &mm2);

/*multiset operator add
 * mm1=mm1 ++ mm2
 * */
void PLUS(Multiset &mm1, const Multiset &mm2);

#endif //ENPAC_CPN_SORT_H
