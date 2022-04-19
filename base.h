//
// Created by hecong on 2020/11/13.
//

#ifndef ENPAC_CPN_BASE_H
#define ENPAC_CPN_BASE_H
#include "vector"
using namespace std;
template <class T>
bool exist_in(vector<T> &vec,T element)
{
    for(unsigned int i=0;i<vec.size();i++)
        if(element == vec[i])
            return true;

    return false;
}
template <class T>
bool array_equal(const T *a1,const T *a2,int num) {
    for(int i=0;i<num;i++)
    {
        if(a1[i] != a2[i])
            return false;
    }
    return true;
}

template <class T>
bool array_less(const T *a1,const T *a2,int num) {
    for(int i=0;i<num;i++)
    {
        if(a1[i]==a2[i])
            continue;
        else if(a1[i]<a2[i])
            return true;
        else if(a1[i]>a2[i])
            return false;
    }
    return false;
}

template <class T>
bool array_lessorequ(const T *a1,const T *a2,int num) {
    for(int i=0;i<num;i++) {
        if(a1[i] == a2[i])
            continue;
        else if(a1[i] < a2[i])
            return true;
        else if(a1[i] > a2[i])
            return false;
    }
    return true;
}

template <class T>
bool array_greater(const T *a1,const T *a2,int num) {
    for(int i=0;i<num;i++)
    {
        if(a1[i] == a2[i])
            continue;
        else if(a1[i] < a2[i])
            return false;
        else if(a1[i] > a2[i])
            return true;
    }
    return false;
}

template <class T>
bool array_greaterorequ(const T *a1,const T *a2,int num) {
    for(int i=0;i<num;i++)
    {
        if(a1[i] == a2[i])
            continue;
        else if(a1[i] < a2[i])
            return false;
        else if(a1[i] > a2[i])
            return true;
    }
    return true;
}
#endif //ENPAC_CPN_BASE_H
