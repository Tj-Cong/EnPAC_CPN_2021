//
// Created by hecong on 2020/11/14.
//
#include "sort.h"

vector<ProductSort> SortTable::productsort;
vector<UserSort> SortTable::usersort;
vector<FiniteIntRange> SortTable::finitintrange;
map<string,MSI> SortTable::mapSort;
map<string,MCI> SortTable::mapColor;

Variable* VarTable::vartable = NULL;
map<string,VARID> VarTable::mapVariable;
SHORTNUM VarTable::varcount = 0;

void Tokens::initiate(SHORTNUM tc, type sort, int PSnum) {
    tokencount = tc;
    switch(sort)
    {
        case type::productsort:{
            if(PSnum!=0)
                tokenvalue = new ProductSortValue(PSnum);
            break;
        }
        case type::usersort:{
            tokenvalue = new UserSortValue;
            break;
        }
        case type::dot:{
            tokenvalue = NULL;
            break;
        }
        case type::finiteintrange:{
            tokenvalue = new FIRSortValue;
            break;
        }
        default:{
            cerr<<"Sorry! Can not support this sort."<<endl;
            exit(-1);
        }
    }
}

/* 1.找到要插入的位置，分为三种：i.末尾;ii.中间;iii.开头
 * 2.插入到该插入的位置，并设置colorcount++；
 * 3.如果是相同的color，仅更改tokencount；
 * */
void Multiset::insert(Tokens *t) {
    if(tid == usersort || tid == finiteintrange)
    {
        Tokens *q=tokenQ,*p = tokenQ->next;
        COLORID pcid,tcid;
        t->tokenvalue->getColor(tcid);
        while(p!=NULL)
        {
            p->tokenvalue->getColor(pcid);
            if(tcid<=pcid)
                break;
            q=p;
            p=p->next;
        }
        if(p == NULL)
        {
            q->next = t;
            colorcount++;
            return;
        }
        if(tcid == pcid)
        {
            p->tokencount+=t->tokencount;
            delete t;
        }
        else if(tcid<pcid)
        {
            t->next = p;
            q->next = t;
            colorcount++;
        }
    }
    else if(tid == productsort)
    {
        Tokens *q=tokenQ,*p = tokenQ->next;
        COLORID *pcid,*tcid;
        int sortnum = SortTable::productsort[sid].sortnum;
        pcid = new COLORID[sortnum];
        tcid = new COLORID[sortnum];
        t->tokenvalue->getColor(tcid, sortnum);
        while(p!=NULL)
        {
            p->tokenvalue->getColor(pcid, sortnum);
            if(array_lessorequ(tcid,pcid,sortnum))
                break;
            q=p;
            p=p->next;
        }
        if(p == NULL)
        {
            q->next = t;
            colorcount++;
            delete [] pcid;
            delete [] tcid;
            return;
        }
        if(array_equal(tcid,pcid,sortnum))
        {
            p->tokencount += t->tokencount;
            delete t;
        }
        else if(array_less(tcid,pcid,sortnum))
        {
            t->next = p;
            q->next = t;
            colorcount++;
        }
        delete [] pcid;
        delete [] tcid;
    }
    else if(tid == dot)
    {
        if(tokenQ->next == NULL)
            tokenQ->next=t;
        else {
            tokenQ->next->tokencount+=t->tokencount;
            delete t;
        }
        colorcount = 1;
    }
}

void Multiset::clear() {
    Tokens *p,*q;
    p=tokenQ->next;
    while(p)
    {
        q=p->next;
        delete p;
        p=q;
    }
    tokenQ->next = NULL;
}

bool Multiset::operator>=(const Multiset &mm) const
{
    bool greaterorequ = true;
    Tokens *p1,*p2;
    p1 = this->tokenQ->next;
    p2 = mm.tokenQ->next;

    /*a quick pre-check*/
    if(mm.colorcount==0)
        return true;
    else if(this->colorcount == 0)
        return false;

    /*formal check begins:*/
    if(this->tid == dot)
    {
        if(p1->tokencount>=p2->tokencount)
            greaterorequ = true;
        else
            greaterorequ = false;
    }
    else if(this->tid == usersort || this->tid == finiteintrange)
    {
        COLORID cid1,cid2;
        while(p2)
        {
            /*p2中的元素，p1中必须都有；遍历p2*/
            p2->tokenvalue->getColor(cid2);
            while(p1)
            {
                p1->tokenvalue->getColor(cid1);
                if(cid1<cid2) {
                    p1=p1->next;
                    continue;
                }
                else if(cid1==cid2) {
                    if(p1->tokencount>=p2->tokencount) {
                        break;
                    }
                    else {
                        greaterorequ = false;
                        break;
                    }
                }
                else if(cid1>cid2) {
                    greaterorequ = false;
                    break;
                }
            }
            if(p1 == NULL)
                greaterorequ = false;
            if(!greaterorequ)
                break;
            else {
                p2=p2->next;
            }
        }
    }
    else if(this->tid == productsort)
    {
        int sortnum = SortTable::productsort[sid].sortnum;
        COLORID *cid1,*cid2;
        cid1 = new COLORID[sortnum];
        cid2 = new COLORID[sortnum];
        while(p2)
        {
            p2->tokenvalue->getColor(cid2, sortnum);
            while(p1)
            {
                p1->tokenvalue->getColor(cid1, sortnum);
                if(array_less(cid1,cid2,sortnum)) {
                    p1=p1->next;
                    continue;
                }
                else if(array_equal(cid1,cid2,sortnum)) {
                    if(p1->tokencount>=p2->tokencount) {
                        break;
                    }
                    else {
                        greaterorequ = false;
                        break;
                    }
                }
                else if(array_greater(cid1,cid2,sortnum)) {
                    greaterorequ = false;
                    break;
                }
            }
            if(p1 == NULL)
                greaterorequ = false;
            if(!greaterorequ)
                break;
            else {
                p2=p2->next;
            }
        }
        delete [] cid1;
        delete [] cid2;
    }
    return greaterorequ;
}

void Multiset::printToken() const
{
    Tokens *p = tokenQ->next;
    if(p==NULL)
    {
        cout<<"NULL"<<endl;
        return;
    }
    while(p)
    {
        cout<<p->tokencount<<"\'";
        if(tid == usersort)
        {
            COLORID cid;
            p->tokenvalue->getColor(cid);
            cout<<SortTable::usersort[sid].members[cid];
        }
        if(tid == finiteintrange)
        {
            COLORID cid;
            p->tokenvalue->getColor(cid);
            cout<<cid;
        }
        else if(tid == productsort)
        {
            int sortnum = SortTable::productsort[sid].sortnum;
            COLORID *cid = new COLORID[sortnum];
            p->tokenvalue->getColor(cid, sortnum);
            cout<<"[";
            for(int i=0;i<sortnum;++i)
            {
                type tid = SortTable::productsort[sid].sortid[i].tid;
                if(tid == finiteintrange) {
                    cout<<cid[i];
                }
                else if(tid == usersort) {
                    SORTID ssid = SortTable::productsort[sid].sortid[i].sid;
                    cout<<SortTable::usersort[ssid].members[cid[i]];
                }
            }
            cout<<"]";
            delete [] cid;
        }
        else if(tid == dot)
        {
            cout<<"dot";
        }
        p=p->next;
        cout<<'+';
    }
    cout<<endl;
}

void Multiset::printToken(string &str) const
{
    Tokens *p = tokenQ->next;
    if(p==NULL)
    {
        str += "NULL";
        return;
    }
    while(p)
    {
        str += to_string(p->tokencount) + "\'";
        if(tid == usersort)
        {
            COLORID cid;
            p->tokenvalue->getColor(cid);
            str += SortTable::usersort[sid].members[cid];
        }
        if(tid == finiteintrange)
        {
            COLORID cid;
            p->tokenvalue->getColor(cid);
            str += to_string(cid);
        }
        else if(tid == productsort)
        {
            int sortnum = SortTable::productsort[sid].sortnum;
            COLORID *cid = new COLORID[sortnum];
            p->tokenvalue->getColor(cid, sortnum);
            str += "[";
            for(int i=0;i<sortnum;++i)
            {
                MSI sortinfo = SortTable::productsort[sid].sortid[i];
                SORTID ssid = sortinfo.sid;
                if(sortinfo.tid == finiteintrange) {
                    str += to_string(cid[i])+",";
                }
                else if(sortinfo.tid == usersort) {
                    str += SortTable::usersort[ssid].members[cid[i]]+",";
                }
            }
            str += "]";
            delete [] cid;
        }
        else if(tid == dot)
        {
            str += "dot";
        }
        p=p->next;
        str += '+';
    }
}

NUM_t Multiset::Tokensum() const
{
    NUM_t sum = 0;
    Tokens *p = tokenQ->next;
    while(p!=NULL)
    {
        sum+=p->tokencount;
        p=p->next;
    }
    return sum;
}

index_t Multiset::Hash()
{
    index_t hv = 0;
    if(tid == usersort || tid == finiteintrange)
    {
        hv = colorcount*H1FACTOR*H1FACTOR*H1FACTOR;
        Tokens *p = tokenQ->next;
        COLORID i=3;
        for(i,p;p!=NULL;p=p->next,i=i*3)
        {
            COLORID cid;
            p->tokenvalue->getColor(cid);
            hv += p->tokencount*H1FACTOR*H1FACTOR+(cid+1)*i;
        }
    }
    else if(tid == productsort)
    {
        hv = colorcount*H1FACTOR*H1FACTOR*H1FACTOR;
        Tokens *p;
        int sortnum = SortTable::productsort[sid].sortnum;
        COLORID *cid = new COLORID[sortnum];
        COLORID i=3;
        for(i,p=tokenQ->next;p!=NULL;p=p->next,i=i*3)
        {
            p->tokenvalue->getColor(cid, sortnum);
            hv += p->tokencount*H1FACTOR*H1FACTOR;
            for(int j=0;j<sortnum;++j)
            {
                hv += (cid[j]+1)*i;
            }
        }
        delete [] cid;
    }
    else if(tid == dot)
    {
        hv = colorcount*H1FACTOR*H1FACTOR*H1FACTOR;
        Tokens *p = tokenQ->next;
        if(p!=NULL)
            hv += p->tokencount*H1FACTOR*H1FACTOR;
    }
    hashvalue = hv;
    return hv;
}

void Multiset::unify_coefficient(SHORTNUM coefficient) {
    Tokens *p = tokenQ->next;
    while (p) {
        p->tokencount = coefficient;
        p=p->next;
    }
}

Multiset::Multiset() {
    tokenQ=new Tokens;
    colorcount=0;
    hashvalue=0;
}

Multiset::~Multiset() {
    clear();
    delete tokenQ;
}

void Multiset::initiate(type t, SORTID s) {
    tid=t;sid=s;
}


/*************************************************************************\
|                             Global functions                            |
\*************************************************************************/
void Tokenscopy(Tokens &t1,const Tokens &t2,type tid,int PSnum)
{
    t1.initiate(t2.tokencount,tid,PSnum);
    if(tid == usersort)
    {
        COLORID cid;
        t2.tokenvalue->getColor(cid);
        t1.tokenvalue->setColor(cid);
    }
    else if(tid == productsort)
    {
        COLORID *cid = new COLORID[PSnum];
        t2.tokenvalue->getColor(cid,PSnum);
        t1.tokenvalue->setColor(cid,PSnum);
        delete [] cid;
    }
    else if(tid == finiteintrange)
    {
        COLORID cid;
        t2.tokenvalue->getColor(cid);
        t1.tokenvalue->setColor(cid);
    }
}

void MultisetCopy(Multiset &mm1, const Multiset &mm2, type tid, SORTID sid)
{
    mm1.colorcount = mm2.colorcount;
    Tokens *q = mm2.tokenQ->next;
    Tokens *p,*ppre = mm1.tokenQ;
    if(mm2.tid == productsort)
    {
        while(q)
        {
            p = new Tokens;
            int sortnum = SortTable::productsort[mm2.sid].sortnum;
            Tokenscopy(*p,*q,tid,sortnum);
            ppre->next = p;
            ppre = p;
            q=q->next;
        }
    }
    else
    {
        while(q)
        {
            p = new Tokens;
            Tokenscopy(*p,*q,tid);
            ppre->next = p;
            ppre = p;
            q=q->next;
        }
    }
}

int MINUS(Multiset &mm1, const Multiset &mm2)
{
    Tokens *pp1,*pp2,*pp1p;
    pp1 = mm1.tokenQ->next;
    pp1p = mm1.tokenQ;
    pp2 = mm2.tokenQ->next;
    if(mm1.tid == dot)
    {
        if(pp1->tokencount<pp2->tokencount)
            return 0;
        pp1->tokencount = pp1->tokencount-pp2->tokencount;
        if(pp1->tokencount == 0) {
            delete pp1;
            mm1.tokenQ->next=NULL;
            mm1.colorcount = 0;
        }
        return 1;
    }
    else if(mm1.tid == usersort || mm1.tid == finiteintrange)
    {
        COLORID cid1,cid2;
        while(pp2)
        {
            pp2->tokenvalue->getColor(cid2);
            while(pp1)
            {
                pp1->tokenvalue->getColor(cid1);
                if(cid1<cid2)
                {
                    pp1p = pp1;
                    pp1=pp1->next;
                    continue;
                }
                else if(cid1==cid2)
                {
                    if(pp1->tokencount < pp2->tokencount)
                        return 0;
                    pp1->tokencount = pp1->tokencount - pp2->tokencount;
                    if(pp1->tokencount == 0)
                    {
                        pp1p->next = pp1->next;
                        mm1.colorcount--;
                        delete pp1;

                        pp2=pp2->next;
                        pp1=pp1p->next;
                        break;
                    } else{
                        pp2=pp2->next;
                        pp1p = pp1;
                        pp1=pp1->next;
                        break;
                    }
                }
                else if(cid1 > cid2)
                {
                    return 0;
                }
            }
            if(pp1==NULL && pp2!=NULL)
            {
                return 0;
            }
        }
        return 1;
    }
    else if(mm1.tid == productsort)
    {
        int sortnum = SortTable::productsort[mm1.sid].sortnum;
        COLORID *cid1,*cid2;
        cid1 = new COLORID[sortnum];
        cid2 = new COLORID[sortnum];
        while(pp2)
        {
            pp2->tokenvalue->getColor(cid2,sortnum);
            while(pp1)
            {
                pp1->tokenvalue->getColor(cid1,sortnum);
                if(array_less(cid1,cid2,sortnum))
                {
                    pp1p = pp1;
                    pp1=pp1->next;
                    continue;
                }
                else if(array_equal(cid1,cid2,sortnum))
                {
                    if(pp1->tokencount<pp2->tokencount)
                        return 0;
                    pp1->tokencount = pp1->tokencount-pp2->tokencount;
                    if(pp1->tokencount == 0)
                    {
                        pp1p->next = pp1->next;
                        mm1.colorcount--;
                        delete pp1;

                        pp1=pp1p->next;
                        pp2=pp2->next;
                        break;
                    }
                    else
                    {
                        pp1p=pp1;
                        pp1=pp1->next;
                        pp2=pp2->next;
                        break;
                    }
                }
                else if(array_greater(cid1,cid2,sortnum))
                {
                    delete [] cid1;
                    delete [] cid2;
                    return 0;
                }
            }
            if(pp1==NULL && pp2!=NULL)
            {
                delete [] cid1;
                delete [] cid2;
                return 0;
            }
        }
        delete [] cid1;
        delete [] cid2;
        return 1;
    }
}

void PLUS(Multiset &mm1, const Multiset &mm2)
{
    Tokens *p = mm2.tokenQ->next;
    Tokens *resp;
    if(mm1.tid == productsort)
    {
        int sortnum = SortTable::productsort[mm1.sid].sortnum;
        while(p)
        {
            resp = new Tokens;
            Tokenscopy(*resp,*p,mm1.tid,sortnum);
            mm1.insert(resp);
            p=p->next;
        }
    } else{
        while(p)
        {
            resp = new Tokens;
            Tokenscopy(*resp,*p,mm1.tid);
            mm1.insert(resp);
            p=p->next;
        }
    }
}

int pattern_merge(COLORID *ret, const COLORID *array1, const COLORID *array2, int length) {
    memcpy(ret,array1,sizeof(COLORID)*length);
    for(int i=0;i<length;++i) {
        if(array2[i] != MAXCOLORID) {
            if(array1[i] == MAXCOLORID) {
                ret[i] = array2[i];
            }
            else if(array1[i] != array2[i]){
                return CONFLICT;
            }
        }
    }
    return SUCCESS;
}

int pattern_merge(COLORID *array1, const COLORID *array2, int length) {
    for(int i=0;i<length;++i) {
        if(array2[i]!=MAXCOLORID) {
            if(array1[i] == MAXCOLORID) {
                array1[i] = array2[i];
            }
            else if(array1[i] != array2[i]) {
                return CONFLICT;
            }
        }
    }
    return SUCCESS;
}