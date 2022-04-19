#include <sys/time.h>
#include "CPN_Product.h"

using namespace std;

#define TOTALTOOLTIME 3595

CPN *cpn;
CPN_RG *cpnRG;
bool ready2exit = false;
bool consistency;
bool NEXTFREE = true;
//以MB为单位
short int total_mem;
short int total_swap;
pid_t mypid;

size_t  heap_malloc_total, heap_free_total,mmap_total, mmap_count;
extern void extract_criteria(STNode *n,atomictable &AT, vector<string> &criteria_p,vector<string> &criteria_t,bool &next);
void CreateBA(StateBuchi &ba);
void print_info() {
    struct mallinfo mi = mallinfo();
//    printf("count by itself:\n");
//    printf("\033[31m\theap_malloc_total=%lu heap_free_total=%lu heap_in_use=%lu\n\tmmap_total=%lu mmap_count=%lu\n",
//           heap_malloc_total*1024, heap_free_total*1024, heap_malloc_total*1024-heap_free_total*1024,
//           mmap_total*1024, mmap_count);
//    printf("count by mallinfo:\n");
//    printf("\theap_malloc_total=%lu heap_free_total=%lu heap_in_use=%lu\n\tmmap_total=%lu mmap_count=%lu\n\033[0m",
//           mi.arena, mi.fordblks, mi.uordblks,
//           mi.hblkhd, mi.hblks);
    malloc_stats();
}

void CHECKMEM() {
    mypid = getpid();
    total_mem = 8000;
}

double get_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec / 1000000.0;
}

void CONSTRUCTCPN() {
    CPN *cpnet = new CPN;
    char filename[] = "model.pnml";
    cpnet->getSize(filename);
//    cpnet->printSort();
    cpnet->readPNML(filename);
//    cpnet->printTransVar();
//    cpnet->printVar();

//    cpnet->printCPN();
    timeflag = true;
    cpn = cpnet;
    CPN_RG *graph;
    atomictable AT;
    graph = new CPN_RG(AT);
//    graph->Generate();
//    cout<<"STATE SPACE SIZE: "<<graph->nodecount<<endl;
//    cpnRG = graph;
//    delete graph;
//    delete cpnet;
//    if (1) {
//        cout << "build cpn, exit" << endl;
//        exit(0);
//    }
}

void CHECKLTL(bool cardinality) {
    double starttime, endtime;
    CPN_RG *crg;
    ofstream outresult("boolresult.txt", ios::app);
    SHORTNUM ltlcount = 0;
    SHORTNUM total_left_time = TOTALTOOLTIME;

    int i;
    string propertyid;
    char ff[]="LTLFireability.xml";
    char cc[]="LTLCardinality.xml";

    for(i=1;i<=16;++i) {
//        print_info();

        unsigned short each_run_time;
        unsigned short each_used_time;
        if(ltlcount<6)
            each_run_time=300;
        else{
            each_run_time=total_left_time/(16-ltlcount);
        }
        Syntax_Tree syntaxTree;

        if(cardinality) {
            if(syntaxTree.ParseXML(cc,propertyid,i)==CONSISTENCY_ERROR) {
                cout << "FORMULA " << propertyid << " CANNOT_COMPUTE"<<endl;
                outresult << '?';
                continue;
            }
        }
        else {
            if(syntaxTree.ParseXML(ff,propertyid,i)==CONSISTENCY_ERROR) {
                cout << "FORMULA " << propertyid << " CANNOT_COMPUTE"<<endl;
                outresult << '?';
                continue;
            }

        }

        if(syntaxTree.root->groundtruth!=UNKNOW) {
            cout << "FORMULA " << propertyid << " " << ((syntaxTree.root->groundtruth==TRUE)?"TRUE":"FALSE")<<endl;
            outresult << ((syntaxTree.root->groundtruth==TRUE)?'T':'F');
            continue;
        }
//        cout<<"original tree:"<<endl;
//        syntaxTree.PrintTree();
//        cout<<"-----------------------------------"<<endl;
        syntaxTree.Push_Negation(syntaxTree.root);
//        cout<<"after negation:"<<endl;
//        syntaxTree.PrintTree();
//        cout<<"-----------------------------------"<<endl;
        syntaxTree.SimplifyLTL();
//        cout<<"after simplification:"<<endl;
//        syntaxTree.PrintTree();
//        cout<<"-----------------------------------"<<endl;
        syntaxTree.Universe(syntaxTree.root);
//        cout<<"after universe"<<endl;
//        syntaxTree.PrintTree();
//        cout<<"-----------------------------------"<<endl;

        syntaxTree.Get_DNF(syntaxTree.root);
        syntaxTree.Build_VWAA();
        syntaxTree.VWAA_Simplify();
//        syntaxTree.getVisibleIterms();
//        NEXTFREE = syntaxTree.isNextFree(syntaxTree.root);
//        cpn->computeVis(syntaxTree.visibleIterms,cardinality);

        General GBA;
        GBA.Build_GBA(syntaxTree);
        GBA.Simplify();
        GBA.self_check();

        Buchi BA;
        BA.Build_BA(GBA);
        BA.Simplify();
        BA.self_check();
        BA.Backward_chaining();
//        BA.PrintBuchi("BA.dot");

        StateBuchi SBA;
        SBA.Build_SBA(BA);
        SBA.Simplify();
        SBA.Tarjan();
        SBA.Complete1();
        SBA.Add_heuristic();
        SBA.Complete2();
        SBA.self_check();
//        SBA.PrintStateBuchi();
        SBA.linkAtomics(syntaxTree.AT);

        //extract criteria from syntaxtree
        vector<string> criteria_p,criteria_t;
        bool next= false;
        extract_criteria(syntaxTree.root,syntaxTree.AT,criteria_p,criteria_t,next);
        //slicing CPN
        int slice_t_size=cpn->SLICE(criteria_p,criteria_t);

        if(!next && slice_t_size/cpn->transitioncount<=0.8) {
            cpn->is_slice=true;
        } else{
            cpn->is_slice= false;
        }
        ready2exit = false;
        consistency = true;
        crg = new CPN_RG(syntaxTree.AT);
        cpnRG = crg;

        starttime = get_time();
        CPN_Product_Automata *product = new CPN_Product_Automata(&SBA);
        each_used_time = product->ModelChecker(propertyid,each_run_time);
        endtime = get_time();
        cout<<" "<<crg->nodecount<<" "<<endtime-starttime;
        if(cpn->is_slice)
            cout<<" SLICE"<<endl;
        else
            cout<<endl;
        int ret = product->getresult();
        outresult << (ret == -1 ? '?' : (ret == 0 ? 'F' : 'T'));

        delete product;
        delete crg;
        total_left_time -= each_used_time;
        ltlcount++;
        MallocExtension::instance()->ReleaseFreeMemory();
//        print_info();
    }
    outresult<<endl;
}

void CHECKLTL(bool cardinality,int num) {
    double starttime, endtime;
    CPN_RG *crg;

    SHORTNUM each_run_time = 3000;

    string propertyid;
//    char ff[]="LTLFireability.xml";
//    char cc[]="LTLCardinality.xml";
//    Syntax_Tree syntaxTree;
//    if(cardinality) {
//        if(syntaxTree.ParseXML(cc,propertyid,num)==CONSISTENCY_ERROR) {
//            cout << "FORMULA " << propertyid << " CANNOT_COMPUTE CONSISTENCY_ERROR"<<endl;
//            return;
//        }
//    }
//    else {
//        if(syntaxTree.ParseXML(ff,propertyid,num)==CONSISTENCY_ERROR) {
//            cout << "FORMULA " << propertyid << " CANNOT_COMPUTE CONSISTENCY_ERROR"<<endl;
//            return;
//        }
//    }
//    cout<<"original tree:"<<endl;
//    syntaxTree.PrintTree();
//    cout<<"-----------------------------------"<<endl;
//    syntaxTree.Push_Negation(syntaxTree.root);
//    cout<<"after negation:"<<endl;
//    syntaxTree.PrintTree();
//    cout<<"-----------------------------------"<<endl;
//    syntaxTree.SimplifyLTL();
//    cout<<"after simplification:"<<endl;
//    syntaxTree.PrintTree();
//    cout<<"-----------------------------------"<<endl;
//    syntaxTree.Universe(syntaxTree.root);
//    cout<<"after universe"<<endl;
//    syntaxTree.PrintTree();
//    cout<<"-----------------------------------"<<endl;
//
//    syntaxTree.Get_DNF(syntaxTree.root);
//    syntaxTree.Build_VWAA();
//    syntaxTree.VWAA_Simplify();
//
//    General GBA;
//    GBA.Build_GBA(syntaxTree);
//    GBA.Simplify();
//    GBA.self_check();
//
//    Buchi BA;
//    BA.Build_BA(GBA);
//    BA.Simplify();
//    BA.self_check();
//    BA.Backward_chaining();
//    BA.PrintBuchi("BA.dot");

    StateBuchi SBA;
    CreateBA(SBA);
//    SBA.Build_SBA(BA);
//    SBA.PrintStateBuchi();
//    SBA.Simplify();
//    SBA.Tarjan();
//    SBA.Complete1();
//    SBA.Add_heuristic();
//    SBA.Complete2();
//    SBA.self_check();
//    SBA.PrintStateBuchi();
//    SBA.linkAtomics(syntaxTree.AT);


    ready2exit = false;
    atomictable AT;
    crg = new CPN_RG(AT);
    cpnRG = crg;

    starttime = get_time();
    CPN_Product_Automata *product = new CPN_Product_Automata(&SBA);
    product->ModelChecker(propertyid,each_run_time);
    endtime = get_time();
    cout<<endl;
    cout<<"STATE SPACE SIZE: "<<cpnRG->nodecount<<" RUNTIME:"<<endtime-starttime<<endl;


    delete product;
    delete crg;
}

int main0(int argc, char* argv[]) {
    CHECKMEM();
    cout << "=================================================" << endl;
    cout << "=====This is our tool-EnPAC for the MCC'2021=====" << endl;
    cout << "=================================================" << endl;

    CONSTRUCTCPN();
    string category = argv[1];
    if(category=="LTLCardinality") {
        CHECKLTL(1);
    }
    else {
        CHECKLTL(0);
    }
    delete cpn;
    return 0;
}

int main() {
    CHECKMEM();
//    cout << "=================================================" << endl;
//    cout << "=====This is our tool-EnPAC for the MCC'2021=====" << endl;
//    cout << "=================================================" << endl;

    CONSTRUCTCPN();

    CHECKLTL(1);
    CHECKLTL(0);

    delete cpn;
    return 0;
}

void CreateBA(StateBuchi &ba) {
    ba.vex_num = ba.state_num = 1;
    Vertex &vertex = ba.vertics[0];
    vertex.id = 0;
    vertex.initial = true;
    vertex.label = "true";
    vertex.producers.insert(0);
    vertex.consumers.insert(0);

    ArcNode *p=new ArcNode;
    p->destination=0;
    vertex.firstarc=p;
}