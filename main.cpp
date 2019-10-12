#include <iostream>
#include "TTree.h"
#include <thread>
#include <queue>
#include <random>
#include <mutex>
#include <random>
#include <semaphore.h>
#include "ThreadedLogic.h"
#define debug false
#include <time.h>
#define test false

using namespace std;

int testhread(TTree* T, ThreadedLogic* X, int i)
{
    int value;
    int casecode;
    std::string Ins;
    while(! X->TO_DO.empty())
    {
        #if debug
        cout << "Performing Operations" << endl;
        #endif // debug

        tuple<int, int, string> temp = X->get();

        value = std::get<0>(temp);

        casecode = std::get<1>(temp);

        Ins = std::get<2>(temp);

        #if debug
        cout << "Value : " << value << endl;

        cout << "Casecode : " << casecode << endl;

        cout << "String : " << Ins << endl;
        #endif // debug

        if(casecode == 0)
        {
            T->Insert(value, Ins, i);
        }
        if(casecode == 1)
        {
            T->Search(value, T->Root, i);
        }
        if(casecode == 3)
        {
            T->Delete(value, i);
        }

        //cout << X->i << endl;

        //T->inorder(T->Root);

    }

    return 1;
}



int main()
{


    std::queue<int> Test;
    std::queue<int> Del_Test;
    int T;

    int Node_Size = 20;
    int Initial_Seed = 5734;

    ThreadedLogic* TH = new ThreadedLogic(Initial_Seed);

    int Initial_Push = 25000;
    int Other_Ops = 0;


    int T_NUM = 1;


    #if test
    cout << "What Would You Like The Node Size To Be : " ;
    cin >> Node_Size;
    cout << endl;
    #endif // test

    TTree* X = new TTree(Node_Size, T_NUM);

    #if test
    cout << "What Would You Like The Initial Insert To Be In Size : ";
    cin >> Initial_Push;
    cout << endl;
    #endif // test

/*
    ThreadedLogic Q(750);

    for(int i = 0; i < 100000; i++)
    {
        Q.add();
    }
    cout << "Finished Initial Push" << endl;

    tuple<int, int, string> temp;

    int value;
    int casecode;
    std::string Ins;

    while(! Q.TO_DO.empty())
    {
        temp = Q.get();

        value = std::get<0>(temp);

        casecode = std::get<1>(temp);

        Ins = std::get<2>(temp);

        if(casecode == 0)
        {
            X->Insert(value, Ins, 0);
        }
        if(casecode == 1)
        {
            X->Search(value, X->Root, 0);
        }
        if(casecode == 3)
        {
            X->Delete(value, 0);
        }

    }
    */




    for(int i = 0; i < Initial_Push; i++)
    {

        TH->add();

    }
    cout << "Finished Initial Adds" << endl;

    #if test
    cout << "How Many Other Operations Would You Like To Perform : ";
    cin >> Other_Ops;
    cout << endl;
    #endif // test

    for(int k = 0; k < Other_Ops; k++)
    {
        TH->Add_Data();
        //cout << "A" << endl;
        if( (k % 10000) == 0 )
        {
            cout << k << endl;
        }
    }
    cout << "Finished Other Operations" << endl;

    #if test
    cout << "How Many Threads Would You Like To Use? : " ;
    cin >> T_NUM;
    cout << endl;
    #endif // test

    std::thread VQ[T_NUM];


    for(int i = 0; i < T_NUM; i++)
    {
        VQ[i] = std::thread(testhread, X, TH, i);
    }

    for(int k = 0; k < T_NUM; k++)
    {
        VQ[k].join();
    }


    //X->inorder(X->Root);

    return 0;

}

