//
// Created by hecong on 2020/11/14.
//
#include "CPN.h"

void CPN_Transition::varinsert(VARID vid) {
    vector<VARID>::const_iterator iter;
    for(iter=relvararray.begin();iter!=relvararray.end();++iter) {
        if(*iter==vid)
            return;
    }
    relvararray.push_back(vid);
}

void CPN_Transition::get_relvariables() {
    vector<CSArc>::const_iterator iter;
    for(iter=producer.begin();iter!=producer.end();++iter) {
        get_relvariables(iter->arc_exp.root->leftnode);
    }
    for(iter=consumer.begin();iter!=consumer.end();++iter) {
        get_relvariables(iter->arc_exp.root->leftnode);
    }
}

void CPN_Transition::get_relvariables(arcnode *node) {
    if(node->mytype == hlnodetype::var) {
        varinsert(node->number);
    }

    if(node->leftnode)
        get_relvariables(node->leftnode);
    if(node->rightnode)
        get_relvariables(node->rightnode);
}

CPN::CPN() {
    placecount = 0;
    transitioncount = 0;
    arccount = 0;
    varcount = 0;
}

/*This function is to first scan the pnml file and do these things:
 * 1. get the number of places,transitions,arcs and variables;
 * 2. alloc space for place table, transition table, arc table,
 * P_sorttable and variable table;
 * 3. parse declarations, get all sorts;
 * */
void CPN::getSize(char *filename) {
    TiXmlDocument *mydoc = new TiXmlDocument(filename);
    if(!mydoc->LoadFile()) {
        cerr<<mydoc->ErrorDesc()<<endl;
    }

    TiXmlElement *root = mydoc->RootElement();
    if(root == NULL) {
        cerr<<"Failed to load file: no root element!"<<endl;
        mydoc->Clear();
    }

    TiXmlElement *net = root->FirstChildElement("net");
    // debug print
//    cout << net->FirstAttribute()->Value() <<endl;
    TiXmlElement *page = net->FirstChildElement("page");

    //doing the first job;
    while(page)
    {
        TiXmlElement *pageElement = page->FirstChildElement();
        while(pageElement)
        {
            string value = pageElement->Value();
            if(value == "place") {
                ++placecount;
            }
            else if(value == "transition") {
                ++transitioncount;
            }
            else if(value == "arc") {
                ++arccount;
            }
            pageElement = pageElement->NextSiblingElement();
        }
        page = page->NextSiblingElement();
    }

    //doing the second job
    place = new CPlace[placecount];
    transition = new CTransition[transitioncount];
    arc = new CArc[arccount];

    //doing the third job
    TiXmlElement *decl = net->FirstChildElement("declaration");
    TiXmlElement *decls = decl->FirstChildElement()->FirstChildElement("declarations");
    TiXmlElement *declContent = decls->FirstChildElement();
    SORTID ps_seek=0,us_seek=0,fir_seek=0;
    while(declContent)
    {
        string value = declContent->Value();
        if(value == "namedsort")
        {
            TiXmlElement *sort = declContent->FirstChildElement();
            string sortname = sort->Value();
            if(sortname == "cyclicenumeration")
            {
                UserSort uu;
                uu.mytype = usersort;
                uu.id = declContent->FirstAttribute()->Value();
                TiXmlElement *feconstant = sort->FirstChildElement();
                COLORID colorindex;
                for(uu.feconstnum=0,colorindex=0; feconstant; ++uu.feconstnum,++colorindex,feconstant=feconstant->NextSiblingElement())
                {
                    string feconstid = feconstant->FirstAttribute()->Value();
                    uu.members.push_back(feconstid);
                    uu.mapValue.insert(pair<string,SORTID>(feconstid,colorindex));
                    MCI  mci;
                    mci.cid = colorindex;
                    mci.sid = us_seek;
                    SortTable::mapColor.insert(pair<string,MCI>(feconstid,mci));
                }
                SortTable::usersort.push_back(uu);

                MSI msi;
                msi.tid = usersort;
                msi.sid = us_seek++;
                SortTable::mapSort.insert(pair<string,MSI>(uu.id,msi));
            }
            else if(sortname == "productsort")
            {
                ProductSort pp;
                pp.id = declContent->FirstAttribute()->Value();
                pp.mytype = productsort;
                int sortcount = 0;
                TiXmlElement *usersort = sort->FirstChildElement();
                for(sortcount=0;usersort;++sortcount,usersort=usersort->NextSiblingElement())
                {
                    string ustname = usersort->Value();
                    if(ustname != "usersort") {
                        cerr<<"Unexpected sort \'"<<ustname<<"\'. We assumed that every sort of productsort is usersort."<<endl;
                        exit(-1);
                    }
                    string relatedsortname = usersort->FirstAttribute()->Value();
                    pp.sortname.push_back(relatedsortname);
                }
                pp.sortnum = sortcount;
                SortTable::productsort.push_back(pp);

                MSI m;
                m.sid = ps_seek++;
                m.tid = productsort;
                SortTable::mapSort.insert(pair<string,MSI>(pp.id,m));
            }
            else if(sortname == "finiteintrange")
            {
                FiniteIntRange ff;
                ff.id = declContent->FirstAttribute()->Value();
                ff.mytype = finiteintrange;
                TiXmlAttribute *attr_start = sort->FirstAttribute();
                ff.start = stringToNum(attr_start->Value());
                TiXmlAttribute *attr_end = attr_start->Next();
                ff.end = stringToNum(attr_end->Value());
                SortTable::finitintrange.push_back(ff);

                MSI msi;
                msi.tid = finiteintrange;
                msi.sid = fir_seek++;
                SortTable::mapSort.insert(pair<string,MSI>(ff.id,msi));
            }
            else if(sortname == "dot") {
//                SortTable::hasdot = true;
            }
        }
        else if(value == "variabledecl")
        {
            ++varcount;
        }
        else if(value == "partition") {}
        else
        {
            cerr<<"Can not support XmlElement "<<value<<endl;
            exit(-1);
        }
        declContent = declContent->NextSiblingElement();
    }

    vector<ProductSort>::iterator psiter;
    for(psiter=SortTable::productsort.begin();psiter!=SortTable::productsort.end();++psiter)
    {
        vector<string>::iterator miter;
        for(miter=psiter->sortname.begin();miter!=psiter->sortname.end();++miter)
        {
            map<string,MSI>::iterator citer;
            citer = SortTable::mapSort.find(*miter);
            if(citer!=SortTable::mapSort.end())
            {
                psiter->sortid.push_back(citer->second);
            }
            else cerr<<"[CPN.cpp\\line223].ProductSort error."<<endl;
        }
    }
    VarTable::vartable = new Variable[varcount];
    VarTable::varcount = varcount;
    delete mydoc;
}

/*This function is to second scan pnml file just after getSize();
 * It does these jobs:
 * 1.parse pnml file, get every place's information,especially initMarking,
 * get every transition's information,especially guard fucntion;
 * get every arc's information, especially arc expressiion
 * 2.get all variable's information, set up variable table;
 * 3.construct the CPN, complete every place and transition's producer and consumer
 * */
void CPN::readPNML(char *filename) {
    TiXmlDocument *mydoc = new TiXmlDocument(filename);
    if(!mydoc->LoadFile()) {
        cerr<<mydoc->ErrorDesc()<<endl;
    }

    TiXmlElement *root = mydoc->RootElement();
    if(root == NULL) {
        cerr<<"Failed to load file: no root element!"<<endl;
        mydoc->Clear();
    }

    TiXmlElement *net = root->FirstChildElement("net");
    TiXmlElement *page = net->FirstChildElement("page");

    index_t pptr = 0;
    index_t tptr = 0;
    index_t aptr = 0;
    index_t vpter = 0;

    //doing the second job:
    TiXmlElement *declContent = net->FirstChildElement("declaration")
            ->FirstChildElement("structure")
            ->FirstChildElement("declarations")
            ->FirstChildElement();
    while(declContent)
    {
        string value = declContent->Value();
        if(value == "variabledecl")
        {
            VarTable::vartable[vpter].id = declContent->FirstAttribute()->Value();
            VarTable::vartable[vpter].vid = vpter;
            TiXmlElement *ust = declContent->FirstChildElement();
            string ustname = ust->Value();
            if(ustname != "usersort") {
                cerr<<"Unexpected sort\'"<<ustname<<"\'. We assumed that all variables are usersort variables."<<endl;
                exit(-1);
            }
            string sortname = ust->FirstAttribute()->Value();
            map<string,MSI>::iterator iter;
            iter = SortTable::mapSort.find(sortname);
            VarTable::vartable[vpter].sid = (iter->second).sid;
            VarTable::vartable[vpter].tid = (iter->second).tid;
            if(VarTable::vartable[vpter].tid == usersort) {
                VarTable::vartable[vpter].upbound = SortTable::usersort[VarTable::vartable[vpter].sid].feconstnum;
            }
            VarTable::mapVariable.insert(pair<string,VARID>(VarTable::vartable[vpter].id,vpter));
            if((iter->second).tid != usersort && (iter->second).tid != finiteintrange) {
                cerr<<"EnPAC only allows \"cyclicenumeration\" and \"finiteintrange\" sort variables."<<endl;
                exit(-1);
            }
            ++vpter;
        }
        declContent = declContent->NextSiblingElement();
    }

    while(page)
    {
        TiXmlElement *pageElement = page->FirstChildElement();
        for(;pageElement;pageElement=pageElement->NextSiblingElement())
        {
            string value = pageElement->Value();
            if(value == "place"){
                CPlace &pp = place[pptr];
                //get id
                TiXmlAttribute *attr = pageElement->FirstAttribute();
                pp.id = attr->Value();
                //get type(mysort,mysortid)
                TiXmlElement *ttype = pageElement->FirstChildElement("type")
                        ->FirstChildElement("structure")->FirstChildElement("usersort");
                string sortname = ttype->FirstAttribute()->Value();
                if(sortname == "dot")
                {
                    pp.tid=dot;
                }
                else {
                    map<string,MSI>::iterator sortiter;
                    sortiter = SortTable::mapSort.find(sortname);
                    if(sortiter!=SortTable::mapSort.end()) {
                        pp.sid = (sortiter->second).sid;
                        pp.tid = (sortiter->second).tid;
                    }
                    else
                    {
                        cerr<<"Can not recognize sort \'"<<sortname<<"\'."<<endl;
                        exit(-1);
                    }
                }
                pp.initM.initiate(pp.tid,pp.sid);

                /*get initMarking*/
                TiXmlElement *hlinitialMarking = pageElement->FirstChildElement("hlinitialMarking");
                if(hlinitialMarking) {
                    getInitMarking(pp,hlinitialMarking);
                }

                mapPlace.insert(pair<string,index_t>(pp.id,pptr));
                ++pptr;
            }
            else if(value == "transition")
            {
                CTransition &tt = transition[tptr];
                tt.id = pageElement->FirstAttribute()->Value();
                TiXmlElement *condition = pageElement->FirstChildElement("condition");
                if(condition)
                {
                    tt.guard.constructor(condition);
                    tt.hasguard = true;
                }
                else
                {
                    tt.hasguard = false;
                }
                mapTransition.insert(pair<string,index_t>(tt.id,tptr));
                ++tptr;
            }
            else if(value == "arc")
            {
                CArc &aa = arc[aptr];
                TiXmlAttribute *attr = pageElement->FirstAttribute();
                aa.id = attr->Value();
                attr = attr->Next();
                aa.source_id = attr->Value();
                attr = attr->Next();
                aa.target_id = attr->Value();
                TiXmlElement *hlinscription = pageElement->FirstChildElement("hlinscription");
                aa.arc_exp.Tree_Constructor(hlinscription);
                ++aptr;
            }
        }
        page = page->NextSiblingElement();
    }

    //doing the third job
    for(int i=0;i<arccount;++i)
    {
        CArc &aa = arc[i];
        map<string,index_t>::iterator souiter;
        map<string,index_t>::iterator tagiter;
        souiter = mapPlace.find(aa.source_id);
        if(souiter == mapPlace.end())
        {
            //souiter是一个变迁
            aa.isp2t = false;
            souiter = mapTransition.find(aa.source_id);
            tagiter = mapPlace.find(aa.target_id);
            CSArc forplace,fortrans;

            forplace.idx = souiter->second;
            forplace.arc_exp = aa.arc_exp;
            forplace.arc_exp.initiate(place[tagiter->second].tid,place[tagiter->second].sid);
            forplace.arc_exp.set_tranid(souiter->second);
            place[tagiter->second].producer.push_back(forplace);

            fortrans.idx = tagiter->second;
            fortrans.arc_exp = aa.arc_exp;
            fortrans.arc_exp.initiate(place[tagiter->second].tid,place[tagiter->second].sid);
            fortrans.arc_exp.set_tranid(souiter->second);
            transition[souiter->second].consumer.push_back(fortrans);
        }
        else
        {
            aa.isp2t = true;
            tagiter = mapTransition.find(aa.target_id);
            CSArc forplace,fortrans;

            forplace.idx = tagiter->second;
            forplace.arc_exp = aa.arc_exp;
            forplace.arc_exp.initiate(place[souiter->second].tid,place[souiter->second].sid);
            forplace.arc_exp.set_tranid(tagiter->second);
            place[souiter->second].consumer.push_back(forplace);

            fortrans.idx = souiter->second;
            fortrans.arc_exp = aa.arc_exp;
            fortrans.arc_exp.initiate(place[souiter->second].tid,place[souiter->second].sid);
            fortrans.arc_exp.set_tranid(tagiter->second);
            transition[tagiter->second].producer.push_back(fortrans);
        }
    }

    for(int i=0;i<transitioncount;++i) {
        transition[i].get_relvariables();
    }
    delete mydoc;
}

void CPN::getInitMarking(CPlace &pp, TiXmlElement *xmlnode) {
    pp.init_exp = new hlinitialmarking(pp.tid,pp.sid);
    pp.init_exp->Tree_Constructor(xmlnode);
    pp.init_exp->get_Multiset(pp.initM);
    delete pp.init_exp;
}


void CPN::printCPN() {
    char placefile[]="place_info.txt";
//    char transfile[]="transition_info.txt";
//    char arcfile[]="arc_info.txt";
    ofstream outplace(placefile,ios::out);
//    ofstream outtrans(transfile,ios::out);
//    ofstream outarc(arcfile,ios::out);

    outplace<<"PLACE INFO {"<<endl;
    for(int i=0;i<placecount;++i)
    {
        string marking = "";
        place[i].initM.printToken(marking);
        outplace<<"\t"<<place[i].id<<" "<<marking<<endl;
    }
    outplace<<"}"<<endl;

    /********************print dot graph*********************/
    char dotfile[]="CPN.dot";
    ofstream outdot(dotfile,ios::out);
    outdot<<"digraph CPN {"<<endl;
//    outdot<<"rankdir = LR;"<<endl;    //指定绘图的方向 (LR从左到右绘制)

    for(int i=0;i<placecount;++i)
    {
        outdot<<"\t"<<place[i].id<<" [shape=circle]"<<endl;
    }
    outdot<<endl;
    for(int i=0;i<transitioncount;++i)
    {
        CTransition &t = transition[i];
        if(t.hasguard)
        {
            string guardexp;
            t.guard.printEXP(guardexp);
            outdot<<"\t"<<transition[i].id<<" [shape=box,label=\""<<transition[i].id<<"//"<<guardexp<<"\"]"<<endl;
        }
        else outdot<<"\t"<<transition[i].id<<" [shape=box]"<<endl;
    }
    for(int i=0;i<arccount;++i)
    {
        CArc &a = arc[i];
        string arcexp;
        a.arc_exp.printEXP(arcexp);
        outdot<<"\t"<<a.source_id<<"->"<<a.target_id<<" [label=\""<<arcexp<<"\"]"<<endl;
    }
    outdot<<"}"<<endl;
}

void CPN::printSort() {
    char sortfile[]="sorts_info.txt";
    ofstream outsort(sortfile,ios::out);

    outsort<<"USERSORT:"<<endl;
    vector<UserSort>::iterator uiter;
    for(uiter=SortTable::usersort.begin();uiter!=SortTable::usersort.end();++uiter)
    {
        outsort<<"name: "<<uiter->id<<endl;
        outsort<<"constants: (";
        for(int i=0;i<uiter->feconstnum;++i)
        {
            outsort << uiter->members[i] << ",";
        }
        outsort<<")"<<endl;
        outsort<<"------------------------"<<endl;
    }
    outsort<<endl;

    outsort<<"FINITEINTRANGE:"<<endl;
    vector<FiniteIntRange>::iterator fiter;
    for(fiter=SortTable::finitintrange.begin();fiter!=SortTable::finitintrange.end();++fiter)
    {
        outsort<<"name: "<<fiter->id<<endl;
        outsort<<"start: "<<fiter->start<<endl;
        outsort<<"end: "<<fiter->end<<endl;
        outsort<<"------------------------"<<endl;
    }
    outsort<<endl;

    outsort<<"PRODUCTSORT:"<<endl;
    vector<ProductSort>::iterator psiter;
    for(psiter=SortTable::productsort.begin();psiter!=SortTable::productsort.end();++psiter)
    {
        outsort<<"name: "<<psiter->id<<endl;
        outsort<<"member: (";
        for(int i=0;i<psiter->sortnum;++i)
        {
            outsort<<psiter->sortname[i]<<",";
        }
        outsort<<")"<<endl;
        outsort<<"------------------------"<<endl;
    }
    outsort<<endl;
}

void CPN::printVar() {
    ofstream outvar("sorts_info.txt",ios::app);
    outvar<<"VARIABLES:"<<endl;
    for(int i=0;i<varcount;++i)
    {
        outvar<<"name: "<<VarTable::vartable[i].id<<endl;
        if(VarTable::vartable[i].tid == usersort)
            outvar<<"related sort: "<<SortTable::usersort[VarTable::vartable[i].sid].id<<endl;
        else if(VarTable::vartable[i].tid == finiteintrange)
            outvar<<"related sort: "<<SortTable::finitintrange[VarTable::vartable[i].sid].id<<endl;
        outvar<<"------------------------"<<endl;
    }
}

void CPN::printTransVar() {
    ofstream outvar("trans_var.txt",ios::out);
    for(int i=0;i<transitioncount;++i)
    {
        CPN_Transition &tt = transition[i];
        outvar<<transition[i].id<<":";
        vector<VARID>::iterator iter;
        for(iter=tt.relvararray.begin();iter!=tt.relvararray.end();++iter)
        {
            outvar<<" "<<VarTable::vartable[*iter].id;
        }
        outvar<<endl;
    }
}

CPN::~CPN() {
    delete [] place;
    delete [] transition;
    delete [] arc;
}

index_t CPN::getPPosition(string str) {
    map<string,index_t>::iterator idx_p;
    idx_p = mapPlace.find(str);
    if(idx_p == mapPlace.end()) {
        return INDEX_ERROR;
    }
    else {
        return idx_p->second;
    }
}

index_t CPN::getTPosition(string str) {
    map<string,index_t>::iterator idx_t;
    idx_t = mapTransition.find(str);
    if(idx_t == mapTransition.end()) {
        return INDEX_ERROR;
    }
    else {
        return idx_t->second;
    }
}

int CPN::SLICE(vector<string> criteria_p, vector<string> criteria_t) {
    int size=0;
    slice_p.clear();
    //slice_t.clear();
    for(auto i=0;i<cpn->transitioncount;i++)
        cpn->transition[i].significant= false;
    vector<string> tmp_p, tmp_t;
    vector<index_t> result_p;
    for (auto i = criteria_p.begin(); i != criteria_p.end(); i++) {
        if (!exist_in(tmp_p, *i))
            tmp_p.push_back(*i);
    }
    for (auto i = criteria_t.begin(); i != criteria_t.end(); i++) {
        if (!exist_in(tmp_t, *i))
            tmp_t.push_back(*i);
    }

    while(!tmp_p.empty() ||!tmp_t.empty()){
        //T′ ← T′∪•P′∪P′•;
        if(!tmp_p.empty()) {
            auto idx=mapPlace.find(*tmp_p.begin())->second;
            CPlace &p = place[idx];
            if(!exist_in(result_p,idx))
                result_p.push_back(idx);
            auto p_pro = p.producer;
            if (!p_pro.empty()) {
                for (auto ipre = p_pro.begin(); ipre != p_pro.end(); ipre++) {
                    CTransition &t_p_pre = transition[ipre->idx];
                    if(!t_p_pre.significant)
                        tmp_t.push_back(t_p_pre.id);
                }
            }
            auto p_con = p.consumer;
            if (!p_con.empty()) {
                for (auto icon = p_con.begin(); icon != p_con.end(); icon++) {
                    CTransition &t_p_con = transition[icon->idx];
                    if(!t_p_con.significant)
                        tmp_t.push_back(t_p_con.id);
                }
            }
            tmp_p.erase(tmp_p.begin());
        }
        // P′ ← P′∪•T′;
        if(!tmp_t.empty()) {
            auto t_idx=mapTransition.find(*tmp_t.begin())->second;
            CTransition &t = transition[t_idx];
            if(!t.significant) {
                t.significant = true;
                ++size;
            }
            auto t_pre = t.producer;
            if (!t_pre.empty()) {
                for (auto ipre = t_pre.begin(); ipre != t_pre.end(); ipre++) {
                    CPlace &p_t_pre = place[ipre->idx];
                    if (!exist_in(result_p, ipre->idx)) {
                        tmp_p.push_back(p_t_pre.id);
                    }
                }
            }
            tmp_t.erase(tmp_t.begin());
        }
    }
    int j=0;
    for (unsigned int i = 0; i < placecount; i++) {
        CPlace &p = place[i];
        if (exist_in(result_p, i)) {
            slice_p.push_back(i);
            p.project_idx=j;
            j++;
        }
    }
    return size;
}
//void CPN::computeVis(set<index_t> &visItems, bool cardinality) {
//    for(int i=0;i<transitioncount;i++)
//        transition[i].significant = false;
//    for(int i=0;i<placecount;i++)
//        place[i].significant = false;
//
//    set<index_t> visTransitions;
//    if(cardinality) {
//        set<index_t>::iterator iter;
//        for(iter=visItems.begin();iter!=visItems.end();iter++) {
//            CPN_Place &pp = place[*iter];
//            pp.significant = true;
//            for(int i=0;i<pp.producer.size();i++) {
//                visTransitions.insert(pp.producer[i].idx);
//            }
//            for(int i=0;i<pp.consumer.size();i++) {
//                visTransitions.insert(pp.consumer[i].idx);
//            }
//        }
//    }
//    else {
//        visTransitions = visItems;
//    }
//    VISpread(visTransitions);
//}

//void CPN::VISpread(set<index_t> &visTransitions) {
//    set<index_t> unexpanded;
//    unexpanded = visTransitions;
//    while (!unexpanded.empty()) {
//        index_t idxT = *unexpanded.begin();
//        unexpanded.erase(unexpanded.begin());
//        CPN_Transition &expandTran = transition[idxT];
//        expandTran.significant = true;
//        vector<CSArc>::iterator sarcP;
//        for(sarcP=expandTran.producer.begin();sarcP!=expandTran.producer.end();sarcP++) {
//            CPN_Place &pp = place[sarcP->idx];
//            pp.significant = true;
//            vector<CSArc>::iterator sarcT;
//            for(sarcT=pp.producer.begin();sarcT!=pp.producer.end();sarcT++) {
//                if(!transition[sarcT->idx].significant) {
//                    transition[sarcT->idx].significant = true;
//                    unexpanded.insert(sarcT->idx);
//                }
//            }
//            for(sarcT=pp.consumer.begin();sarcT!=pp.consumer.end();sarcT++) {
//                if(!transition[sarcT->idx].significant) {
//                    transition[sarcT->idx].significant = true;
//                    unexpanded.insert(sarcT->idx);
//                }
//            }
//        }
//    }
//}
//
//bool CPN::utilizeSlice() {
//    for(int i=0;i<transitioncount;i++) {
//        if(!transition[i].significant)
//            return true;
//    }
//    return false;
//}
