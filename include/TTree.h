#ifndef TTREE_H
#define TTREE_H
#include "TNode.h"
#include <string>
#include <utility>
#include <vector>
#include <stack>
#include <queue>
#include <deque>
#include <condition_variable>
#include <semaphore.h>




class TTree
    {
    public:
        TTree(int, int);
        ~TTree();

        void Destroy(TNode*);

        TNode* Insert(int value, std::string, int);
        bool Delete(int val, int);
        std::pair<TNode*, bool> Search_To_Insert(int val, std::string dat, TNode* root, TNode* Parent, int);
        std::pair<TNode* , bool> Search_To_Delete(int value, TNode* root, TNode* Parent, int);
        std::pair<std::string, bool> Search(int value, TNode* root, int);

        std::mutex SF;
        std::vector<std::stack<TNode* >> Safe;




        std::map<void*, TNode*> NODES;


        int Height(TNode*);
        int Difference(TNode* root);

        bool pessimistic;




        ///Tree rotation functions
        ///Based off the AVL tree rotation algorithm as described in:
        ///A Study of Index Structures for Main Memory Database Management Systems
        ///Tobin J. Lehman
        ///Michael J. Carey

        TNode* rr_rot(TNode* x);
        TNode* ll_rot(TNode* x);
        TNode* lr_rot(TNode* x);
        TNode* rl_rot(TNode* x);

        TNode* Reb(TNode*);

        ///Tree Balance
        TNode* balance(TNode* x, bool&);
        std::mutex BL;

        ///Tree Output commands
        void inorder(TNode* x);
        void preorder(TNode* x);
        void postorder(TNode* x);


        ///Internal Stuctures related to the testing of the structure
        bool best;
        int Data_Size;

        TNode* Root;

    protected:

    private:

    };



#endif // TTREE_H
