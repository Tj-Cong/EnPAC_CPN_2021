//
// Created by hecong on 2020/11/13.
//
#include "expression.h"
#include "CPN_RG.h"

bool colorflag;

int stringToNum(const string& str)
{
    istringstream iss(str);
    int num;
    iss >> num;
    return num;
}

condition_tree::condition_tree() {
    root = new CTN;
    root->mytype = nodetype::Root;
}

condition_tree::~condition_tree() {
    destructor(this->root);
}

void condition_tree::build_step(TiXmlElement *elem, CTN *&curnode) {
    string value = elem->Value();
    if(value == "subterm")
    {
        elem = elem->FirstChildElement();
        value = elem->Value();
    }

    if(value == "and" ||
       value == "or" ||
       value == "not" ||
       value == "imply")
    {
        curnode = new CTN;
        curnode->myname = value;
        curnode->mytype = nodetype::Boolean;
        TiXmlElement *leftchild,*rightchild;
        leftchild = elem->FirstChildElement();
        rightchild = leftchild->NextSiblingElement();
        build_step(leftchild,curnode->left);
        if(rightchild)
            build_step(rightchild,curnode->right);
    }
    else if(value == "equality" ||
            value == "inequality" ||
            value == "lessthan" ||
            value == "lessthanorequal" ||
            value == "greaterthan" ||
            value == "greaterthanorequal")
    {
        curnode = new CTN;
        curnode->myname = value;
        curnode->mytype = nodetype::Relation;
        TiXmlElement *leftchild,*rightchild;
        leftchild = elem->FirstChildElement();
        rightchild = leftchild->NextSiblingElement();
        build_step(leftchild,curnode->left);
        build_step(rightchild,curnode->right);
    }
    else if(value == "variable")
    {
        TiXmlAttribute *refvariable = elem->FirstAttribute();
        curnode = new CTN;
        curnode->myname = refvariable->Value();
        curnode->mytype = nodetype::variable;
    }
    else if(value == "useroperator")
    {
        TiXmlAttribute *declaration = elem->FirstAttribute();
        curnode = new CTN;
        curnode->myname = declaration->Value();
        curnode->mytype = nodetype::useroperator;
    }
    else if(value == "finiteintrangeconstant")
    {
        string value = elem->FirstAttribute()->Value();
        curnode = new CTN ;
        curnode->mytype = nodetype::finiteintrangeconstant;
        curnode->myname = value;
        curnode->cid = stringToNum(value);
    }
    else
    {
        cerr<<"Can not recognize operator \'"<<value<<"\' in Transition's guard function!"<<endl;
        exit(0);
    }
}

void condition_tree::constructor(TiXmlElement *condition) {
    string value = condition->Value();
    if(value == "condition")
    {
        TiXmlElement *structure = condition->FirstChildElement("structure");
        TiXmlElement *firstoperator = structure->FirstChildElement();
        build_step(firstoperator,root->left);
    }
    else
        cerr<<"Guard function's entrance is XmlElement \'condition\'"<<endl;
}

void condition_tree::destructor(CTN *node) {
    if(node->left!=NULL)
        destructor(node->left);
    if(node->right!=NULL)
        destructor(node->right);
    delete node;
}

void condition_tree::computeEXP(CTN *node) {
    switch(node->mytype)
    {
        case Boolean:{
            if(node->myname == "and")
            {
                computeEXP(node->left);
                if(node->right) {
                    computeEXP(node->right);
                    node->myexp = "("+node->left->myexp+")&&("+node->right->myexp+")";
                }
                else {
                    node->myexp = node->left->myexp;
                }
            }
            else if(node->myname == "or")
            {
                computeEXP(node->left);
                if(node->right) {
                    computeEXP(node->right);
                    node->myexp = "("+node->left->myexp+")||("+node->right->myexp+")";
                }
                else {
                    node->myexp = node->left->myexp;
                }
            }
            else if(node->myname == "imply")
            {
                computeEXP(node->left);
                computeEXP(node->right);
                node->myexp = "("+node->left->myexp+")->("+node->right->myexp+")";
            }
            else if(node->myname == "not")
            {
                computeEXP(node->left);
                node->myexp = "!("+node->left->myexp+")";
            }
            break;
        }
        case Relation:{
            computeEXP(node->left);
            computeEXP(node->right);
            if(node->myname == "equality")
            {
                node->myexp = "("+node->left->myexp+")==("+node->right->myexp+")";
            }
            else if(node->myname == "inequality")
            {
                node->myexp = "("+node->left->myexp+")!=("+node->right->myexp+")";
            }
            else if(node->myname == "lessthan")
            {
                node->myexp = "("+node->left->myexp+")<("+node->right->myexp+")";
            }
            else if(node->myname == "lessthanorequal")
            {
                node->myexp = "("+node->left->myexp+")<=("+node->right->myexp+")";
            }
            else if(node->myname == "greaterthan")
            {
                node->myexp = "("+node->left->myexp+")>("+node->right->myexp+")";
            }
            else if(node->myname == "greaterthanorequal")
            {
                node->myexp = "("+node->left->myexp+")>=("+node->right->myexp+")";
            }
            break;
        }
        case variable:{
            node->myexp = node->myname;
            break;
        }
        case useroperator: {
            node->myexp = node->myname;
            break;
        }
        case finiteintrangeconstant: {
            node->myexp = node->myname;
            break;
        }
    }
}

void condition_tree::printEXP(string &str) {
    computeEXP(root->left);
    str = root->left->myexp;
}

bool condition_tree::judgeGuard(const COLORID *vararray) {
    judgeGuard(root->left,vararray);
    return root->left->mytruth;
}

void condition_tree::judgeGuard(CTN *node,const COLORID *vararray) {
    switch(node->mytype)
    {
        case Boolean:{
            if(node->myname == "and")
            {
                judgeGuard(node->left,vararray);
                if(!node->left->mytruth)
                    node->mytruth = false;
                else if(node->right) {
                    judgeGuard(node->right,vararray);
                    node->mytruth = node->right->mytruth;
                }
                else {
                    node->mytruth = true;
                }
            }
            else if(node->myname == "or")
            {
                judgeGuard(node->left,vararray);
                if(node->left->mytruth)
                    node->mytruth = true;
                else if(node->right){
                    judgeGuard(node->right,vararray);
                    node->mytruth = node->right->mytruth;
                }
                else {
                    node->mytruth = false;
                }
            }
            else if(node->myname == "imply")
            {
                judgeGuard(node->left,vararray);
                judgeGuard(node->right,vararray);
                if(!node->left->mytruth)
                    node->mytruth = true;
                else if(node->left->mytruth && node->right->mytruth)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "not")
            {
                judgeGuard(node->left,vararray);
                if(node->left->mytruth)
                    node->mytruth = false;
                else
                    node->mytruth = true;
            }
            break;
        }
        case Relation:{
//            if(node->left->mytype!=variable && node->left->mytype!=useroperator)
//                cerr<<"[ERROR @ condition_tree::judgeGuard] Relation node's leftnode is not a variable or color."<<endl;
//            if(node->right->mytype!=variable && node->right->mytype!=useroperator)
//                cerr<<"[ERROR @ condition_tree::judgeGuard] Relation node's rightnode is not a variable or color."<<endl;
            judgeGuard(node->left,vararray);
            judgeGuard(node->right,vararray);
            if(node->myname == "equality")
            {
                if(node->left->cid == node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "inequality")
            {
                if(node->left->cid != node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "lessthan")
            {
                if(node->left->cid < node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "lessthanorequal")
            {
                if(node->left->cid <= node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "greaterthan")
            {
                if(node->left->cid > node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            else if(node->myname == "greaterthanorequal")
            {
                if(node->left->cid >= node->right->cid)
                    node->mytruth = true;
                else
                    node->mytruth = false;
            }
            break;
        }
        case variable:{
            map<string,VARID>::iterator viter;
            viter=VarTable::mapVariable.find(node->myname);
            if(vararray[viter->second] == MAXCOLORID)
            {
                cerr<<"[ERROR @ condition_tree::judgeGuard] Variable "<<node->myname<<" hasn't been assigned."<<endl;
                exit(-1);
            }
            node->cid = vararray[viter->second];
            break;
        }
        case useroperator: {
            map<string,mapcolor_info>::iterator citer;
            citer = SortTable::mapColor.find(node->myname);
            node->cid = citer->second.cid;
            break;
        }
        case finiteintrangeconstant: {
            break;
        }
        default:{
            cerr<<"[judgeGuard] Unrecognized node in condition tree"<<endl;
            exit(-1);
        }
    }
}

/*************************************************************************\
| *                                                                     * |
| *                            hlinscription                            * |
| *                                                                     * |
\*************************************************************************/
hlinscription::hlinscription() {
    root = new arcnode;
    Tokensum = 0;
    pure_multiset = true;
}

hlinscription::~hlinscription() {
//    if(root!=NULL) {
//        Tree_Destructor(root);
//        root = NULL;
//    }
}

void hlinscription::initiate(type tid, SORTID sid) {
    placetid = tid;
    placesid = sid;
}

void hlinscription::insertvar(VARID vid) {
    vector<VARID>::const_iterator iter;
    for(iter=arcrelvar.begin();iter!=arcrelvar.end();++iter) {
        if(*iter == vid)
            return;
    }
    arcrelvar.push_back(vid);
}

void hlinscription::set_tranid(SHORTIDX tid) {
    tranid = tid;
}

void hlinscription::Tree_Constructor(TiXmlElement *hlinscription) {
    string value = hlinscription->Value();
    if(value == "hlinscription") {
        TiXmlElement *structure = hlinscription->FirstChildElement("structure");
        TiXmlElement *firstoperator = structure->FirstChildElement();
        Tree_Constructor(firstoperator, root->leftnode);
    }
    else {
        cerr<<"hlinscription expression's entrance must be XmlElement \'hlinscription\'"<<endl;
    }

    TokenSum(root->leftnode);
    Tokensum = root->leftnode->tokencount;
}

void hlinscription::Tree_Constructor(TiXmlElement *xmlnode, arcnode *&curnode, int tuplenum) {
    string text = xmlnode->Value();
    if(text == "subterm") {
        xmlnode = xmlnode->FirstChildElement();
        text = xmlnode->Value();
    }

    if(text == "numberof") {
        curnode = new arcnode;
        curnode->mytype = numberof;
        TiXmlElement *subterm1 = xmlnode->FirstChildElement();
        TiXmlAttribute *numberconstant_value = subterm1->FirstChildElement("numberconstant")->FirstAttribute();
        curnode->tokencount = curnode->number = stringToNum(numberconstant_value->Value());
        Tree_Constructor(subterm1->NextSiblingElement(),curnode->leftnode,tuplenum);
    }
    else if(text == "tuple") {
        curnode = new arcnode;
        curnode->mytype = hlnodetype::tuple;
        TiXmlElement *tupleterm = xmlnode->FirstChildElement();
        Tree_Constructor(tupleterm,curnode->leftnode,0);
        tupleterm = tupleterm->NextSiblingElement();
        Tree_Constructor(tupleterm,curnode->rightnode,1);
        tupleterm = tupleterm->NextSiblingElement();
        tuplenum = 2;

        arcnode *present,*predecessor = curnode;
        while (tupleterm) {
            present = new arcnode;
            present->mytype = hlnodetype::tuple;
            present->leftnode = predecessor->rightnode;
            predecessor->rightnode = present;
            Tree_Constructor(tupleterm,present->rightnode,tuplenum);
            predecessor = present;
            tupleterm = tupleterm->NextSiblingElement();
            tuplenum++;
        }
    }
    else if(text == "add") {
        curnode = new arcnode;
        curnode->mytype = add;
        TiXmlElement *addterm = xmlnode->FirstChildElement();
        Tree_Constructor(addterm,curnode->leftnode,tuplenum);

        addterm = addterm->NextSiblingElement();
        if(addterm!=NULL) {
            Tree_Constructor(addterm,curnode->rightnode,tuplenum);
            addterm = addterm->NextSiblingElement();
        }

        arcnode *present,*predecessor=curnode;
        while(addterm) {
            present = new arcnode;
            present->mytype = add;
            present->leftnode = predecessor->rightnode;
            predecessor->rightnode = present;
            Tree_Constructor(addterm,present->rightnode,tuplenum);

            predecessor = present;
            addterm = addterm->NextSiblingElement();
        }
    }
    else if(text == "subtract") {
        curnode = new arcnode;
        curnode->mytype = subtract;
        TiXmlElement *subterm = xmlnode->FirstChildElement();
        Tree_Constructor(subterm,curnode->leftnode,tuplenum);

        subterm = subterm->NextSiblingElement();
        Tree_Constructor(subterm,curnode->rightnode,tuplenum);
        subterm = subterm->NextSiblingElement();

        arcnode *present,*predecessor = curnode;
        while(subterm) {
            present = new arcnode;
            present->mytype = add;
            present->leftnode = predecessor->rightnode;
            predecessor->rightnode = present;
            Tree_Constructor(subterm,present->rightnode,tuplenum);
            predecessor = present;
            subterm = subterm->NextSiblingElement();
        }
        pure_multiset = false;
    }
    else if(text == "successor") {
        curnode = new arcnode;
        curnode->mytype = successor;
        Tree_Constructor(xmlnode->FirstChildElement(),curnode->leftnode,tuplenum);
    }
    else if(text == "predecessor") {
        curnode = new arcnode;
        curnode->mytype = predecessor;
        Tree_Constructor(xmlnode->FirstChildElement(),curnode->leftnode,tuplenum);
    }
    else if(text == "all") {
        curnode = new arcnode;
        curnode->mytype = all;
        curnode->position = tuplenum;
        string sortname = xmlnode->FirstChildElement()->FirstAttribute()->Value();
        map<string,MSI>::iterator sortiter;
        sortiter = SortTable::mapSort.find(sortname);
        if(sortiter != SortTable::mapSort.end()) {
            curnode->tid = sortiter->second.tid;
            curnode->sid = sortiter->second.sid;
        }
        else {
            cerr<<"[ERROR @ hlinscription::Tree_Constructor] Can not recognize sort \'"<<sortname<<"\'."<<endl;
            exit(-1);
        }

        if(curnode->tid == finiteintrange) {
            curnode->tokencount = SortTable::finitintrange[curnode->sid].end - SortTable::finitintrange[curnode->sid].start + 1;
        }
        else if(curnode->tid == usersort) {
            curnode->tokencount = SortTable::usersort[curnode->sid].feconstnum;
        }
    }
    else if(text == "variable") {
        curnode = new arcnode;
        curnode->mytype = var;
        curnode->position = tuplenum;
        string varname = xmlnode->FirstAttribute()->Value();
        map<string,VARID>::iterator variter;
        variter = VarTable::mapVariable.find(varname);
        if(variter!=VarTable::mapVariable.end()) {
            curnode->number = variter->second;
            insertvar(variter->second);
            curnode->tid = VarTable::vartable[curnode->number].tid;
            curnode->sid = VarTable::vartable[curnode->number].sid;
        }
        else {
            cerr<<"[ERROR @ hlinscription::Tree_Constructor] Can not find refvariable \""<<varname<<"\"."<<endl;
            exit(-1);
        }
    }
    else if(text == "useroperator") {
        curnode = new arcnode;
        curnode->mytype = color;
        curnode->position = tuplenum;
        string colorname = xmlnode->FirstAttribute()->Value();
        map<string,MCI>::iterator coloriter;
        coloriter = SortTable::mapColor.find(colorname);
        if(coloriter!=SortTable::mapColor.end()) {
            curnode->number = coloriter->second.cid;
            curnode->tid = usersort;
            curnode->sid = coloriter->second.sid;
        }
        else {
            cerr<<"[ERROR @ hlinscription::Tree_Constructor] Can not find useroperator \""<<colorname<<"\"."<<endl;
            exit(-1);
        }
    }
    else if(text == "finiteintrangeconstant") {
        curnode = new arcnode;
        curnode->mytype = color;
        curnode->tid = finiteintrange;
        curnode->position = tuplenum;
        curnode->number = stringToNum(xmlnode->FirstAttribute()->Value());

        TiXmlAttribute *attr = xmlnode->FirstChildElement()->FirstAttribute();
        int start = stringToNum(attr->Value());
        int end = stringToNum(attr->Next()->Value());

        SORTID i;
        for(i=0;i<SortTable::finitintrange.size();++i) {
            if(start==SortTable::finitintrange[i].start && end==SortTable::finitintrange[i].end) {
                curnode->sid=i;
            }
        }
        if(i>=SortTable::finitintrange.size()) {
            cerr<<"[ERROR @ hlinscription::Tree_Constructor] Can not find a finiteintrange sort from "<<start<<" to "<<end<<endl;
            exit(-1);
        }
    }
    else if(text == "dotconstant") {
        curnode = new arcnode;
        curnode->mytype = color;
        curnode->tid = dot;
        curnode->position = tuplenum;
    }
    else {
        cerr<<"[ERROR @ hlinscription::Tree_Constructor] Can not recognize operator \""<<text<<"\""<<endl;
        exit(0);
    }
}

void hlinscription::Tree_Destructor(arcnode *node) {
    if(node->leftnode)
        Tree_Destructor(node->leftnode);
    if(node->rightnode)
        Tree_Destructor(node->rightnode);
    delete node;
}

/*calculate the multiset of this arc arccording to @parm binding
 * @parm ms (out): the multiset of arc
 * @parm binding (in): a binding (an variable array whose length is he number of ALL variables)*/
void hlinscription::to_Multiset(Multiset &ms, const COLORID *binding) {
    colorflag = true;
    ms.initiate(placetid,placesid);
    to_Multiset(root->leftnode,ms,binding);
}

/*This is a recursive function: get the product color according to a binding @parm vararray
 * @parm node (in): current node (initially, it is the top tuple node)
 * @parm colarray (out): the calculated product color (an array whose length is the sortnum of this product sort)
 * @parm vararay (in): a binding
 * Types of nodes contained in the tuple subtree：
 * {tuple,predecessor,successor,var,color}*/
void hlinscription::to_Multiset(arcnode *node,Multiset &ms,const COLORID *Binding,SHORTNUM tokennum) {
    if(!colorflag)
        return;

    switch (node->mytype)
    {
        /*create a token and insert it into ms;
         * this token's tokenvalue is its leftnode's color--*/
        case hlnodetype::predecessor: {
            Tokens *token = new Tokens;
            /*anomaly detection*/
            if(node->leftnode->tid != usersort || placetid!=usersort) {
                cerr<<"[ERROR @ hlinscription::to_Multiset] predecessor error."<<endl;
                exit(-1);
            }
            token->initiate(tokennum,placetid);
            COLORID cid = MAXCOLORID;

            /*fetch original color*/
            if(node->leftnode->mytype == var) {
                cid = Binding[node->leftnode->number];
            }
            else if(node->leftnode->mytype == color) {
                cid = node->leftnode->number;
            }

            /*anomaly detection*/
            if(cid == MAXCOLORID) {
                cerr<<"[ERROR @ hlinscription::to_Multiset] MAXCOLORID error."<<endl;
                exit(-1);
            }

            /*calculate new color*/
            if(cid == 0) {
                cid = SortTable::usersort[node->leftnode->sid].feconstnum-1;
            }
            else {
                cid = cid-1;
            }
            token->tokenvalue->setColor(cid);
            ms.insert(token);
            break;
        }

        /*create a token and insert it into ms;
        * this token's tokenvalue is its leftnode's color++*/
        case hlnodetype::successor: {
            Tokens *token = new Tokens;
            /*anomaly detection*/
            if(node->leftnode->tid!=usersort || placetid!=usersort) {
                cerr<<"[ERROR @ hlinscription::to_Multiset] predecessor error."<<endl;
                exit(-1);
            }
            token->initiate(tokennum,placetid);
            COLORID cid = MAXCOLORID;

            /*fetch original color*/
            if(node->leftnode->mytype == var) {
                cid = Binding[node->leftnode->number];
            }
            else if(node->leftnode->mytype == color) {
                cid = node->leftnode->number;
            }

            /*anomaly detection*/
            if(cid == MAXCOLORID) {
                cerr<<"[ERROR @ hlinscription::to_Multiset] MAXCOLORID error."<<endl;
                exit(-1);
            }

            cid = (cid+1) % SortTable::usersort[node->leftnode->sid].feconstnum;
            token->tokenvalue->setColor(cid);
            ms.insert(token);
            break;
        }

        /*get leftnode multiset and rightnode multiset*/
        case hlnodetype::add: {
            to_Multiset(node->leftnode,ms,Binding,tokennum);
            if(node->rightnode)
                to_Multiset(node->rightnode,ms,Binding,tokennum);
            break;
        }

        /*leftnode multiset - rightnode multiset*/
        case hlnodetype::subtract: {
            Multiset l_tempms,r_tempms;
            l_tempms.initiate(placetid,placesid);
            r_tempms.initiate(placetid,placesid);
            to_Multiset(node->leftnode,l_tempms,Binding,tokennum);
            to_Multiset(node->rightnode,r_tempms,Binding,tokennum);
            if(MINUS(l_tempms,r_tempms) == 0) {
                colorflag = false;
                return;
            }
            else {
                MultisetCopy(ms,l_tempms,placetid,placesid);
            }
            break;
        }
        case hlnodetype::tuple: {
            int psnum = SortTable::productsort[placesid].sortnum;
            BindingList bindingList;
            bindingList.strategy = byhead;
            binding *initbind = new binding(tranid);
            bindingList.insert(initbind);

            getTupleColor(node,bindingList,Binding);

            binding *p = bindingList.listhead;
            while (p) {
                /*anomaly detection*/
                for(int i=0;i<psnum;++i) {
                    if(p->vararray[i]==MAXCOLORID) {
                        cerr<<"[ERROR @ hlinscription::to_Multiset] MAXCOLORID error."<<endl;
                        exit(-1);
                    }
                }

                Tokens *token = new Tokens;
                token->initiate(tokennum,placetid,psnum);
                token->tokenvalue->setColor(p->vararray,psnum);
                ms.insert(token);

                p=p->next;
            }

            break;
        }
        case hlnodetype::numberof: {
            to_Multiset(node->leftnode,ms,Binding,node->number);
            break;
        }
        case hlnodetype::all: {
            Tokens *token;
            if(node->tid == usersort) {
                int feconstnum = SortTable::usersort[node->sid].feconstnum;
                for(int i=0; i<feconstnum; ++i) {
                    token = new Tokens;
                    token->initiate(tokennum,placetid);
                    token->tokenvalue->setColor(i);
                    ms.insert(token);
                }
            }
            else if(node->tid == finiteintrange) {
                int start = SortTable::finitintrange[node->sid].start;
                int end = SortTable::finitintrange[node->sid].end;
                for(int i=start; i<=end; ++i) {
                    token = new Tokens;
                    token->initiate(tokennum,placetid);
                    token->tokenvalue->setColor(i);
                    ms.insert(token);
                }
            }
            break;
        }

        /*create a token and insert into ms*/
        case hlnodetype::color: {
            Tokens *token = new Tokens;
            token->initiate(tokennum,placetid);
            if(placetid != dot)
                token->tokenvalue->setColor(node->number);
            ms.insert(token);
            break;
        }

        /*create a token and insert into ms*/
        case hlnodetype::var: {
            Tokens *token = new Tokens;
            token->initiate(tokennum,placetid);

            /*anomaly detection*/
            if(Binding[node->number] == MAXCOLORID) {
                cerr<<"[ERROR @ hlinscription::to_Multiset] MAXCOLORID error."<<endl;
                exit(-1);
            }

            token->tokenvalue->setColor(Binding[node->number]);
            ms.insert(token);
            break;
        }
        default: {
            cerr<<"[ERROR @ hlinscription::to_Multiset] Unexpected nodetype."<<endl;
            exit(-1);
        }
    }
}

/*This is a recursive function: get the product color according to a binding @parm vararray
 * @parm node (in): current node (initially, it is the top tuple node)
 * @parm colarray (out): the calculated product color (an array whose length is the sortnum of this product sort)
 * @parm vararay (in): a binding
 * Types of nodes contained in the tuple subtree：
 * {tuple,predecessor,successor,var,color}*/
void hlinscription::getTupleColor(arcnode *node, BindingList &bindingList, const COLORID *vararray) {
    if(node->mytype == predecessor) {
        COLORID cid;
        /*fetch original color*/
        if(node->leftnode->mytype == var) {
            cid = vararray[node->leftnode->number];
        }
        else if(node->leftnode->mytype == color) {
            cid = node->leftnode->number;
        }

        /*claculate new color*/
        if(cid == 0) {
            cid = SortTable::usersort[node->leftnode->sid].feconstnum-1;
        }
        else {
            cid = cid-1;
        }
        binding *p = bindingList.listhead;
        while (p) {
            p->vararray[node->leftnode->position] = cid;
            p=p->next;
        }
    }
    else if(node->mytype == successor) {
        COLORID cid;
        /*fetch original color*/
        if(node->leftnode->mytype == var) {
            cid = vararray[node->leftnode->number];
        }
        else if(node->leftnode->mytype == color) {
            cid = node->leftnode->number;
        }
        /*calculate new color*/
        cid = (cid+1) % SortTable::usersort[node->leftnode->sid].feconstnum;

        binding *p = bindingList.listhead;
        while(p) {
            p->vararray[node->leftnode->position] = cid;
            p=p->next;
        }
    }
    else if(node->mytype == all) {
        if(node->tid == usersort) {
            int feconstnum = SortTable::usersort[node->sid].feconstnum;
            binding *p = bindingList.listhead;
            while (p) {
                p->vararray[node->position] = 0;
                for(int i=1; i<feconstnum; ++i) {
                    binding *q = new binding(*p);
                    q->vararray[node->position] = i;
                    bindingList.insert(q);
                }
                p=p->next;
            }
        }
        else if(node->tid == finiteintrange) {
            int start = SortTable::finitintrange[node->sid].start;
            int end = SortTable::finitintrange[node->sid].end;
            binding *p = bindingList.listhead;
            while (p) {
                p->vararray[node->position] = start;
                for(int i=start+1;i<=end;++i) {
                    binding *q = new binding(*p);
                    q->vararray[node->position] = i;
                    bindingList.insert(q);
                }
                p=p->next;
            }
        }
        else {
            cerr<<"[ERROR @ hlinscription::to_Multiset] Operator \"all\"'s childnode can not be outside of {usersort,finiteintrange}."<<endl;
            exit(-1);
        }
    }
    else if(node->mytype == hlnodetype::tuple) {
        getTupleColor(node->leftnode,bindingList,vararray);
        getTupleColor(node->rightnode,bindingList,vararray);
    }
    else if(node->mytype == color) {
        if(node->tid == dot) {
            cerr<<"[ERROR @ hlinscription::getTupleColor] dot sort is not supposed to be a member of a product sort."<<endl;
            exit(-1);
        }

        binding *p = bindingList.listhead;
        while (p) {
            p->vararray[node->position] = node->number;
            p=p->next;
        }
    }
    else if(node->mytype == var) {
        binding *p = bindingList.listhead;
        while (p) {
            p->vararray[node->position] = vararray[node->number];
            p=p->next;
        }
    }
    else {
        cerr<<"[ERROR @ hlinscription::getTupleColor] Unexcepted nodetype."<<endl;
        exit(-1);
    }
}

/*this is a recursive function: calculate myexp*/
void hlinscription::computeEXP(arcnode *node) {
    switch (node->mytype) {
        case hlnodetype::predecessor: {
            computeEXP(node->leftnode);
            node->myexp = node->leftnode->myexp+"--";
            break;
        }
        case hlnodetype::successor: {
            computeEXP(node->leftnode);
            node->myexp = node->leftnode->myexp+"++";
            break;
        }
        case hlnodetype::all: {
            node->myexp = node->tid==usersort?SortTable::usersort[node->sid].id:SortTable::finitintrange[node->sid].id+".all";
            break;
        }
        case hlnodetype::add: {
            computeEXP(node->leftnode);
            if(node->rightnode!=NULL) {
                computeEXP(node->rightnode);
                node->myexp = node->leftnode->myexp + "+" + node->rightnode->myexp;
            }
            else {
                node->myexp=node->leftnode->myexp;
            }
            break;
        }
        case hlnodetype::subtract: {
            computeEXP(node->leftnode);
            computeEXP(node->rightnode);
            node->myexp = node->leftnode->myexp + "-" + node->rightnode->myexp;
            break;
        }
        case hlnodetype::tuple: {
            computeEXP(node->leftnode);
            computeEXP(node->rightnode);
            if(node->leftnode->position==0) {
                node->myexp = "[";
            }
            node->myexp += node->leftnode->myexp + "," + node->rightnode->myexp;
            if(node->rightnode->mytype!=hlnodetype::tuple) {
                node->myexp += "]";
            }
            break;
        }
        case hlnodetype::numberof: {
            computeEXP(node->leftnode);
            node->myexp = to_string(node->number)+"\'"+node->leftnode->myexp;
            break;
        }
        case hlnodetype::color: {
            if(node->tid == finiteintrange) {
                node->myexp = to_string(node->number);
            }
            else if(node->tid == usersort) {
                node->myexp = SortTable::usersort[node->sid].members[node->number];
            }
            else if(node->tid == dot) {
                node->myexp = "dot";
            }
            break;
        }
        case hlnodetype::var: {
            node->myexp = VarTable::vartable[node->number].id;
            break;
        }
        default: {

        }
    }
}

/*print the arc expression into string @parm arcexp*/
void hlinscription::printEXP(string &arcexp) {
    computeEXP(root->leftnode);
    arcexp = root->leftnode->myexp;
}

/*destroy the whole tree*/
void hlinscription::Tree_Destructor() {
    if(root!=NULL) {
        Tree_Destructor(root);
        root = NULL;
    }
}

/*get all possible enabled incomplete bindings of this arc under a state @parm marking
 * all the incomplete bindings will be stored in IBS.
 * if this arc is a pure multiset arc (doesn't contain subtract operator),
 * then it will adopt multiset match strategy,
 * otherwise it implements variable enumeration strategy.
 * */
int hlinscription::get_IBS(const Multiset &marking,const BindingList &givenIBS,BindingList &retIBS) {
    if(pure_multiset)
        return multiset_match(marking,givenIBS,retIBS);
    else
        return variable_enumeration(marking,givenIBS,retIBS);
}

/*match the arc multiset with a given multiset to determine variables' value,
 * All possible values of variables will be recorded in a IBS in the form of
 * incomplete bindings.
 * @parm  marking (in): the given multiset
 * @parm IBS (out): incomplete binding set
 *
 * Function call relationship:
 * multiset_match
 *        |
 * arcmeta_multiset_match(recursive function)
 *        |
 * arcmeta_pattern_match
 *        |
 * tuplemeta_pattern_match*/
int hlinscription::multiset_match(const Multiset &marking, BindingList &IBS)
{
    /*1.pre-check: if Cardinality(marking) >= Cardinality(arc)*/
    if(marking.Tokensum() < this->Tokensum) {
        return FAIL;
    }

    if(metaentrance.empty()) {
        /*2.calculate metaentrance*/
        arcnode *p = root->leftnode;
        if(p->mytype != add) {
            metaentrance.push_back(p);
        }
        else {
            while (p && p->mytype == add) {
                metaentrance.push_back(p->leftnode);
                p=p->rightnode;
            }
            if(p)
                metaentrance.push_back(p);
        }
    }

    /*3.match begins*/
    COLORID *vararry = new COLORID [cpn->varcount];
    for(int i=0;i<cpn->varcount;++i) {
        vararry[i] = MAXCOLORID;
    }
//    memset(vararry,MAXCOLORID,sizeof(COLORID)*cpn->varcount);
    arcmeta_multiset_match(0,IBS,vararry,marking);
    delete [] vararry;

    if(IBS.empty())
        return FAIL;
    else
        return SUCCESS;
}
int hlinscription::multiset_match(const Multiset &marking, const BindingList &givenIBS, BindingList &retIBS) {
    /*1.pre-check: if Cardinality(marking) >= Cardinality(arc)*/
    if(marking.Tokensum() < this->Tokensum) {
        return FAIL;
    }

    if(metaentrance.empty()) {
        /*2.calculate metaentrance*/
        arcnode *p = root->leftnode;
        if(p->mytype != add) {
            metaentrance.push_back(p);
        }
        else {
            while (p && p->mytype == add) {
                metaentrance.push_back(p->leftnode);
                p=p->rightnode;
            }
            if(p)
                metaentrance.push_back(p);
        }
    }

    /*3.match begins*/
    binding *p = givenIBS.listhead;
    while (p) {
        arcmeta_multiset_match(0,retIBS,p->vararray,marking);
        if(ready2exit)
            return FAIL;
        p=p->next;
    }

    if(retIBS.empty())
        return FAIL;
    else
        return SUCCESS;
}

/*This is a recursive function only called by multiset_match
 * match an arc pattern with all the patterns in marking to get all possible enabled
 * incomplete bindings
 * @parm arcmeta (in): the (arcmeta)th member of metaentrance
 * @parm marking (in): the CPN state
 * @parm IBS (out): the set storing all possible enabled partial bindings
 * @parm vararray (in & out): the variable array which can be regarded as an incomplete binding*/
void hlinscription::arcmeta_multiset_match(int arcmetanum,BindingList &IBS,const COLORID *vararray,const Multiset &marking)
{
    if(arcmetanum >= metaentrance.size()) {
        Multiset tempms;
        to_Multiset(tempms,vararray);

        if(!colorflag)
            return;

        if(marking>=tempms) {
            IBS.copy_insert(tranid,vararray);
        }
        return;
    }

    arcnode *arcmeta = metaentrance[arcmetanum];
    Tokens *p=marking.tokenQ->next;
    COLORID *temp = new COLORID [cpn->varcount];
    while (p) {
        memcpy(temp,vararray,sizeof(COLORID)*cpn->varcount);
        if(arcmeta_pattern_match(arcmeta,p,temp)==SUCCESS) {
            arcmeta_multiset_match(arcmetanum+1,IBS,temp,marking);
        }
        if(ready2exit)
            return;
        p=p->next;
    }
    delete [] temp;
}

/*A pattern is meta of a multiset.
 * It is composed of a coefficient $k and a color $c or a variable $v.
 * i.e., if a multiset is 1'1 ++ 1'2 ++ 2'(a++), then (1'1),(1'2) and (2'(a++)) are all patterns
 * This function is to match an arc pattern with an given pattern @parm pattern,
 * its goal is to determine the related variables' value involved in the arc pattern.
 * The variables' value will be recorded in the array @parm vararray.
 * @parm curmeta (in): arc parttern (represented by an arc tree)
 * @parm pattern (in): the given pattern
 * @parm vararry (out): the variable array which can be regarded as an incomplete binding
 * return: FAIL or SUCCESS
 * */
int hlinscription::arcmeta_pattern_match(arcnode *curmeta, Tokens *pattern, COLORID *vararry)
{
    if(curmeta->mytype == numberof) {
        /*pre-check*/
        if(pattern->tokencount < curmeta->tokencount && !contain_All_operator(curmeta))
            return FAIL;

        /*recurse down*/
        return arcmeta_pattern_match(curmeta->leftnode,pattern,vararry);
    }
    else if(curmeta->mytype == hlnodetype::tuple) {
        return tuplemeta_pattern_match(curmeta,pattern,vararry);
    }
    else if(curmeta->mytype == var) {
        COLORID cid;
        pattern->tokenvalue->getColor(cid);
        if(vararry[curmeta->number]!=MAXCOLORID && vararry[curmeta->number]!=cid) {
            return CONFLICT;
        }
        vararry[curmeta->number] = cid;
        return SUCCESS;
    }
    else if(curmeta->mytype == color) {
        COLORID cid;
        if(curmeta->tid == dot)
            return SUCCESS;
        pattern->tokenvalue->getColor(cid);
        if(cid == curmeta->number)
            return SUCCESS;
        else
            return FAIL;
    }
    else if(curmeta->mytype == successor) {
        COLORID cid;
        pattern->tokenvalue->getColor(cid);
        if(curmeta->leftnode->mytype == var){
            if(cid == 0) {
                cid = SortTable::usersort[curmeta->leftnode->sid].feconstnum-1;
            }
            else {
                cid = cid-1;
            }

            int offset = curmeta->leftnode->number;
            if(vararry[offset]!=MAXCOLORID && vararry[offset]!=cid) {
                return CONFLICT;
            }
            vararry[offset] = cid;
            return SUCCESS;
        }
        else if(curmeta->leftnode->mytype == color) {
            COLORID retcolor = (curmeta->leftnode->number+1)%SortTable::usersort[curmeta->leftnode->sid].feconstnum;
            if(retcolor != cid)
                return FAIL;
            else
                return SUCCESS;
        }
    }
    else if(curmeta->mytype == predecessor) {
        COLORID cid;
        pattern->tokenvalue->getColor(cid);
        if(curmeta->leftnode->mytype == var) {
            cid = (cid+1)%SortTable::usersort[curmeta->leftnode->sid].feconstnum;

            int offset = curmeta->leftnode->number;
            if(vararry[offset]!=MAXCOLORID && vararry[offset]!=cid) {
                return CONFLICT;
            }
            vararry[offset] = cid;
            return SUCCESS;
        }
        else if(curmeta->leftnode->mytype == color) {
            cid = (cid+1)%SortTable::usersort[curmeta->leftnode->sid].feconstnum;
            if(cid == curmeta->leftnode->number)
                return SUCCESS;
            else
                return FAIL;
        }
    }
    else if(curmeta->mytype == all) {
        return SUCCESS;
    }
    else {
        cerr<<"[ERROR @ hlinscription::meta_match] unexpected operator "<<curmeta->mytype<<endl;
        exit(-1);
    }
}

/*match a product sort arc pattern with a given pattern
 * and record involved variable's value in vararray.
 * @parm curmeta (in): product sort arc pattern
 * @parm token (in): given pattern
 * @parm vararry(out): the variable array which can be regarded as an incomplete binding
 * return: FAIL or SUCCESS
 * */
int hlinscription::tuplemeta_pattern_match(arcnode *curmeta, Tokens *token, COLORID *vararry)
{
    int psnum = SortTable::productsort[placesid].sortnum;
    COLORID *cid = new COLORID [psnum];
    token->tokenvalue->getColor(cid,psnum);
    arcnode *tuplenode = curmeta;
    int i=0;
    bool last_left_finished=false,last_right_finished=false;
    while (tuplenode) {
        arcnode *leaf;
        if(tuplenode->rightnode->mytype != hlnodetype::tuple) {
            if(!last_left_finished) {
                leaf = tuplenode->leftnode;
                last_left_finished = true;
            }
            else {
                leaf = tuplenode->rightnode;
                last_right_finished = true;
            }
        }
        else {
            leaf = tuplenode->leftnode;
        }

        /*match begins*/
        if(leaf->mytype == predecessor) {
            if(leaf->leftnode->mytype == var) {
                COLORID varcolor = cid[i];
                varcolor = (varcolor+1)%SortTable::usersort[leaf->leftnode->sid].feconstnum;
                int offset = leaf->leftnode->number;
                if(vararry[offset]!=MAXCOLORID && vararry[offset]!=varcolor) {
                    /*variable value conflicts*/
                    delete [] cid;
                    return CONFLICT;
                }
                vararry[offset] = varcolor;
            }
            else if(leaf->leftnode->mytype == color) {
                COLORID varcolor = cid[i];
                varcolor = (varcolor+1)%SortTable::usersort[leaf->leftnode->sid].feconstnum;
                if(varcolor != leaf->leftnode->number) {
                    delete [] cid;
                    return FAIL;
                }
            }
        }
        else if(leaf->mytype == successor) {
            if(leaf->leftnode->mytype == var) {
                COLORID varcolor = cid[i];
                if(varcolor == 0) {
                    varcolor = SortTable::usersort[leaf->leftnode->sid].feconstnum-1;
                }
                else {
                    varcolor = varcolor-1;
                }

                int offset = leaf->leftnode->number;
                if(vararry[offset]!=MAXCOLORID && vararry[offset]!=varcolor) {
                    /*variable value conflicts*/
                    delete [] cid;
                    return CONFLICT;
                }
                vararry[offset] = varcolor;
            }
            else if(leaf->leftnode->mytype == color) {
                COLORID color = leaf->leftnode->number;
                color = (color+1)%SortTable::usersort[leaf->leftnode->sid].feconstnum;
                if(color!=cid[i]) {
                    delete [] cid;
                    return FAIL;
                }
            }
        }
        else if(leaf->mytype == var) {
            if(vararry[leaf->number]!=MAXCOLORID && vararry[leaf->number]!=cid[i]) {
                /*variable value conflicts*/
                delete [] cid;
                return CONFLICT;
            }
            vararry[leaf->number] = cid[i];
        }
        else if(leaf->mytype == color) {
            if(leaf->tid == dot){}
            else if(cid[i] != leaf->number) {
                delete [] cid;
                return FAIL;
            }
        }
        else if(leaf->mytype == all) {}
        else {
            cerr<<"[ERROR @ hlinscription::meta_tuple_match] Unexpected nodetype "<<leaf->mytype<<endl;
            exit(-1);
        }
        /*match ends*/

        if(tuplenode->rightnode->mytype == hlnodetype::tuple) {
            tuplenode = tuplenode->rightnode;
        }
        else if(last_right_finished) {
            break;
        }

        ++i;
    }

    delete [] cid;
    return SUCCESS;
}

/*calculate the number of tokens this arc expression contains*/
void hlinscription::TokenSum(arcnode *node) {
    if(node->mytype == hlnodetype::add) {
        TokenSum(node->leftnode);
        if(node->rightnode) {
            TokenSum(node->rightnode);
            node->tokencount = node->leftnode->tokencount + node->rightnode->tokencount;
        }
        else {
            node->tokencount = node->leftnode->tokencount;
        }
    }
    else if(node->mytype == hlnodetype::subtract) {
        TokenSum(node->leftnode);
        TokenSum(node->rightnode);
        node->tokencount = node->leftnode->tokencount - node->rightnode->tokencount;
    }
    else if(node->mytype == hlnodetype::predecessor) {
        TokenSum(node->leftnode);
        node->tokencount = node->leftnode->tokencount;
    }
    else if(node->mytype == hlnodetype::successor) {
        TokenSum(node->leftnode);
        node->tokencount = node->leftnode->tokencount;
    }
    else if(node->mytype == hlnodetype::tuple) {
        TokenSum(node->leftnode);
        TokenSum(node->rightnode);
        node->tokencount = node->leftnode->tokencount > node->rightnode->tokencount ?
                            node->leftnode->tokencount : node->rightnode->tokencount;
    }
    else if(node->mytype == hlnodetype::numberof) {
        TokenSum(node->leftnode);
        node->tokencount = node->number * node->leftnode->tokencount;
    }
}

/*get all possible enabled incomplete bindings under state @parm marking
 * by color space exploration strategy
 * @parm marking: the CPN state
 * @parm IBS: the set storing all possible enabled partial bindings
 * */
int hlinscription::variable_enumeration(const Multiset &marking, const BindingList &givenIBS, BindingList &retIBS) {

    binding *p=givenIBS.listhead;
    COLORID *inbinding = new COLORID [cpn->varcount];
    while (p) {
        memcpy(inbinding,p->vararray,sizeof(COLORID)*cpn->varcount);
        color_space_exploration(0,inbinding,marking,retIBS);
        p=p->next;
    }
    delete [] inbinding;

    if(retIBS.empty())
        return FAIL;
    else
        return SUCCESS;
}

/*this is recursive function: explore every variable's color space to get all
 * possible enabled partial bindings under a state @parm marking
 * @parm relvarpt: the (relvarpt)th member of arcrelvar
 * @parm vararray: current incomplete binding
 * @parm marking: a CPN state
 * @parm IBS: the set storing all possible enabled partial bindings*/
void hlinscription::color_space_exploration(int relvarpt, COLORID *vararray, const Multiset &marking, BindingList &IBS) {
    if(relvarpt >= arcrelvar.size()) {
        Multiset arcms;
        to_Multiset(arcms,vararray);

        if(!colorflag)
            return;

        if(marking >= arcms) {
            IBS.copy_insert(tranid,vararray);
        }
        return;
    }

    Variable &curvar = VarTable::vartable[arcrelvar[relvarpt]];

    if(vararray[curvar.vid]!=MAXCOLORID)
        color_space_exploration(relvarpt+1,vararray,marking,IBS);
    else {
        if(curvar.tid == productsort) {
            int start = SortTable::finitintrange[curvar.sid].start;
            int end = SortTable::finitintrange[curvar.sid].end;
            for(int i=start;i<=end;++i) {
                vararray[curvar.vid] = i;
                color_space_exploration(relvarpt+1,vararray,marking,IBS);
            }
        }
        else if(curvar.tid == usersort) {
            SHORTNUM feconstnum = SortTable::usersort[curvar.sid].feconstnum;
            for(SHORTIDX i=0;i<feconstnum;++i) {
                vararray[curvar.vid] = i;
                color_space_exploration(relvarpt+1,vararray,marking,IBS);
            }
        }
    }
}

bool hlinscription::contain_All_operator(arcnode *curmeta) {
    if(curmeta->mytype == hlnodetype::all) {
        return true;
    }

    bool l_contain = false, r_contain = false;
    if(curmeta->leftnode)
        l_contain = contain_All_operator(curmeta->leftnode);
    if(curmeta->rightnode)
        r_contain = contain_All_operator(curmeta->rightnode);

    return (l_contain || r_contain);
}


/*************************************************************************\
| *                                                                     * |
| *                          hlinitialmarking                           * |
| *                                                                     * |
\*************************************************************************/
hlinitialmarking::hlinitialmarking(type tid,SORTID sid) {
    root = new initMarking_node;
    placetid = tid;
    placesid = sid;
}

hlinitialmarking::~hlinitialmarking() {
    Tree_Destructor(root);
}

void hlinitialmarking::Tree_Constructor(TiXmlElement *hlinitialMarking) {
    string value = hlinitialMarking->Value();
    if(value == "hlinitialMarking") {
        TiXmlElement *structure = hlinitialMarking->FirstChildElement("structure");
        TiXmlElement *firstoperator = structure->FirstChildElement();
        Tree_Constructor(firstoperator, root->leftnode);
    }
    else
        cerr<<"hlinitialMarking expression's entrance must be XmlElement \'hlinitialMarking\'"<<endl;
}

void hlinitialmarking::Tree_Constructor(TiXmlElement *xmlnode, initMarking_node *&curnode,int tuplenum) {
    string text = xmlnode->Value();
    if(text == "subterm") {
        xmlnode = xmlnode->FirstChildElement();
        text = xmlnode->Value();
    }

    if(text == "numberof") {
        curnode = new initMarking_node;
        curnode->mytype = numberof;
        curnode->mm.initiate(placetid,placesid);
        TiXmlElement *subterm1 = xmlnode->FirstChildElement();
        TiXmlAttribute *numberconstant_value = subterm1->FirstChildElement()->FirstAttribute();
        curnode->value = stringToNum(numberconstant_value->Value());
        Tree_Constructor(subterm1->NextSiblingElement(),curnode->leftnode,tuplenum);
    }
    else if(text == "add") {
        TiXmlElement *addterm = xmlnode->FirstChildElement();
        curnode = new initMarking_node;
        curnode->mytype = add;
        curnode->mm.initiate(placetid,placesid);
        Tree_Constructor(addterm,curnode->leftnode,tuplenum);
        addterm = addterm->NextSiblingElement();
        if(addterm!=NULL) {
            Tree_Constructor(addterm,curnode->rightnode,tuplenum);
            addterm = addterm->NextSiblingElement();
        }

        initMarking_node *present,*predecessor;
        predecessor = curnode;
        while(addterm) {
            present = new initMarking_node;
            present->mytype = add;
            present->mm.initiate(placetid,placesid);
            present->leftnode = predecessor->rightnode;
            predecessor->rightnode = present;
            Tree_Constructor(addterm,present->rightnode,tuplenum);
            predecessor = present;
            addterm = addterm->NextSiblingElement();
        }
    }
    else if(text == "subtract") {
        TiXmlElement *subterm = xmlnode->FirstChildElement();
        curnode = new initMarking_node;
        curnode->mytype = subtract;
        curnode->mm.initiate(placetid,placesid);
        Tree_Constructor(subterm,curnode->leftnode,tuplenum);
        subterm = subterm->NextSiblingElement();
        Tree_Constructor(subterm,curnode->rightnode,tuplenum);
    }
    else if(text == "all") {
        string sortname = xmlnode->FirstChildElement("usersort")->FirstAttribute()->Value();
        curnode = new initMarking_node;
        curnode->mytype = all;
        curnode->mm.initiate(placetid,placesid);
        curnode->value = tuplenum;
        map<string,MSI>::iterator sortiter;
        sortiter = SortTable::mapSort.find(sortname);
        if(sortiter!=SortTable::mapSort.end()) {
            curnode->sid = (sortiter->second).sid;
            curnode->tid = (sortiter->second).tid;
        }
        else {
            cerr<<"[ERROR @ hlinitialmarking::Tree_Constructor] Can not recognize sort \'"<<sortname<<"\'."<<endl;
            exit(-1);
        }
    }
    else if(text == "tuple") {
        curnode = new initMarking_node;
        curnode->mytype = hlnodetype::tuple;
        curnode->mm.initiate(placetid,placesid);
        TiXmlElement *tupleterm = xmlnode->FirstChildElement();
        Tree_Constructor(tupleterm,curnode->leftnode,0);
        tupleterm = tupleterm->NextSiblingElement();
        Tree_Constructor(tupleterm,curnode->rightnode,1);
        tupleterm = tupleterm->NextSiblingElement();

        initMarking_node *present, *predecessor;
        predecessor = curnode;
        tuplenum = 2;
        while(tupleterm) {
            present = new initMarking_node;
            present->mytype = hlnodetype::tuple;
            present->mm.initiate(placetid,placesid);
            present->leftnode = predecessor->rightnode;
            predecessor->rightnode = present;
            Tree_Constructor(tupleterm,present->rightnode,tuplenum);
            predecessor = present;
            tupleterm = tupleterm->NextSiblingElement();
            tuplenum++;
        }
    }
    else if(text == "useroperator") {
        curnode = new initMarking_node;
        curnode->mytype = color;
        curnode->mm.initiate(placetid,placesid);
        curnode->value = tuplenum;
        string colorname = xmlnode->FirstAttribute()->Value();
        map<string,MCI>::iterator coloriter;
        coloriter = SortTable::mapColor.find(colorname);
        if(coloriter!=SortTable::mapColor.end()) {
            curnode->cid = coloriter->second.cid;
            curnode->sid = coloriter->second.sid;
            curnode->tid = usersort;
        }
        else {
            cerr<<"[ERROR @ hlinitialmarking::Tree_Constructor] Can not recognize color \'"<<colorname<<"\'."<<endl;
            exit(-1);
        }
    }
    else if(text == "finiteintrangeconstant") {
        curnode = new initMarking_node;
        curnode->mytype = color;
        curnode->tid = finiteintrange;
        curnode->mm.initiate(placetid,placesid);
        curnode->value = tuplenum;
        curnode->cid = stringToNum(xmlnode->FirstAttribute()->Value());
    }
    else if(text == "dotconstant") {
        curnode = new initMarking_node;
        curnode->mytype = color;
        curnode->tid = dot;
        curnode->mm.initiate(placetid,placesid);
    }
    else {
        cerr<<"[ERROR @ hlinitialmarking::Tree_Constructor] Can not recognize operator \'"<<text<<"\'."<<endl;
        exit(-1);
    }
}

void hlinitialmarking::Tree_Destructor(initMarking_node *node) {
    if(node->leftnode)
        Tree_Destructor(node->leftnode);
    if(node->rightnode)
        Tree_Destructor(node->rightnode);
    delete node;
}

void hlinitialmarking::get_Multiset(Multiset &ms) {
    get_Multiset(root->leftnode);
    MultisetCopy(ms,root->leftnode->mm,placetid,placesid);
}

void hlinitialmarking::get_Multiset(initMarking_node *node) {
    switch (node->mytype) {
        case hlnodetype::color: {
            Tokens *token = new Tokens;
            int psnum = 0;
            if(placetid == productsort) {
                psnum = SortTable::productsort[placesid].sortnum;
            }
            token->initiate(1,placetid,psnum);
            if(placetid == dot) {}
            else if(placetid == productsort) {
                token->tokenvalue->setColor(node->cid,node->value);
            }
            else {
                token->tokenvalue->setColor(node->cid);
            }
            node->mm.insert(token);
            break;
        }
        case hlnodetype::numberof: {
            get_Multiset(node->leftnode);
            MultisetCopy(node->mm,node->leftnode->mm,placetid,placesid);
            node->mm.unify_coefficient(node->value);
            break;
        }
        case hlnodetype::add: {
            get_Multiset(node->leftnode);
            MultisetCopy(node->mm,node->leftnode->mm,placetid,placesid);
            if(node->rightnode) {
                get_Multiset(node->rightnode);
                PLUS(node->mm,node->rightnode->mm);
            }
            break;
        }
        case hlnodetype::subtract: {
            get_Multiset(node->leftnode);
            MultisetCopy(node->mm,node->leftnode->mm,placetid,placesid);
            get_Multiset(node->rightnode);
            MINUS(node->mm,node->rightnode->mm);
            break;
        }
        case hlnodetype::all: {
            Tokens *token;
            int psnum = 0;
            if(placetid == productsort) {
                psnum = SortTable::productsort[placesid].sortnum;
            }

            if(node->tid == finiteintrange) {
                FiniteIntRange &firsort = SortTable::finitintrange[node->sid];
                int i;
                for(i=firsort.start;i<=firsort.end;++i) {
                    token = new Tokens;
                    token->initiate(1,placetid,psnum);
                    if(placetid == productsort) {
                        token->tokenvalue->setColor(i,node->value);
                    }
                    else {
                        token->tokenvalue->setColor(i);
                    }
                    node->mm.insert(token);
                }
            }
            else if(node->tid == usersort) {
                UserSort &sort = SortTable::usersort[node->sid];
                int i;
                for(i=0;i<sort.feconstnum;++i) {
                    token = new Tokens;
                    token->initiate(1,placetid,psnum);
                    if(placetid == productsort) {
                        token->tokenvalue->setColor(i,node->value);
                    }
                    else {
                        token->tokenvalue->setColor(i);
                    }
                    node->mm.insert(token);
                }
            }
            else {
                cerr<<"operator \"all\" can only be used for cyclicenumeration and finiteintrange sort"<<endl;
                exit(-1);
            }
            break;
        }
        case hlnodetype::tuple: {
            get_Multiset(node->leftnode);
            get_Multiset(node->rightnode);

            Tokens *token,*left,*right;
            int psnum = SortTable::productsort[placesid].sortnum;
            COLORID *product_color = new COLORID[psnum];
            COLORID *left_color = new COLORID[psnum];
            COLORID *right_color = new COLORID[psnum];

            for (left=node->leftnode->mm.tokenQ->next;left;left=left->next)
            {
                left->tokenvalue->getColor(left_color,psnum);
                for (right=node->rightnode->mm.tokenQ->next;right;right=right->next)
                {
                    right->tokenvalue->getColor(right_color,psnum);
                    pattern_merge(product_color,left_color,right_color,psnum);
                    token = new Tokens;
                    token->initiate(1,placetid,psnum);
                    token->tokenvalue->setColor(product_color,psnum);
                    node->mm.insert(token);
                }
            }

            delete [] product_color;
            delete [] left_color;
            delete [] right_color;
            break;
        }
        default: {
            cerr<<"[ERROR @ hlinitialmarking::get_Multiset] Encounter an unrecognized operator."<<endl;
            exit(-1);
            break;
        }
    }
}

