#include "TTree.h"
#include "TNode.h"
#include <iostream>
#include <string>
#include <mutex>
#include <utility>
#include <condition_variable>
#include <semaphore.h>
#define debug false
#define debug2 false
#define concur false

using namespace std;

///T-Tree Functions
TTree::TTree(int D_Size, int T_NUM)
    {
    ///Set the maximum size of the data storage in each node in the tree
    Root = nullptr;
    Data_Size = D_Size;

    Safe.resize(T_NUM);


    }

TTree::~TTree()
    {
    if(Root != nullptr)
        Destroy(Root);
    }


void TTree::Destroy(TNode* root)
    {

    if(root->Left != nullptr)
        Destroy(root->Left);
    if(root->Right != nullptr)
        Destroy(root->Right);
    if(root->Tail != nullptr)
        Destroy(root->Tail);

    delete(root);
    root = nullptr;
    return;

    }

///Inserting into a T-Tree is a bit different than a AVL Tree. The Pseudocode for Insertions is:
/*
Insertion
1)Search for a bounding node for the new value.

2)If such a node exist then check whether there is still space in its data array, if so then insert the
new value and finish。

3)if no space is available in the node, then remove the minimum value from the node's data array
and insert the new value. Now proceed to the node holding the greatest lower bound for the node
that the new value was inserted to. If the removed minimum value still fits in there then add it as the
new maximum value of the node, else create a new right subnode for this node.

///This is the part that will require some thought
///Can check if the subtrees are null before doing the attempted insert
///If they are then we can attempt to insert into the current node
4)If no bounding node was found then insert the value into the last node searched if it still fits
into it. In this case the new value will either become the new minimum or maximum value. If the
value doesn't fit anymore then create a new left or right subtree.

5)If a new node was added then the tree might need to be rebalanced, as described below.
*/
///Pseudocode thanks to:
/*
Study and Optimization of T-tree Index in Main Memory Database
Fengdong.Sun, Quan.Guo, Lan.Wang
Department Of Computer Science and Technology , DaLian Neusoft University of
Information, Dalian, China
*/



//Important Note:
//Any lock operation preceded by a:
/*P*/
//Commrent is intended for the pessimistic approach
//And any preceded by a
/*O*/
//Is intended for the Optimistic approach


///The bool in the return will be used to decide if the pointers need to be cascaded/tree balanced

std::pair<TNode*, bool> TTree::Search_To_Insert(int val, std::string dat, TNode* root, TNode* Par, int i)
    {



#if debug
    std::cout << "Searching" << std::endl;
    std::cout << val << std::endl;
#endif // debug


    ///This will be used in the case where we need to make a new node, either on root creation, or on similar cases
    if(root == nullptr)
        {
        //Create the root node
        root = new TNode(Data_Size, &NODES);

        root->T_Read.lock();

        //NODES.insert(std::make_pair(root, root));

        root->Base_leaf = true;

        ///Insert the value into the array
        root->Insert(val, dat, false);
        ///Set all the pointers to null
        root->Left = nullptr;
        root->Right = nullptr;
        root->Parent = Par;
        root->Tail = nullptr;

        root->C_Write = 0;


#if debug
        std::cout << "Created New Node" << std::endl;
        std::cout << root << std::endl;
#endif // debug

        root->T_Read.unlock();

        return std::make_pair(root, false);
        }

#if concur
    std::cout << "Waiting on W.lock()" << std::endl;
#endif // debug

    root->W.lock();
    root->WW = true;
    root->W.unlock();

#if concur
    std::cout << "Got W.lock()" << std::endl;
#endif // debug

#if concur
    std::cout << "Waiting on T_Read.lock()" << std::endl;
#endif // debug



#if concur
    std::cout << "Got T_Read.lock()" << std::endl;
#endif // debug

    std::pair<TNode*, bool> temp_node;



#if debug
    std::cout << "Max :" << root->VMax << std::endl << "Min :" << root->VMin << std::endl;
#endif

#if concur
    std::cout << "Waiting on Insert.lock()" << std::endl;
#endif // concur

    root->Ins.lock();

    root->T_Read.lock();
    root->C_Write++;
    root->T_Read.unlock();

#if concur
    std::cout << "Finished Waiting on Insert.lock()" << std::endl;
#endif // concur

    //Temporary values
    std::pair<int, std::string> temp = std::make_pair(0, "");
    int temp_int;
    std::string temp_string;

    ///Try to insert the value into this node
    ///Step (1)
    if( (val <= root->VMax ) && (val >= root->VMin) )
        {

#if debug
        std::cout << "Reached insert into root" << std::endl;
        std::cout << root->stored << std::endl;
        std::cout << "root has tail :" << root->has_tail(root) << std::endl;
#endif

        ///If there is space inside this node insert the value
        ///Step (2)
        if(root->is_space())
            {

            root->Insert(val, dat, false);

            root->Ins.unlock();

#if concur
            std::cout << "Insert Unlocked " << std::endl;
#endif // concur

#if concur
            std::cout << "Waiting on T_Read.lock()" << std::endl;
#endif // debug

root->T_Read.lock();
    root->C_Write--;
    root->Rea.notify_one();
    root->T_Read.unlock();


#if concur
            std::cout << "Got T_Read.lock()" << std::endl;
#endif // debug


#if concur
            std::cout << "Waiting on W.lock() on insert" << std::endl;
#endif // concur
            root->W.lock();
            root->WW = false;
            root->Wri.notify_one();
#if concur
            std::cout << "Wri.notify_one()" << std::endl;
#endif // concur
            root->W.unlock();
#if concur
            std::cout << "Finished Waiting on W.lock() on insert" << std::endl;
#endif // concur

#if debug
            std::cout << "Inserted into node with space" << std::endl;
#endif

            //Return
            return std::make_pair(root, false);
            }



        ///Otherwise if the node has a tail, insert the value into the combined space
        ///If the node does not have a tail, create it
        ///This case will cover what happens if the tail does not have enough space for the value
        else if(root->has_tail(root))
            {
            ///If this can insert into the combined space without overflowing
            if(root->can_insert_b())
                {

                root->insert_b(val, dat);

                root->Ins.unlock();

                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();

                root->W.lock();
                root->WW = false;
                root->Wri.notify_one();
                root->W.unlock();

                //Safe[i].push_back(root);

#if debug
                std::cout << "Inserted into tail" << std::endl;
#endif

                //Return
                return std::make_pair(root, false);
                }
            else
                {



                ///Get the overflowed value
                temp = root->insert_o(val, dat);

                root->Ins.unlock();

                temp_int = std::get<0>(temp);
                temp_string = std::get<1>(temp);

#if debug
                std::cout << "Value was overflowed" << std::endl;
                std::cout << "Val: " << temp_int << " String: " << temp_string << std::endl;
#endif
                ///Step (3)

#if debug
                std::cout << temp_int << std::endl;
#endif

                ///if the returned key is the same as the attempted insert we know that we have updated the value
                if(temp_int != val)
                    {

#if debug
                    std::cout << "Value was non_identical" << std::endl;
#endif // debug

                    ///If the value will fit in the left subtree:
                    ///The left subtree is empty
                    if(root->Left == nullptr)
                        {
#if debug
                        std::cout << "Left Null OO" << std::endl;
#endif // debug

                        //root->Ins.unlock();

                        root->Base_leaf = false;
                        temp_node = Search_To_Insert(temp_int, temp_string, nullptr, root, i);
                        root->Left = temp_node.first;

                        //Unlock
                        //root->Write.unlock();

                        root->T_Read.lock();
                        root->C_Write--;
                        root->Rea.notify_one();
                        root->T_Read.unlock();

                        root->cleanup();


                        bool q;
                        root = balance(root, q);






#if debug
                        std::cout << "Left child was null, rebalanced" << std::endl;
#endif // debug

                        //Return
                        return std::make_pair(root, true);

                        }
                    ///Insert into Left. We cannot overflow the minimum value into the right subtree by definition
                    else
                        {

                        //root->Ins.unlock();

#if concur
                        std::cout << "Waiting on W.lock() in overflow" << std::endl;
#endif // concur

                        root->Left->W.lock();
                        if(root->Left->WW)
                            {
                            root->Left->W.unlock();

#if concur
                            std::cout << "Released W.lock() in overflow" << std::endl;
#endif // concur


                            std::unique_lock<std::mutex> lk_Wr_L(root->Left->Write);
                            root->Left->Wri.wait(lk_Wr_L, std::bind(&TNode::can_write, root->Left));
                            }
                        else
                            {
                            root->Left->W.unlock();


#if concur
                            std::cout << "Released W.lock() in overflow" << std::endl;
#endif // concur
                            }



#if concur
                        std::cout << "Waiting on T_Exc.lock() in overflow" << std::endl;
#endif // concur
                        root->Left->T_Exc.lock();
                        if(root->Left->BAL)
                            {
                            root->Left->T_Exc.unlock();

#if concur
                            std::cout << "Released T_Exc.lock() in overflow" << std::endl;

                            std::cout << "Waiting on Excl.wait in overflow" << std::endl;
#endif // concur


                            std::unique_lock<std::mutex> lk_Ex_l(root->Left->Exclude);
                            root->Left->Excl.wait(lk_Ex_l, std::bind(&TNode::inscheck, root->Left));

#if concur
                            std::cout << "Finished waiting on Excl.wait in overflow" << std::endl;
#endif // concur

                            }
                        else
                            {
                            root->Left->T_Exc.unlock();
#if concur
                            std::cout << "Released T_Exc.lock() in overflow" << std::endl;
#endif // concur
                            }


                        temp_node = Search_To_Insert(temp_int, temp_string, root->Left, root, i);

                        root->Left = temp_node.first;

#if concur
                        std::cout << "Waiting on T_Read.lock() in overflow after Left Overflow insert" << std::endl;
#endif // concur
                        root->T_Read.lock();
                        root->C_Write--;
                        root->Rea.notify_one();
                        root->T_Read.unlock();
#if concur
                        std::cout << "Finished Waiting on T_Read.lock() in overflow after Left Overflow insert" << std::endl;
#endif // concur


                        root->cleanup();

                        bool q;
                        root = balance(root, q);



#if debug
                        std::cout << "Overflowed into Left subtree" << std::endl;
#endif // debug

                        return std::make_pair(root, temp_node.second);
                        }

                    }
                else
                    {


                    ///The value associated with the key has been updated
#if debug
                    std::cout << "Updated existing value" << std::endl;
#endif // debug



#if concur
                    std::cout << "Waiting on T_Read.lock() in overflow after Update" << std::endl;
#endif // concur
                    root->T_Read.lock();
                    root->C_Write--;
                    root->Rea.notify_one();
                    root->T_Read.unlock();
#if concur
                    std::cout << "Finished Waiting on T_Read.lock() in overflow after Update" << std::endl;
#endif // concur

                    //Unlock the root node
#if concur
                    std::cout << "Waiting on W.lock() in overflow after Update" << std::endl;
#endif // concur
                    root->W.lock();
                    root->WW = false;
                    root->Wri.notify_one();
#if concur
                    std::cout << "Wri.notify_one()" << std::endl;
#endif // concur
                    root->W.unlock();
#if concur
                    std::cout << "Finished Waiting on W.lock() in overflow after Update" << std::endl;
#endif // concur

                    ///The value associated with the key has been updated
                    return std::make_pair(root, false);
                    }
                }
            }
        else if(root->can_tail(root))
            {


            root->insert_b(val, dat);

            root->Ins.unlock();

#if debug
            std::cout << "Made tail and inserted into tail on condition int between vmax and vmin" << std::endl;
#endif // debug

#if concur
            std::cout << "Waiting on T_read.lock() after tail creation" << std::endl;
#endif // concur


            root->T_Read.lock();
            root->C_Write--;
            root->Rea.notify_one();
            root->T_Read.unlock();

#if concur
            std::cout << "Finished Waiting on T_read.lock() after tail creation" << std::endl;
#endif // concur


#if concur
            std::cout << "Waiting on W.lock() after tail creation" << std::endl;
#endif // concur
            root->W.lock();
            root->WW = false;
            root->Wri.notify_one();
#if concur
            std::cout << "Wri.notify_one()" << std::endl;
#endif // concur
            root->W.unlock();
#if concur
            std::cout << "Finished Waiting on W.lock() after tail creation" << std::endl;
#endif // concur

            return std::make_pair(root, false);
            }
        else
            {
            std::cout << "Reached End of Insert Into Node: Something went wrong" << std::endl;
            }
        }

///Search the left subtree
//val < root->VMin
    else if(val < root->VMin)
        {

#if debug
        std::cout << "Reached Insert into left subtree" << std::endl;
#endif // debug

        ///If the Left subtree does not exist
        if(root->Left == nullptr)
            {
            ///If there is still room inside the current node+tail
            if(root->is_space())
                {
                root->Insert(val, dat, false);

                root->Ins.unlock();




#if debug
                std::cout << "Inserted into root" << std::endl;
#endif // debug

#if concur
                std::cout << "Waiting on T_Read.lock() after insert into root" << std::endl;
#endif // concur
                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();
#if concur
                std::cout << "Finished Waiting on T_Read.lock() after insert into root" << std::endl;
#endif // concur

#if concur
                std::cout << "Waiting on W.lock() after tail creation" << std::endl;
#endif // concur
                root->W.lock();
                root->WW = false;
                root->Wri.notify_one();
#if concur
                std::cout << "Wri.notify_one()" << std::endl;
#endif // concur
                root->W.unlock();
#if concur
                std::cout << "Finished Waiting on W.lock() in overflow after Update" << std::endl;
#endif // concur

                return std::make_pair(root, false);
                }
            ///If the tail exists
            else if(root->has_tail(root))
                {
                if(root->can_insert_b())
                    {
                    root->insert_b(val, dat);

                    root->Ins.unlock();



#if debug
                    std::cout << "Inserted into tail" << std::endl;
#endif

#if concur
                    std::cout << "Waiting on T_Read.lock() after insert into tail" << std::endl;
#endif // concur
                    root->T_Read.lock();
                    root->C_Write--;
                    root->Rea.notify_one();
                    root->T_Read.unlock();
#if concur
                    std::cout << "Finished Waiting on T_Read.lock() after insert into tail" << std::endl;
#endif // concur

                    //Unlock the root node

#if concur
                    std::cout << "Waiting on W.lock() after tail insertion" << std::endl;
#endif // concur
                    root->W.lock();
                    root->WW = false;
                    root->Wri.notify_one();
#if concur
                    std::cout << "Wri.notify_one()" << std::endl;
#endif // concur
                    root->W.unlock();
#if concur
                    std::cout << "Finished Waiting on W.lock() after tain insertion" << std::endl;
#endif // concur

                    return std::make_pair(root, false);
                    }
                else
                    {

#if debug
                    std::cout << "Inserted into newly created left subtree" << std::endl;
#endif // debug

                    root->Base_leaf = false;

                    root->Ins.unlock();

                    temp_node = Search_To_Insert(val, dat, nullptr, root, i);

                    root->Left = temp_node.first;
                    root->cleanup();


#if concur
                    std::cout << "Waiting on T_Read.lock() after insert newly created left subtree" << std::endl;
#endif // concur
                    root->T_Read.lock();
                    root->C_Write--;
                    root->Rea.notify_one();
                    root->T_Read.unlock();
#if concur
                    std::cout << "Finished Waiting on T_Read.lock() after insert newly created left subtree" << std::endl;
#endif // concur

                    //Unlock the root node
                    //root->Write.unlock();

                    bool q;
                    root = balance(root, q);



                    return std::make_pair(root, true);

                    }
                }
            ///Otherwise generate the tail node and insert the minimum value into it
            ///This is done after the existance of the tail is checked because this case will happen less frequently
            ///Slight runtime optimization
            else if(root->can_tail(root))
                {
                root->insert_b(val, dat);
                root->Ins.unlock();

#if debug
                std::cout << "Made tail and inserted into tail" << std::endl;
#endif // debug


#if concur
                std::cout << "Waiting on T_Read.lock() after insert newly created tail" << std::endl;
#endif // concur
                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();
#if concur
                std::cout << "Finished Waiting on T_Read.lock() after insert newly created tail" << std::endl;
#endif // concur


                //Unlock the root node
                //root->Write.unlock();

                bool q;
                root = balance(root, q);




                return std::make_pair(root, false);
                }
            else
                {
                root->Ins.unlock();
                temp_node = Search_To_Insert(val, dat, nullptr, root, i);

                root->Base_leaf = false;

                root->Left = temp_node.first;

#if concur
                std::cout << "Waiting on T_Read.lock() after insert newly created left subtree" << std::endl;
#endif // concur
                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();
#if concur
                std::cout << "Finished Waiting on T_Read.lock() after insert newly created left subtree" << std::endl;
#endif // concur


                //root->cleanup();

                //Unlock the root node
                //root->Write.unlock();

                bool q;
                root = balance(root, q);

                if(q == false)
                    {

                    }





#if debug
                std::cout << "Inserted into newly created left subtree" << std::endl;
#endif // debug

                return std::make_pair(root, true);

                }
            }
        else
            {

#if debug
            std::cout << "Searching Left Subtree" << std::endl;
#endif // debug

            //root->cleanup();

            root->Ins.unlock();


            root->Left->W.lock();
            if(root->Left->WW)
                {
                root->Left->W.unlock();

                #if concur
                std::cout << "Waiting on Wri.wait in attempt to search left subtree" << std::endl;
                #endif // concur

                std::unique_lock<std::mutex> lk_Wr_L(root->Left->Write);
                root->Left->Wri.wait(lk_Wr_L, std::bind(&TNode::can_write, root->Left));

                #if concur
                std::cout << "Finished Waiting on Wri.wait in attempt to search left subtree" << std::endl;
                #endif // concur
                }
            else
                {
                root->Left->W.unlock();
                }



            root->Left->T_Exc.lock();
            if(root->Left->BAL)
                {
#if concur
                std::cout << "Waiting on Excl.wait on search left subtree" << i << std::endl;
#endif // debug

                root->Left->T_Exc.unlock();
                std::unique_lock<std::mutex> lk_Ex_l(root->Left->Exclude);
                root->Left->Excl.wait(lk_Ex_l, std::bind(&TNode::inscheck, root->Left));

                #if concur
                std::cout << "Waiting on Excl.wait on search left subtree" << std::endl;
                #endif // concur

                }
            root->Left->T_Exc.unlock();



            // root->Left->Write.lock();

            Safe[i].push(root->Left);

            temp_node = Search_To_Insert(val, dat, root->Left, root, i);

            root->Left = temp_node.first;

#if debug
            std::cout << root->Left << " " << root->Right << std::endl;
#endif // debug

            root->cleanup();

            #if concur
                std::cout << "Waiting on T_Read.lock() after search left subtree" << std::endl;
#endif // concur
                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();
#if concur
                std::cout << "Finished Waiting on T_Read.lock() after search left subtree" << std::endl;
#endif // concur


            //root->Write.unlock();

            bool q;
            root = balance(root, q);



            return std::make_pair(root, true);

            }
        }


///Search the right subtree
//val > root->VMax
    else if(val > root->VMax)
        {
#if debug
        std::cout << "Inserting into right subtree" << std::endl;
#endif

        ///If the Right subtree does not exist
        if(root->Right == nullptr)
            {
#if debug
            std::cout << "Right root is Null" << std::endl;
#endif // debug

            ///If there is still room inside the current node+tail
            if(root->is_space())
                {
                root->Insert(val, dat, false);

                root->Ins.unlock();

                            #if concur
                std::cout << "Waiting on T_Read.lock() after regular insert" << std::endl;
#endif // concur
                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();
#if concur
                std::cout << "Finished Waiting on T_Read.lock() after regular insert" << std::endl;
#endif // concur

#if debug
                std::cout << "Inserted into parent since it still has space" << std::endl;
#endif

                root->W.lock();
                root->WW = false;
                root->Wri.notify_one();
                root->W.unlock();

                return std::make_pair(root, false);
                }
            ///If the tail exists
            else if(root->has_tail(root))
                {
#if debug
                std::cout << "Root has tail" << std::endl;
#endif // debug


                if(root->can_insert_b())
                    {
#if debug
                    std::cout << "Attempting to insert into tail" << std::endl;
#endif // debug

                    root->insert_b(val, dat);

                    root->Ins.unlock();

                                #if concur
                std::cout << "Waiting on T_Read.lock() after insert into tail" << std::endl;
#endif // concur
                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();
#if concur
                std::cout << "Finished Waiting on T_Read.lock() after insert into tail" << std::endl;
#endif // concur

                    root->cleanup();

                    //root->Write.unlock();

                    bool q;
                    root = balance(root, q);



#if debug
                    std::cout << "Inserted into tail" << std::endl;
#endif

                    return std::make_pair(root, false);
                    }
                else
                    {
                    root->Ins.unlock();
                    temp_node = Search_To_Insert(val, dat, nullptr, root, i);
                    root->Base_leaf = false;
                    root->Right = temp_node.first;

                    //Unlock
                    //root->Write.unlock();

#if concur
                std::cout << "Waiting on T_Read.lock() after insert into newly created right subtree" << std::endl;
#endif // concur
                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();
#if concur
                std::cout << "Finished Waiting on T_Read.lock() after insert into newly created right subtree" << std::endl;
#endif // concur

                    root->cleanup();

                    //root->Write.unlock();

                    bool q;
                    root = balance(root, q);



#if debug
                    std::cout << "Inserted into right subtree after creating right subtree" << std::endl;
#endif

                    return std::make_pair(root, true);
                    }
                }
            ///Otherwise generate the tail node and insert the minimum value into it
            ///This is done after the existance of the tail is checked because this case will happen less frequently
            ///Slight runtime optimization
            else if(root->can_tail(root))
                {
                root->insert_b(val, dat);
                root->Ins.unlock();

                //Unlock the root node
                // root->Write.unlock();

#if debug
                std::cout << "inserted into tail because node was not full on right subtree insert" << std::endl;
#endif

                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();


                root->cleanup();

                //root->Write.unlock();

                bool q;
                root = balance(root, q);



                return std::make_pair(root, false);
                }
            else
                {
                root->Ins.unlock();

                //TNode* tempp = new TNode(Data_Size, &NODES);

                temp_node = Search_To_Insert(val, dat, nullptr, root, i);

                root->Base_leaf = false;

                root->Right = temp_node.first;

                //Unlock
                //root->Write.unlock();

                root->T_Read.lock();
                root->C_Write--;
                root->Rea.notify_one();
                root->T_Read.unlock();

                root->cleanup();

                //root->Write.unlock();

                bool q;
                root = balance(root, q);



                return std::make_pair(root, true);

                }
            }
        else
            {

#if debug
            std::cout << "Right Root is not null" << std::endl;
#endif

            root->Ins.unlock();

            #if concur
            std::cout << "Trying to Get Right->W.lock() " << i <<std::endl;
            #endif // concur

            root->Right->W.lock();
            if(root->Right->WW)
                {
                root->Right->W.unlock();

                #if concur
                std::cout << "Got Right->W.lock()" << std::endl;

                std::cout << "Waiting on Wri.wait() on search right subtree " << i<< std::endl;
                #endif // concur

                std::unique_lock<std::mutex> lk_Wr_R(root->Right->Write);
                root->Right->Wri.wait(lk_Wr_R, std::bind(&TNode::can_write, root->Right));

                #if concur
                std::cout << "Finished waiting on Wri.wait() on search right subtree "<<i << std::endl;
                #endif // concur

                }
            else
                {
                root->Right->W.unlock();
                #if concur
                std::cout << "Got Right->W.lock()" << std::endl;
                #endif // concur
                }



            root->Right->T_Exc.lock();
            if(root->Right->BAL)
                {
#if concur
                std::cout << "ON REGULAR INSERT INTO RIGHT WAITING ON EXCL " << i << std::endl;
#endif // debug

                root->Right->T_Exc.unlock();
                std::unique_lock<std::mutex> lk_Ex_r(root->Right->Exclude);
                root->Right->Excl.wait(lk_Ex_r, std::bind(&TNode::inscheck, root->Right));
#if concur
                std::cout << "ON REGULAR INSERT INTO RIGHT FINISHED WAITING ON EXCL " << i << std::endl;
#endif // debug

                }
            root->Right->T_Exc.unlock();


#if debug
            std::cout << "Locked Right Root Write" << std::endl;
#endif // debug

            temp_node = Search_To_Insert(val, dat, root->Right, root, i);

            root->Right = temp_node.first;



#if debug
            std::cout << "Right Root is :" << root->Right << std::endl;
#endif

            //Unlock
            //root->Write.unlock();

            root->T_Read.lock();
            root->C_Write--;
            root->Rea.notify_one();
            root->T_Read.unlock();

            root->cleanup();

            //root->Write.unlock();

            bool q;
            root = balance(root, q);








#if debug
            std::cout << "Balancing on right subtree" << std::endl;
            std::cout << "Right Root is :" << root->Right << std::endl;
#endif

            return std::make_pair(root, true);

            }
        }


    //Make sure if things somehow manage to reach the end we clean up
    //root->Write.unlock();
    ///If everything fails, well then, I guess I have some debugging to do
    cout << "Something went wrong on value Insert in TTree" << endl;
    root->Ins.unlock();
    root->T_Read.lock();
    root->C_Write--;
    root->Rea.notify_one();
    root->T_Read.unlock();

    //root->Write.unlock();

    bool q;
    root = balance(root, q);

    root->W.lock();
    root->WW = false;
    root->Wri.notify_one();
    root->W.unlock();

    return std::make_pair(root, false);
    }

//Pseudocode for Deletion
/*
(3)Deletion
1) Search for the node that bounds the delete value. Search for the delete value within this node,
reporting an error and stopping if it is not found.
2) If the delete will not cause an underflow (if the node has more than the minimum allowable
number of entries prior to the delete), then simply delete the value and stop. Else, if this is an
internal node, then delete the value and borrow the greatest lower bound of this node from a leaf or
half-leaf to bring this node’s element count back up to the minimum. Else, this is a leaf or a
half-leaf, so just delete the element. (Leaves are permitted to underflow, and half-leaves are handled
in step(3).
3) If the node is a half-leaf and can be merged with a leaf ,coalesce the two nodes into one node
(a leaf) and discard the other node. Proceed to step (5).
4) If the current node (a leaf) is not empty, then stop. Else, free the node and proceed to step (5)
to rebalance the tree.
5) For every node along the path from the leaf up to the root, if the two subtrees of the node
differ in height by more than one ,perform a rotation operation. Since a rotation at one node may
create an imbalance for a node higher up in the tree, balance-checking for deletion must examine all
of search path until a node of even balance is discovered.
*/
///Pseudocode thanks to:
/*
Study and Optimization of T-tree Index in Main Memory Database
Fengdong.Sun, Quan.Guo, Lan.Wang
Department Of Computer Science and Technology , DaLian Neusoft University of
Information, Dalian, China
*/


std::pair<TNode*, bool> TTree::Search_To_Delete(int val, TNode* root, TNode* Parent, int i)
    {
    std::pair<TNode*, bool> temp_node;

    if(NODES.count(root) > 0)
        {
        //std::cout << "Node exists in List of Nodes" << std::endl;


        if(root == nullptr)
            {
            return std::make_pair(nullptr, false);
            }

        root->T_Read.lock();
        root->C_Write++;
        root->T_Read.unlock();



        //std::cout << root->VMin << std::endl;

        if(val < root->VMin)
            {



            temp_node = Search_To_Delete(val, root->Left, root, i);
            root->Left = temp_node.first;

            root->cleanup();

            bool q;
            root = balance(root, q);

            return std::make_pair(root, true);


            }
        else if(val > root->VMax)
            {
            temp_node = Search_To_Delete(val, root->Right, root, i);
            root->Right = temp_node.first;

            root->cleanup();

            bool q;
            root = balance(root, q);


            return std::make_pair(root, true);

            }
        else
            {
            if(root->exists(val))
                {
                if(root == Root)
                    {
                    //std::cout << "TRYING TO DELETE MAIN ROOT" << std::endl;
                    }

#if debug
                std::cout << Height(root) << std::endl;
#endif // debug
                root->Delete(val);

                root->cleanup();

                bool q;
                root = balance(root, q);

                //This will check if the value is below the minimum amount allowed
                if(root->underflow())
                    {
                    if(root->internal())
                        {
                        //Lock the left subtree
                        //root->Left->Read.lock();

                        //We know that this node has both a left subtree and a right subtree
                        root->borrow(root->Left);

                        //Unlock
                        //root->Left->Read.unlock();
                        }
                    else if(root->half_leaf())
                        {
                        if(root->can_merge())
                            {

                            root->merge_data();

                            root->cleanup();

                            bool q;
                            root = balance(root, q);

                            return std::make_pair(root, true);
                            }
                        }
                    else
                        {
                        if(root->stored == 0)
                            {
                            //If the root is empty, delete it and return nullptr
                            NODES.erase(root);
                            delete(root);
                            return std::make_pair(nullptr, true);
                            }
                        }
                    }


                root->cleanup();

                //root = balance(root);

                return std::make_pair(root, true);
                }
            else
                {
                root->cleanup();

                bool q;
                root = balance(root, q);
#if debug
                std::cout << "Data to delete not found" << std::endl;
#endif // debug
                return std::make_pair(root, false);
                }
            }
        }
    return std::make_pair(root, true);
    }




std::pair<std::string, bool> TTree::Search(int val, TNode* root, int i)
    {
#if debug2
    std::cout << "Value: " << val << " root: " << root << endl;
#endif // debug2

    std::pair<std::string, bool> val_ret;

    if(root == nullptr)
        {

#if debug2
        std::cout << "Search is false" << std::endl;
#endif // debug2

        return std::make_pair("", false);
        }

    root->T_Read.lock();
    root->C_Write++;
    root->T_Read.unlock();

#if debug2
    std::cout << root->VMax << " " << root->VMin << std::endl;
#endif // debug2

    //If this is the root node of the entire tree
    if(root->Parent == nullptr)
        {
#if debug2
        std::cout << "Parent is null" << std::endl;
#endif

        //root->Read.lock();
        }

    root->Read.try_lock();

    //Cannot claim the mutex of a nonexistant node

    ///We need to chain parent/child locks in order to make searching thread safe

    if(val > root->VMax)
        {

#if debug2
        std::cout << "Searching Right Subtree :" << root->Right << std::endl;
#endif // debug2

        if(root->Right != nullptr)
            {

            root->Right->T_Exc.lock();
            if(root->Right->BAL)
                {
                root->Right->T_Exc.unlock();
                std::unique_lock<std::mutex> lk_Ex_r(root->Right->Exclude);
                root->Right->Excl.wait(lk_Ex_r, std::bind(&TNode::inscheck, root->Right));
                }
            root->Right->T_Exc.unlock();

            }

        val_ret = Search(val, root->Right, i);

        root->T_Read.lock();
        root->C_Write--;
        root->Rea.notify_one();
        root->T_Read.unlock();

        root->Rea.notify_one();

        return val_ret;





        }
    else if(val < root->VMin)
        {

#if debug2
        std::cout << "Searching Left Subtree :" << root->Left << std::endl;
#endif // debug2

        if(root->Left != nullptr)
            {

            root->Left->T_Exc.lock();
            if(root->Left->BAL)
                {
                root->Left->T_Exc.unlock();
                std::unique_lock<std::mutex> lk_Ex_l(root->Left->Exclude);
                root->Left->Excl.wait(lk_Ex_l, std::bind(&TNode::inscheck, root->Left));
                }
            root->Left->T_Exc.unlock();

            }

        val_ret = Search(val, root->Left, i);


        root->T_Read.lock();
        root->C_Write--;
        root->Rea.notify_one();
        root->T_Read.unlock();

        root->Rea.notify_one();

        return val_ret;


        }

    else if(root->exists(val))
        {

#if debug
        std::cout << "Trying to get Value in Root" << endl;
#endif // debug

        std::string temp = root->get(val);

#if debug2
        std::cout << "Value Exists in Node" << std::endl;
        std::cout <<  root->get(val) << std::endl;
#endif // debug

        //root->Read.lock();

        root->T_Read.lock();
        root->C_Write--;
        root->Rea.notify_one();
        root->T_Read.unlock();

        return std::make_pair(temp, true);
        }
    else
        {
        //root->Read.unlock();
        //std::cout << "REACHED END OF FUNCTION 1" << endl;

        root->T_Read.lock();
        root->C_Write--;
        root->Rea.notify_one();
        root->T_Read.unlock();

        return std::make_pair("END OF FUNCTION", false);
        }

    //root->Read.unlock();
    //std::cout << "REACHED END OF FUNCTION" << endl;

    root->T_Read.lock();
    root->C_Write--;
    root->Rea.notify_one();
    root->T_Read.unlock();

    return std::make_pair("END OF FUNCTION", false);
    }


TNode* TTree::balance(TNode* root, bool &released)
    {

    //BL.lock();
#if debug
    std::cout << "Balancing:" << std::endl;
#endif
    if(root == nullptr)
        {
       // BL.unlock();
        released = false;
        return nullptr;
        }

    int bal = Difference(root);

#if debug
    std::cout << "Balance :" <<  bal << std::endl;
#endif


    TNode* temp;
    //This will set the lock

    if(bal > 1 || bal < -1)
        {

        }
    else
        {
        //BL.unlock();

#if concur
        std::cout << "Waiting on W.lock() after non-balance" << std::endl;
#endif // concur
        root->W.lock();
        root->WW = false;
        root->Wri.notify_one();
#if concur
        std::cout << "Wri.notify_one()" << std::endl;
#endif // concur
        root->W.unlock();
#if concur
        std::cout << "Finished Waiting on W.lock() after non-balance" << std::endl;
#endif // concur
        released = true;
        return root;
        }

    temp = root->grab();

#if debug
    std::cout << "Grabbed" << std::endl;
#endif // debug

    if(bal > 1)
        {
        released = true;
        if(Difference(root->Left) > 0)
            {
            root = ll_rot(root);
            }
        else
            {
            root = ll_rot(root);
            }
        }
    else if(bal < -1)
        {
        released = true;
        if(Difference(root->Right) > 0)
            {
            root = rl_rot(root);
            }
        else
            {
            root = rr_rot(root);
            }
        }


    //This will release the locks
    temp->release(temp);
    root->release(root);

#if concur
    std::cout << "Waiting on W.lock() in balance" << std::endl;
#endif // concur
    temp->W.lock();
    temp->WW = false;
    temp->Wri.notify_one();
#if concur
    std::cout << "Wri.notify_one()" << std::endl;
#endif // concur
    temp->W.unlock();
#if concur
    std::cout << "Finished Waiting on W.lock() in balance" << std::endl;
#endif // concur
    //BL.unlock();


#if debug
    std::cout << root << std::endl;
#endif // debug

    return root;
    }




//Algorithm for finding the height of a node
int TTree::Height(TNode* root)
    {
    if(root == nullptr)
        {
        return 0;
        }
    int h = 0;
    if(root != nullptr)
        {
        int left_height = 0;
        int right_height = 0;



        if(root->Base_leaf)
            {
            return 0;
            }

        if(root->Left != nullptr)
            {
            root->Left->T_Exc.lock();
            if(root->Left->BAL)
                {
                //std::cout << "IN LEFT ROOT NOT NULL WAITING ON EXCL " << i << std::endl;

                root->Left->T_Exc.unlock();
                std::unique_lock<std::mutex> lk_rlh(root->Left->Exclude);
                root->Left->Excl.wait(lk_rlh, std::bind(&TNode::inscheck, root->Left));
                left_height = Height(root->Left);
                }
            else
                {
                root->Left->T_Exc.unlock();
                }
            }
        else
            {
            left_height = 0;
            }



        if(root->Right != nullptr)
            {
            root->Right->T_Exc.lock();
            if(root->Right->BAL)
                {
                //std::cout << "IN LEFT ROOT NOT NULL WAITING ON EXCL " << i << std::endl;

                root->Right->T_Exc.unlock();
                std::unique_lock<std::mutex> lk_rlh(root->Right->Exclude);
                root->Right->Excl.wait(lk_rlh, std::bind(&TNode::inscheck, root->Right));
                right_height = Height(root->Right);
                }
            else
                {
                root->Right->T_Exc.unlock();
                }
            }
        else
            {
            right_height = 0;
            }


        h = std::max(left_height, right_height) + 1;
        }
    return h;
    }

//This will find the difference in height between the two subtrees
int TTree::Difference(TNode* root)
    {
    if(root != nullptr)
        {
        if(root->Left != nullptr)
            {
            if(root->Right != nullptr)
                {

                int left_height;
                int right_height;

                if(root->Left->Base_leaf)
                    {
                    left_height = 0;
                    }
                else
                    {
                    root->Left->T_Exc.lock();
                    if(root->Left->BAL)
                        {
                        //std::cout << "IN LEFT ROOT NOT NULL WAITING ON EXCL " << i << std::endl;

                        root->Left->T_Exc.unlock();
                        std::unique_lock<std::mutex> lk_rlh(root->Left->Exclude);
                        root->Left->Excl.wait(lk_rlh, std::bind(&TNode::inscheck, root->Left));

#if debug
                        std::cout << root->Left << std::endl;
#endif // debug

                        if(root->Left->Base_leaf)
                            {
                            left_height = 0;
                            }

                        else if(root->Left != nullptr)
                            left_height = Height(root->Left);
                        }
                    else
                        {
                        root->Left->T_Exc.unlock();
                        left_height = Height(root->Left);
                        }
                    }




                if(root->Right->Base_leaf)
                    {
                    right_height = 0;
                    }
                else
                    {
                    root->Right->T_Exc.lock();
                    if(root->Right->BAL)
                        {
                        //std::cout << "IN LEFT ROOT NOT NULL WAITING ON EXCL " << i << std::endl;

                        root->Right->T_Exc.unlock();
                        std::unique_lock<std::mutex> lk_rrh(root->Right->Exclude);
                        root->Right->Excl.wait(lk_rrh, std::bind(&TNode::inscheck, root->Right));

#if debug
                        std::cout << root->Left << std::endl;
#endif // debug

                        if(root->Right->Base_leaf)
                            {
                            right_height = 0;
                            }

                        else if(root->Right != nullptr)
                            right_height = Height(root->Right);
                        }
                    else
                        {
                        root->Right->T_Exc.unlock();
                        right_height = Height(root->Right);
                        }
                    }





#if debug
                std::cout << "Left: " << left_height << " Right: " << right_height << std::endl;
#endif // debug

                return left_height - right_height;
                }
            }
        }
    return 0;

    }




void TTree::inorder(TNode* root)
    {
    //std::cout << "Inorder :" << root << std::endl;
    if(root == nullptr)
        {
        return;
        }

    else
        {



        inorder(root->Left);
        if(!root->Data.empty())
            {
            //std::cout << "MIN: " << root->VMin << " MAX: " << root->VMax << std::endl;
            //std::cout << root->Data.begin()->first << " " << root->Data.rbegin()->first << endl;
            root->show();
            }
        //std::cout << root << std::endl;
        inorder(root->Right);

        }

    }


void TTree::preorder(TNode* root)
    {
    if(root == nullptr)
        {
        return;
        }

    else
        {



        }

    }

//Rotation Functions


//Right Rotate
TNode* TTree::rr_rot(TNode* root)
    {

    TNode* temp = nullptr;

    temp = root->Right;

    //if(temp->Left != nullptr)
    root->Right = temp->Left;

    temp->Left = root;

    return temp;

    }

//Left Rotate
TNode* TTree::ll_rot(TNode* root)
    {
    TNode* temp = nullptr;;

    temp = root->Left;

    root->Left = temp->Right;

    temp->Right = root;

    return temp;
    }

//

//Double Left Rotate
TNode* TTree::lr_rot(TNode* root)
    {
    TNode* temp = nullptr;
    temp = root->Left;
    root->Left = rr_rot(temp);
    return ll_rot(root);
    }


//Double Right Rotate
TNode* TTree::rl_rot(TNode* root)
    {
    TNode* temp = nullptr;
    temp = root->Right;
    root->Right = ll_rot(temp);
    return rr_rot(root);
    }


TNode* TTree::Insert(int val, std::string dat, int i)
    {
    if(Root == nullptr)
        {
        bool q;
        Root = balance(Search_To_Insert(val, dat, Root, nullptr, i).first, q);
        }
    else
        {
        Root->T_Exc.lock();
        if(Root->BAL)
            {
            //std::cout << "IN LEFT ROOT NOT NULL WAITING ON EXCL " << i << std::endl;

            Root->T_Exc.unlock();
            std::unique_lock<std::mutex> lk_RO(Root->Exclude);
            Root->Excl.wait(lk_RO, std::bind(&TNode::inscheck, Root));
            }
        else
            {
            Root->T_Exc.unlock();
            }


        // std::cout << "IF THIS IS NON ZERO THERES A PROBLEM : " << Safe[i].size() << std::endl;
        //Root->Write.lock();
        //Safe[i].push(Root);

        bool q;
        Root = balance(Search_To_Insert(val, dat, Root, nullptr, i).first, q);

        /*
        while(! Safe[i].empty())
            {
            SF.lock();
            std::cout << "Problem " << std::endl;
            TNode* temp;
            temp = Safe[i].top();
            temp->Write.unlock();
            Safe[i].pop();
            SF.unlock();
            }
            */
        }
    return nullptr;
    }

bool TTree::Delete(int val, int i)
    {
    if(Root == nullptr)
        {
        return false;
        }
    else
        {
        std::pair<TNode*, bool> temp_node;
        //Root->Write.lock();
        temp_node = Search_To_Delete(val, Root, nullptr, i);

        bool q;
        Root = balance(temp_node.first, q);
        return temp_node.second;
        }
    }


TNode* TTree::Reb(TNode* root)
    {
    if(root != nullptr)
        {

        root->Left = Reb(root->Left);
        root->Right = Reb(root->Right);

        bool q;
        if(root->Height(root) > 2)
            return balance(root, q);

        }


    return root;

    }



///Rewriting the balance code









