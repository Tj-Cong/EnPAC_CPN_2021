//
// Created by leo on 2022/2/16.
//
#include<string>
#include"CPN_Product.h"

using namespace std;

void extract_from_exp(STNode * n,atomictable &AT,vector<string> &criteria_p,vector<string> &criteria_t){
    string myformula = n->formula;
    if(myformula[0]== '!' ) {
        myformula = myformula.substr(1);
    }
    //查表
    for(int i=0;i<=AT.atomiccount;++i){
        if(myformula == AT.atomics[i].mystr){
            atomicmeta &aa=AT.atomics[i];
            if(aa.mytype==PT_CARDINALITY){
                //提取左表达式
                if(aa.leftexp.constnum==-1){
                    cardmeta* meta=aa.leftexp.expression;
                    while (meta){
                        if(!exist_in(criteria_p,cpn->place[meta->placeid].id)){
                            criteria_p.push_back(cpn->place[meta->placeid].id);
                        }
                        meta=meta->next;
                    }
                }
                //提取右表达式
                if(aa.rightexp.constnum==-1){
                    cardmeta* meta=aa.rightexp.expression;
                    while (meta){
                        if(!exist_in(criteria_p,cpn->place[meta->placeid].id)){
                            criteria_p.push_back(cpn->place[meta->placeid].id);
                        }
                        meta=meta->next;
                    }
                }
            } else{
                if(aa.fires.empty())
                    break;
                vector<index_t>::iterator it;
                it=aa.fires.begin();
                while(it!=aa.fires.end()){
                    if(!exist_in(criteria_t,cpn->transition[*it].id)){
                        criteria_t.push_back(cpn->transition[*it].id);
                    }
                    it++;
                }
            }
        }
    }
}
void extract_criteria(STNode * n,atomictable &AT,vector<string> &criteria_p,vector<string> &criteria_t,bool &next){
    if(n==NULL)
        return;
    switch (n->ntyp) {
        case ROOT:{
            extract_criteria(n->nleft,AT,criteria_p,criteria_t,next);
            break;
        }
        case PREDICATE:{
            extract_from_exp(n,AT,criteria_p,criteria_t);
            break;
        }
        case NEG:{
            cerr<<"The formula hasn't been transformed into Negation Normal Form!"<<endl;
            exit(-1);
        }
        case CONJUNC:{
            extract_criteria(n->nleft,AT,criteria_p,criteria_t,next);
            extract_criteria(n->nright,AT,criteria_p,criteria_t,next);
            break;
        }
        case DISJUNC:{
            extract_criteria(n->nleft,AT,criteria_p,criteria_t,next);
            extract_criteria(n->nright,AT,criteria_p,criteria_t,next);
            break;
        }
        case NEXT:{
            next= true;
            break;
        }
        case ALWAYS:{
            extract_criteria(n->nleft,AT,criteria_p,criteria_t,next);
            break;
        }
        case EVENTUALLY:{
            extract_criteria(n->nleft,AT,criteria_p,criteria_t,next);
            break;
        }
        case U_OPER:{
            extract_criteria(n->nleft,AT,criteria_p,criteria_t,next);
            extract_criteria(n->nright,AT,criteria_p,criteria_t,next);
            break;
        }
        case V_OPER:{
            extract_criteria(n->nleft,AT,criteria_p,criteria_t,next);
            extract_criteria(n->nright,AT,criteria_p,criteria_t,next);
            break;
        }
        default:cerr<<"Can not recognize operator: "<<n->ntyp<<endl; break;
    }
}