#ifndef THREADEDLOGIC_H
#define THREADEDLOGIC_H
#include <queue>
#include <string>
#include <tuple>
#include <map>
#include <mutex>
#include <random>


class ThreadedLogic
{
    public:
        ThreadedLogic(int seed);
        virtual ~ThreadedLogic();
        int i;

        std::default_random_engine gen;
        std::uniform_int_distribution<int> dist;

        std::map<int, int> Stored;

        std::queue< std::tuple<int, int, std::string>> TO_DO;

        void Add_Data();

        std::mutex Lock;

        void add();

        std::tuple<int, int, std::string> get();



    protected:

    private:
};

#endif // THREADEDLOGIC_H
