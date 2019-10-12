#ifndef TNODE_H
#define TNODE_H
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <condition_variable>
#include <semaphore.h>

class TNode
{
    public:
    TNode(int, std::map<void*, TNode*>*);
    ~TNode();
    ///Node Pointers
    TNode* Parent;
    TNode* Left;
    TNode* Right;
    TNode* Tail;
    ///Internal Data
    bool t;

    int VMax;
    int VMin;
    int Maximum;
    int Minumum;

    int height;

    int Data_Size;
    int D_Min;

    //The map is used over a basic array in order to provide some more workload
    //In an actual use case with fixed node size a sorted array of pairs will also do
    //I might use a object pointer type to a polygonizer so that this has some access time required
    std::map<int, std::string> Data;
    int stored;

    ///Internal Methods that:
    ///Sets VMax/VMin
    void Reconf(int);

    ///Methods that are meant to work with the T-tail modification
    void insert_b(int, std::string dat);
    std::pair<int, std::string> insert_o(int, std::string dat);

    std::map<void*, TNode*>* NODES;


    ///Helper functions
    //
    bool internal();
    //
    void borrow(TNode* &left);
    //
    bool has_tail(TNode* root);
    //
    std::string get(int);
    //
    bool is_space();
    //
    bool can_tail(TNode* root);
    //
    bool can_insert_b();
    //
    int isbound(int);
    //
    bool exists(int);
    //
    void show();
    //This sets the VMax/VMin to the maximum and minimum values contained in the node
    //Better to use this over reconf when values are removed from a node
    //However this is more costly to run
    void actualize(TNode* root);

    ///Delete Helpers

    bool underflow();
    bool half_leaf();
    bool leaf();
    bool can_merge();
    void merge_data();
    bool can_write();
    int adj_height();

    TNode* grab();
    void release(TNode*);

    void cleanup();

    TNode* balance(TNode*);
    int Height(TNode*);
    int Difference(TNode*);
    TNode* ll_rot(TNode*);
    TNode* rr_rot(TNode*);
    TNode* lr_rot(TNode*);
    TNode* rl_rot(TNode*);

    TNode* Reb(TNode*);

    bool Base_leaf;




    ///Locks:
    /*
        Side Note: Depending on the application it may be better to use spinlocks here

        Spinlocks provide the following benefits:
        Less thread initilization/halt time/cost
        Better for operations that have a small access time

        On most system mutex provides a hybrid mutex/spinlock functionality
    */
    ///Use 4 mutex here
    ///Threads will grab the correct mutex when:




    ///We may need to use semaphors
    ///This may seem costly, but both semaphors and mutexes take up 4 bytes of space when empty
    ///These are heavily optimized classes

    ///This needs a condition variable
    //Used by Readers and Searching Writers to exclude rotations
    std::mutex Read;
    std::condition_variable Rea;
    std::mutex T_Read;
    int C_Write;

    bool rotcheck();
    bool inscheck();

    ///This only needs a mutex
    //Used by writers during insertion to exclude other writers
    std::recursive_mutex Ins;

    ///This should be a cond var
    //Used by Writers to claim during search & Rebalance
    std::mutex Write;
    std::condition_variable Wri;

    std::mutex W;
    bool WW;

    ///This needs to be a condition variable
    //Used By Writers to Exclude other Readers and Writers during rebalance
    std::mutex Exclude;
    std::condition_variable Excl;
    std::recursive_mutex T_Exc;
    bool BAL;

    //These are the main functions
    //They may have varients, such as is the case for insert, having two additional cases to deal with the T-Tail modification
    ///Methods
    int Insert(int value, std::string dat, bool overfl);
    int Delete(int value);
    std::string Retrieve(int value, std::string dat);


};
#endif // TNODE_H
