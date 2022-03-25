//
// Created by hecong on 2020/12/4.
//
#include "CPN_Product.h"

bool timeflag;
void sig_handler(int num)
{
    //printf("time out .\n");
    timeflag = false;
    ready2exit = true;
}

ProductState::ProductState() {
    id = 0;
    RGname_ptr = NULL;
    BAname_id = -1;
    stacknext = UNREACHABLE;
    cur_RGchild = NULL;
    pba = NULL;
}

void ProductState::getNextRGChild(bool &exist) {
    if(fireinfo.tranid >= cpn->transitioncount) {
        cur_RGchild = NULL;
        return;
    }

    while(fireinfo.virgin) {
        if(!RGname_ptr->Binding_Available[fireinfo.tranid]) {
            RGname_ptr->tran_FireableBindings(fireinfo.tranid);
        }
        if(ready2exit)
            return;
        fireinfo.fireBinding = RGname_ptr->enbindings[fireinfo.tranid].listhead;
        if(fireinfo.fireBinding == NULL) {
            if(NEXTTRANSITION() == FAIL)
            {
                cur_RGchild = NULL;
                return;
            }
            else {
                continue;
            }
        }
        else {
            fireinfo.virgin = false;
            cur_RGchild = cpnRG->RGNode_Child(RGname_ptr,fireinfo.fireBinding,fireinfo.tranid,exist);
            return;
        }
    }

    if(NEXTBINDING() == FAIL) {
        if(NEXTTRANSITION() == FAIL) {
            cur_RGchild = NULL;
            return;
        }
        else {
            getNextRGChild(exist);
        }
    }
    else {
        cur_RGchild = cpnRG->RGNode_Child(RGname_ptr,fireinfo.fireBinding,fireinfo.tranid,exist);
        return;
    }
}

int ProductState::NEXTBINDING() {
    if(fireinfo.fireBinding == NULL)
        return FAIL;
    fireinfo.fireBinding = fireinfo.fireBinding->next;
    if(fireinfo.fireBinding == NULL)
        return FAIL;
    return SUCCESS;
}

int ProductState::NEXTTRANSITION() {
    if(fireinfo.tranid>=cpn->transitioncount)
        return FAIL;
    if(NEXTFREE) {
        do{
            fireinfo.tranid++;
        } while(!cpn->transition[fireinfo.tranid].significant && fireinfo.tranid<cpn->transitioncount);
    }
    else {
        fireinfo.tranid++;
    }
    fireinfo.virgin = true;
    if(fireinfo.tranid>=cpn->transitioncount)
        return FAIL;
    return SUCCESS;
}

CHashtable::CHashtable() {
    table = new S_ProductState*[CHASHSIZE];
    for(int i=0;i<CHASHSIZE;++i){
        table[i] = NULL;
    }
    nodecount = 0;
    hash_conflict_times = 0;
}

CHashtable::~CHashtable() {
    for(int i=0;i<CHASHSIZE;++i) {
        if(table[i]!=NULL)
        {
            S_ProductState *p=table[i];
            S_ProductState *q;
            while(p!=NULL) {
                q = p->hashnext;
                delete p;
                p = q;
            }
        }
    }
    delete [] table;
    MallocExtension::instance()->ReleaseFreeMemory();
}

index_t CHashtable::hashfunction(ProductState *node) {
    index_t hashvalue;
    index_t size = CHASHSIZE-1;
    hashvalue = node->RGname_ptr->Hash(cpnRG->weight);
    hashvalue = hashvalue & size;

    index_t Prohashvalue = hashvalue + node->BAname_id;
    Prohashvalue = Prohashvalue & size;
    return Prohashvalue;
}

index_t CHashtable::hashfunction(S_ProductState *node) {
    index_t hashvalue;
    index_t size = CHASHSIZE-1;
    hashvalue = node->RGname_ptr->Hash(cpnRG->weight);
    hashvalue = hashvalue & size;

    index_t Prohashvalue = hashvalue + node->BAname_id;
    Prohashvalue = Prohashvalue & size;
    return Prohashvalue;
}

void CHashtable::insert(ProductState *node) {
    S_ProductState *scp = new S_ProductState(node);
    index_t idx = hashfunction(node);
    if(table[idx]!=NULL)
        hash_conflict_times++;
    scp->hashnext = table[idx];
    table[idx] = scp;
    nodecount++;
}

S_ProductState *CHashtable::search(ProductState *node) {
    index_t idx = hashfunction(node);
    S_ProductState *p = table[idx];
    while(p!=NULL)
    {
        if(p->BAname_id==node->BAname_id && p->RGname_ptr==node->RGname_ptr)
            return p;
        p=p->hashnext;
    }
    return NULL;
}

void CHashtable::remove(ProductState *node) {
    index_t idx = hashfunction(node);
    S_ProductState *p=table[idx];
    S_ProductState *q;
    if(p->BAname_id==node->BAname_id && p->RGname_ptr==node->RGname_ptr)
    {
        q=p->hashnext;
        table[idx] = q;
        delete p;
        return;
    }

    q=p;
    p=p->hashnext;
    while(p!=NULL)
    {
        if(p->BAname_id==node->BAname_id && p->RGname_ptr==node->RGname_ptr)
        {
            q->hashnext=p->hashnext;
            delete p;
            return;
        }

        q=p;
        p=p->hashnext;
    }
    cerr<<"Couldn't delete from hashtable!"<<endl;
}

void CHashtable::resetHash() {
    for(int i=0;i<CHASHSIZE;++i)
    {
        if(table[i]!=NULL) {
            S_ProductState *p=table[i];
            S_ProductState *q;
            while(p!=NULL) {
                q=p->hashnext;
                delete p;
                p=q;
            }
        }
    }
    memset(table,NULL,sizeof(S_ProductState*)*CHASHSIZE);
    nodecount = 0;
    hash_conflict_times = 0;
}


CPstack::CPstack() {
    toppoint = 0;
    hashlink = new index_t[CHASHSIZE];
    memset(hashlink,UNREACHABLE,sizeof(index_t)*CHASHSIZE);
    mydata = new ProductState* [CPSTACKSIZE];
    memset(mydata,NULL,sizeof(ProductState*)*CPSTACKSIZE);
}

CPstack::~CPstack() {
    delete [] hashlink;
    for(int i=0;i<toppoint;++i)
    {
        if(mydata[i]!=NULL)
            delete mydata[i];
    }
    delete [] mydata;
    MallocExtension::instance()->ReleaseFreeMemory();
}

index_t CPstack::hashfunction(ProductState *node) {
    index_t hashvalue = node->RGname_ptr->Hash(cpnRG->weight);
    index_t size = CHASHSIZE-1;
    hashvalue = hashvalue & size;
    index_t  Prohashvalue = (hashvalue+node->BAname_id)&size;
    return Prohashvalue;
}

ProductState *CPstack::top() {
    return mydata[toppoint-1];
}

ProductState *CPstack::pop() {
    ProductState *popitem = mydata[--toppoint];
    index_t hashpos = hashfunction(popitem);
    hashlink[hashpos] = popitem->stacknext;
    mydata[toppoint] = NULL;
    return popitem;
}

ProductState *CPstack::search(ProductState *node) {
    index_t hashpos = hashfunction(node);
    index_t pos = hashlink[hashpos];
    ProductState *p;
    while(pos!=UNREACHABLE)
    {
        p=mydata[pos];
        if(p->BAname_id==node->BAname_id && p->RGname_ptr==node->RGname_ptr)
        {
            return p;
        }
        pos = p->stacknext;
    }
    return NULL;
}

int CPstack::push(ProductState *node) {
    if(toppoint >= CPSTACKSIZE) {
        return ERROR;
    }
    index_t hashpos = hashfunction(node);
    node->stacknext = hashlink[hashpos];
    hashlink[hashpos] = toppoint;
    mydata[toppoint++] = node;
    return OK;
}

NUM_t CPstack::size() {
    return toppoint;
}

bool CPstack::empty() {
    return (toppoint==0)?true:false;
}

void CPstack::clear() {
    toppoint = 0;
    for(int i=0;i<toppoint;++i)
    {
        if(mydata[i]!=NULL) {
            delete mydata[i];
        }
    }
    memset(mydata,NULL, sizeof(ProductState*)*CPSTACKSIZE);
    memset(hashlink,UNREACHABLE,sizeof(index_t)*CHASHSIZE);
}

CPN_Product_Automata::CPN_Product_Automata(StateBuchi *sba) {
    ret = -1;
    result = true;
    ba = sba;
    memory_flag = true;
    stack_flag = true;
    consistency = true;
}

CPN_Product_Automata::~CPN_Product_Automata() {
    MallocExtension::instance()->ReleaseFreeMemory();
}

/*function: 判断可达图的一个状态和buchi自动机的一个状态能否合成交状态
 * in: state,可达图状态指针，指向要合成的可达图状态
 * sj,buchi自动机状态在邻接表中的序号
 * out: true(可以合成交状态) or false(不能合成状态)
 * */
bool CPN_Product_Automata::isLabel(CPN_RGNode *state, int sj) {
    string str = ba->vertics[sj].label;
    if(str == "true")
        return true;

    bool mark = false;
    while(true)
    {
        int pos = str.find_first_of("&&");
        if(pos == string::npos)   //最后一个原子命题
        {
            if(judgeF(str))
            {
                //如果是F类型公式
                /*a && b && c:
                 * true: 都为true
                 * false： 只要有一个为false
                 * */
                mark=handleLTLF(str,state);
                if(mark == false)
                    return false;

            } else {
                //如果是C类型公式
                mark = handleLTLC(str,state);
                if(mark == false)
                    return false;
            }
            break;
        }

        string AP = str.substr(0,pos);

        if(judgeF(AP))
        {
            //如果是F类型公式
            /*a && b && c:
             * true: 都为true
             * false： 只要有一个为false
             * */
            mark = handleLTLF(AP,state);
            if(mark==false)
                return false;
        }
        else
        {
            mark = handleLTLC(AP,state);
            if(mark == false)
                return false;
        }
        str = str.substr(pos+2,str.length()-pos-2);
    }
    return true;
}

bool CPN_Product_Automata::newisLabel(CPN_RGNode *state, int sj) {
    if (ba->vertics[sj].invalid)
        return false;
    if (ba->vertics[sj].label == "true")
        return true;

    atomictable &AT = *(ba->pAT);
    const vector<Atomic> &links = ba->vertics[sj].links;
    Atomic temp;
    int atomic_num = links.size();
    bool atomicTrue = false;

    for (int i = 0; i < atomic_num; i++){
        temp = links[i];

        if (AT.atomics[temp.atomicmeta_link].mytype == PT_FIREABILITY)
            //call F handle
            atomicTrue = checkLTLF(state, AT.atomics[temp.atomicmeta_link]);
        else if (AT.atomics[temp.atomicmeta_link].mytype == PT_CARDINALITY)
            //call C handle
            atomicTrue = checkLTLC(state, AT.atomics[temp.atomicmeta_link]);

        if ((atomicTrue&&temp.negation)||(!atomicTrue&&!temp.negation)) {
            return false;
        }
    }
    return true;
}

bool CPN_Product_Automata::checkLTLF(CPN_RGNode *state, atomicmeta &am) {
    bool ans = false;
    int t_num = am.fires.size();
    for(int i=0;i<t_num;++i) {
        if(state->isfirable(am.fires[i])) {
            ans = true;
            break;
        }
    }
    return ans;
}

bool CPN_Product_Automata::checkLTLC(CPN_RGNode *state, atomicmeta &am) {
    NUM_t left = 0;
    NUM_t right = 0;
    const cardmeta *p;

    //left
    if (am.leftexp.constnum != -1)
        left = am.leftexp.constnum;
    else {
        p = am.leftexp.expression;
        while (p) {
            left += state->readPlace(p->placeid);
            p = p->next;
        }
    }

    //right
    if (am.rightexp.constnum != -1)
        right = am.rightexp.constnum;
    else {
        p = am.rightexp.expression;
        while (p) {
            right += state->readPlace(p->placeid);
            p = p->next;
        }
    }

    return left <= right;
}

bool CPN_Product_Automata::judgeF(string s) {
    int pos = s.find("<=");
    if (pos == string::npos)
        return true;            //LTLFireability category
    else
        return false;          //LTLCardinality category
}

/*function:判断F类型中的一个原子命题在状态state下是否成立
 * in: s是公式的一小部分，一个原子命题； state，状态
 * out: true(成立) or false(不成立)
 * */
bool CPN_Product_Automata::handleLTLF(string s, CPN_RGNode *state) {
    if(s[0] == '!')   //前面带有'!'的is-fireable{}
    {
        /*!{t1 || t2 || t3}：
         * true：t1不可发生 并且 t2不可发生 并且 t3不可发生
         * false： 只要有一个能发生
         * */
        s=s.substr(2,s.length()-2); //去掉“!{”
        while(true)
        {
            //取出一个变迁，看其是否在firetrans里
            int pos = s.find_first_of(",");
            if(pos < 0)
                break;

            string transname = s.substr(0,pos);  //取出一个变迁

            if(state->isfirable(transname))
                return false;

            s=s.substr(pos+1,s.length()-pos);
        }
        return true;
    }
    else              //单纯的is-fireable{}原子命题
    {

        /*{t1 || t2 || t3}:
	     * true: 只要有一个能发生
	     * false: 都不能发生
	     * */
        s=s.substr(1,s.length()-1);//去掉‘{’

        while(true)
        {
            int pos = s.find_first_of(",");
            if(pos < 0)
                break;

            string transname = s.substr(0,pos);

            if(state->isfirable(transname))
                return true;

            s=s.substr(pos+1,s.length()-pos);
        }
        return false;
    }
}

/*function: 判断C类型公式中的一个原子命题s在状态state下是否成立
 * in: s,原子命题； state,状态
 * out: true(成立) or false(不成立)
 * */
bool CPN_Product_Automata::handleLTLC(string s, CPN_RGNode *state) {
    NUM_t front_sum,latter_sum;
    if(s[0]=='!')
    {
        /*!(front <= latter)
	     * true:front > latter
	     * false:front <= latter
	     * */
        s=s.substr(2,s.length()-2);
        handleLTLCstep(front_sum,latter_sum,s,state);
        if(front_sum<=latter_sum)
            return false;
        else
            return true;
    }
    else
    {
        /*(front <= latter)
        * true:front <= latter
        * false:front > latter
        * */
        s=s.substr(1,s.length()-1);
        handleLTLCstep(front_sum,latter_sum,s,state);
        if(front_sum<=latter_sum)
            return true;
        else
            return false;
    }
}

/*function:计算在状态state下，C公式"<="前面库所的token和front_sum和后面库所的token和latter_sum
 * in: s,公式； state,状态
 * out: front_sum前半部分的和, latter_sum后半部分的和
 * */
void CPN_Product_Automata::handleLTLCstep(NUM_t &front_sum, NUM_t &latter_sum, string s, CPN_RGNode *state) {
    if(s[0]=='t')    //前半部分是token-count的形式
    {
        int pos = s.find_first_of("<=");      //定位到<=前
        string s_token = s.substr(12,pos-13); //去除"token-count(" ")"  ֻ只剩p1,p2,
        front_sum = sumtoken(s_token,state);  //计算token和

        s=s.substr(pos+2,s.length()-pos-2);

        if(s[0]=='t')  //后半部分是token-count
        {
            s_token = s.substr(12,s.length()-14);
            latter_sum = sumtoken(s_token,state);
        }
        else           //后半部分是常数
        {
            s=s.substr(0,s.length()-1);
            latter_sum=atoi(s.c_str());
        }
    }
    else             //前半部分是常数，那后半部分肯定是token-count
    {
        int pos = s.find_first_of("<=");
        string num = s.substr(0,pos);
        front_sum = atoi(num.c_str());

        s=s.substr(pos+14,s.length()-pos-16);
        latter_sum = sumtoken(s,state);
    }
}

NUM_t CPN_Product_Automata::sumtoken(string s, CPN_RGNode *state) {
    NUM_t sum=0;
    while(true)
    {
        int pos = s.find_first_of(",");
        if(pos == string::npos)
            break;
        string placename = s.substr(0,pos);
        map<string,index_t>::iterator piter;
        piter = cpn->mapPlace.find(placename);
        if(piter == cpn->mapPlace.end()) {
            consistency = false;
            return 0;
        }
        sum += state->marking[piter->second].Tokensum();
        s=s.substr(pos+1,s.length()-pos);
    }
    return sum;
}

ProductState *CPN_Product_Automata::getNextChild(ProductState *curstate) {
    bool exist;
    if(curstate->cur_RGchild == NULL) {
        curstate->getNextRGChild(exist);
    }

    while(curstate->cur_RGchild)
    {
        while(curstate->pba)
        {
            if(ready2exit)
                return NULL;
            if(newisLabel(curstate->cur_RGchild,curstate->pba->destination))
            {
                //能够生成交状态
                ProductState *qs = new ProductState;
                qs->RGname_ptr = curstate->cur_RGchild;
                qs->BAname_id = curstate->pba->destination;
                qs->pba = ba->vertics[qs->BAname_id].firstarc;

                if(qs->pba == NULL) {
                    cerr<<"detect a non-sense SBA state"<<endl;
                    exit(5);
                }

//                cout<<"\033[41m("<<qs->RGname_ptr->markingid<<","<<qs->BAname_id<<")\033[0m"<<endl;
                curstate->pba = curstate->pba->next;
                return qs;
            }
            curstate->pba = curstate->pba->next;
        }
        curstate->getNextRGChild(exist);
        curstate->pba = ba->vertics[curstate->BAname_id].firstarc;
    }

    if(curstate->RGname_ptr->deadmark())
    {
        //表示当前状态没有可发生变迁，他的下一个状态仍然为自己
        CPN_RGNode *rgseed = curstate->RGname_ptr;
        while(curstate->pba)
        {
            if(ready2exit)
                return NULL;
            if(newisLabel(rgseed,curstate->pba->destination))
            {
                //能够生成交状态
                ProductState *qs = new ProductState;
                qs->RGname_ptr = rgseed;
                qs->BAname_id = curstate->pba->destination;
                qs->pba = ba->vertics[qs->BAname_id].firstarc;

                if(qs->pba == NULL) {
                    cerr<<"detect a non-sense SBA state"<<endl;
                    exit(5);
                }

                //cout<<"\033[41m("<<qs->RGname_ptr->numid<<","<<qs->BAname_id<<")\033[0m"<<endl;
                curstate->pba = curstate->pba->next;
                return qs;
            }
            curstate->pba = curstate->pba->next;
        }
        return NULL;
    } else {
        return NULL;
    }
}

void CPN_Product_Automata::TCHECK(ProductState *p0) {
    PUSH(p0);
    while(!dstack.empty() && !ready2exit)
    {
        index_t ddtop = dstack.top();
        ProductState *p = cstack.mydata[ddtop];
        ProductState *qs  = getNextChild(p);
        if(qs == NULL)
        {
            //没有后继了
            POP();
            continue;
        }
        else
        {
            //还有后继状态
            if(h.search(qs)!=NULL) {
                //存在在哈希表中，表示当前节点已经探索过，并且没有反例路径
                delete qs;
                continue;
            }
            ProductState *existnode = cstack.search(qs);
            if(existnode != NULL)
            {
                UPDATE(existnode);
                delete qs;
                continue;
            }
            if(PUSH(qs)==ERROR)
                break;
//            CPN_Product *existnode = cstack.search(qs);
//            if(existnode != NULL)
//            {
//                UPDATE(existnode);
//                delete qs;
//                continue;
//            }
//            if(h.search(qs) == NULL)
//            {
//                if(PUSH(qs)==ERROR)
//                    break;
//                continue;
//            }
//            delete qs;
        }
    }
}

void CPN_Product_Automata::UPDATE(ProductState *p0) {
    if(p0 == NULL)
        return;
    index_t dtop = dstack.top();
    if(p0->id <= cstack.mydata[dtop]->id)
    {
        if(!astack.empty() && p0->id<=astack.top())
        {
            result = false;
            ready2exit = true;
        }
        cstack.mydata[dtop]->id = p0->id;
    }
}

int CPN_Product_Automata::PUSH(ProductState *p0) {
    p0->id = cstack.toppoint;
    dstack.push(cstack.toppoint);
    if(ba->vertics[p0->BAname_id].accepted)
        astack.push(cstack.toppoint);
    if(cstack.push(p0)==ERROR)
    {
        stack_flag = false;
        return ERROR;
    }
    return OK;
}

void CPN_Product_Automata::POP() {
    index_t p=dstack.pop();
    if(cstack.mydata[p]->id == p)
    {
        //强连通分量的根节点
        while(cstack.toppoint>p)
        {
            ProductState *popitem = cstack.pop();
            h.insert(popitem);
            delete popitem;
        }
    }
    if(!astack.empty() && astack.top()==p){
        astack.pop();
    }
    if(!dstack.empty())
        UPDATE(cstack.mydata[p]);
}

void CPN_Product_Automata::getinitial_status() {
    if(cpnRG->initnode==NULL)
        cpnRG->CPNRGinitnode();

    for(int i=0;i<ba->vex_num;++i)
    {
        if(ba->vertics[i].id==-1)
            continue;
        if(ba->vertics[i].initial)
        {
            if(newisLabel(cpnRG->initnode,i))
            {
                ProductState ps;
                ps.BAname_id = i;
                ps.RGname_ptr = cpnRG->initnode;
                initial_status.push_back(ps);
            }
        }
    }
}

void CPN_Product_Automata::getProduct() {
    detect_mem_thread = thread(&CPN_Product_Automata::detect_memory,this);

    getinitial_status();

    int i=0;
    int end = initial_status.size();
    for(i;i<end;++i)
    {
        ProductState *init = new ProductState;
        init->RGname_ptr = initial_status[i].RGname_ptr;
//        init->RGname_ptr->printMarking();
        init->BAname_id = initial_status[i].BAname_id;
        init->pba = ba->vertics[init->BAname_id].firstarc;
        init->id = 0;
        TCHECK(init);
        if(ready2exit) {
            break;
        }
    }
    ready2exit = true;
    detect_mem_thread.join();
}

void CPN_Product_Automata::detect_memory() {
    for(;;)
    {
        int size=0;
        char filename[64];
        sprintf(filename,"/proc/%d/status",mypid);
        FILE *pf = fopen(filename,"r");
        if(pf == nullptr)
        {
//            cout<<"未能检测到enPAC进程所占内存"<<endl;
            pclose(pf);
        } else{
            char line[128];
            while(fgets(line,128,pf) != nullptr)
            {
                if(strncmp(line,"VmRSS:",6) == 0)
                {
                    int len = strlen(line);
                    const char *p=line;
                    for(;std::isdigit(*p) == false;++p) {}
                    line[len-3]=0;
                    size = atoi(p);
                    break;
                }
            }
            fclose(pf);
            size = size/1024;
            if(100*size/total_mem > 90)
            {
                memory_flag = false;
                ready2exit = true;
                break;
            }

        }
        if(ready2exit)
        {
            break;
        }
        usleep(500000);
    }
}

unsigned short CPN_Product_Automata::ModelChecker(string propertyid, unsigned short each_run_time) {
    signal(SIGALRM, sig_handler);
    alarm(each_run_time);
    timeflag = true;
    memory_flag = true;
    stack_flag = true;
    result = true;
    getProduct();

    string re;
    string slice = cpn->utilizeSlice() && NEXTFREE?"SLICE":"";
    if(timeflag && memory_flag && stack_flag && consistency)
    {
        if(result)
        {
            re="TRUE";
            cout << "FORMULA " << propertyid << " " << re <<" "+slice;
            ret = 1;
        }
        else
        {
            re="FALSE";
            cout << "FORMULA " << propertyid + " " << re <<" "+slice;
            ret = 0;
        }
    }
    else if(!memory_flag)
    {
        cout<<"FORMULA "<<propertyid<<" "<<"CANNOT_COMPUTE-MEMORY";
        ret = -1;
    }
    else if(!stack_flag)
    {
        cout<<"FORMULA "<<propertyid<<" "<<"CANNOT_COMPUTE-STACK";
        ret = -1;
    }
    else if(!consistency) {
        cout<<"FORMULA "<<propertyid<<" "<<"CANNOT_COMPUTE-CONSISTENCY";
        ret = -1;
    }
    else if(!timeflag)
    {
        cout<<"FORMULA "<<propertyid<<" "<<"CANNOT_COMPUTE-TIME";
        ret = -1;
    }
    unsigned short timeleft=alarm(0);
    return (each_run_time-timeleft);
}