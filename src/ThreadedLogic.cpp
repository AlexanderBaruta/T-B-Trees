#include "ThreadedLogic.h"
#include <queue>

using namespace std;

ThreadedLogic::ThreadedLogic(int Seed)
{
    i = 0;
    dist = uniform_int_distribution<int>(0, 1000000);


}

ThreadedLogic::~ThreadedLogic()
{
    //dtor
}


void ThreadedLogic::add()
{

    tuple<int, int, string> temp = make_tuple(0, -1, "NOP");
    Lock.lock();
    temp = make_tuple(dist(gen), 0 ,  "String Thing");
    TO_DO.emplace(make_tuple(dist(gen), 0 ,  "String Thing : "));
    Stored.insert( make_pair( std::get<0>(temp), std::get<1>(temp) ));
    i++;
    Lock.unlock();
}

void ThreadedLogic::Add_Data()
{

    tuple<int, int, string> temp = make_tuple(0, -1, "NOP");

    int d = dist(gen);
    d %= 3;

    if(d == 0)
    {
        temp = make_tuple(dist(gen), 0, "String Thing : ");
        i++;
        Stored.insert(make_pair( std::get<0>(temp), std::get<1>(temp)));
        Lock.lock();
        TO_DO.emplace(temp);
        Lock.unlock();
    }
    if(d == 1)
    {
        int v = Stored.size();
        int q = dist(gen) % v;
        std::map<int, int>::iterator it = Stored.begin();
        for(int s = 0; s < q; s++)
        {
            it++;
        }
        temp = make_tuple( it->first, 1, "String Thing : ");
        i++;
        Lock.lock();
        TO_DO.emplace(temp);
        Lock.unlock();
    }
    if(d == 2)
    {
        int v = Stored.size();
        int q = dist(gen) % v;
        std::map<int, int>::iterator it = Stored.begin();
        for(int s = 0; s < q; s++)
        {
            it++;
        }
        temp = make_tuple( it->first, 2, "String Thing : ");
        i++;
        Lock.lock();
        TO_DO.emplace(temp);
        Lock.unlock();
        Stored.erase(it->first);
    }
}


tuple<int, int, string> ThreadedLogic::get()
{
    tuple<int, int, string> temp = make_tuple(0, -1, "NOP");
    Lock.lock();
    if(!TO_DO.empty())
    {
        temp = TO_DO.front();
        TO_DO.pop();
    }
    Lock.unlock();

    return temp;
}



