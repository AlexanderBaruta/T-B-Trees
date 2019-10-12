#include "TNode.h"
#include "TTree.h"
#include <vector>
#include <shared_mutex>
#include <condition_variable>
#include <semaphore.h>
#include <utility>
#include <iostream>
#define debug false
#define concur false

TNode::TNode(int D_Size, std::map<void*, TNode*>* N)
    {

    NODES = N;

    Base_leaf = false;
    Data_Size = D_Size;
    D_Min = 0.25 * Data_Size;
    ///This is set up like this so whatever value will always be larger than the default VMax, and smaller than the default VMin
    VMax = INT_MIN;
    VMin = INT_MAX;
    stored = 0;
    t = false;
    Left = nullptr;
    Right = nullptr;
    Tail = nullptr;
    Parent = nullptr;

    C_Write = 0;
    }

TNode::~TNode()
    {
    Parent = nullptr;
    Data.clear();
    Left = nullptr;
    Right = nullptr;
    Tail = nullptr;
    Parent = nullptr;
    }

///Using 3 seperate insert functions saves a tiny bit of time, given that we neeeded to make the comparisons in the insert function in the first place.
///We only need to make the full set of tail comparisons when the tail exists
int TNode::Insert(int val, std::string dat, bool overfl)
    {

    Ins.lock();


    //Ins.lock();

    ///Handle the case where we need to reconfigure VMax/VMin
    ///This will be true if the value stored is new, and false if this is a modification
    std::pair<std::map<int,std::string>::iterator,bool> ret;

///Insert the value into the map
    ret = Data.insert(std::make_pair(val, dat));
///If the value is new
    if(ret.second == true)
        {
///increment the stored value
        if(!overfl)
            {
            stored++;
            }
#if debug
        std::cout << "Inserted Val: " << val << " String: " << dat << std::endl;
        std::cout << "Stored: " << stored << std::endl;
        std::cout << "Pre actualization max and min" << std::endl;
        std::cout << "VMax: " << VMax << std::endl;
        std::cout << "VMin: " << VMin << std::endl;
        actualize(this);
        std::cout << "Post actualization max and min" << std::endl;
        std::cout << "VMax: " << VMax << std::endl;
        std::cout << "VMin: " << VMin << std::endl;

#endif
        }

    if(stored > Data_Size)
        {
        std::cout << "Error: Stored exceeds maximum" << std::endl;
        throw "Data Size Violation";
        }

    actualize(this);
    Ins.unlock();
    return 1;
    }


/**
Insertion.When inserting a new keyword , it determines the boundary node by search algorithm
firstly,this operation is same as the classic T-tree insertion. If the boundary node is full ,it generates
the tail node, and the minimum keyword in the boundary node will be moved to the tail node ,the
new keyword will be inserted into the boundary node. Thus, the subsequent insertion operation is
perform in the tail node.
*/
///Pseudocode thanks to:
/**
Study and Optimization of T-tree Index in Main Memory Database
Fengdong.Sun, Quan.Guo, Lan.Wang
Department Of Computer Science and Technology , DaLian Neusoft University of
Information, Dalian, China
*/

void TNode::insert_b(int val, std::string dat)
    {

    //Lock
    Ins.lock();

    std::map<int, std::string>::iterator temp_data;
    int temp_int;
    std::string temp_string;
    int count_return;
    ///According to the pseudocode, the tail will always store the lower portion of the data
    ///We know that the node is full and that it is the tail that has space. So if the value is greater than the
    ///Maximum tail value then we need to evict the minimum node value into the tail
    ///This is fine since insert_o evicts the minimum value from the shared space
    ///We can use the reconf method to keep track of the min/max values of the tail, since the tail also has these

    ///However this also acts as the update/modify case so we need to ensure that we are updating the values properly



    ///Data is smaller than current minimum
    //That is, smaller than the current maximum value stored in the tail.
    //Need <= to handle the update case on the max value in tail
    ///So we know that the value isn't in the main node
    ///And we know we have space in the tail by default
    if(val <= Tail->VMax)
        {
        Tail->Insert(val, dat, false);

        actualize(this);
        actualize(this->Tail);

        }
    else
        {
        //Otherwise we know that we need to remove the current minimum value from the node and store it in the tail
        //Unless the data exists inside the node
        count_return = Data.count(val);
        ///If the element is already in the storage
        if(count_return > 0)
            {
            ///Update the data object
            Data.insert(std::make_pair(val, dat));
            //Unlock
            Ins.unlock();
            return;
            }
        ///Get the minimum key
        ///Need to use the iterator, just never iterate on it
        ///Might as well get the data object while we're at it



        temp_data = Data.begin();
        temp_int = temp_data->first;
        temp_string = temp_data->second;

        //Need to again consider the case where the new value is smaller than the old
        if(val < temp_int)
            {
            temp_int = val;
            temp_string = dat;
            }
        else
            {
            ///Erase the old data
            Data.erase(temp_int);
            ///Insert the new
            Insert(val, dat, true);
            }

        Tail->Insert(temp_int, temp_string, false);

        actualize(this);
        actualize(this->Tail);

        }

    /**
        Handle the case where we need to reconfigure VMax/VMin
        We want to do this after so we get accurate overall VMax/VMin
        The nice part is that the tail keeps track of its own VMax/VMin so we can make it a fully fledged node at any time by altering a few pointers
        Ideally we want do do it if the tail reaches a certain level of saturation
    */
    actualize(this);
    actualize(this->Tail);

    //Unlock
    Ins.unlock();
    return;
    }

/**
    3)if no space is available in the node, then remove the minimum value from the node's data array
    and insert the new value. Now proceed to the node holding the greatest lower bound for the node
    that the new value was inserted to. If the removed minimum value still fits in there then add it as the
    new maximum value of the node, else create a new right subnode for this node.
*/
///Pseudocode thanks to:
/**
    Study and Optimization of T-tree Index in Main Memory Database
    Fengdong.Sun, Quan.Guo, Lan.Wang
    Department Of Computer Science and Technology , DaLian Neusoft University of
    Information, Dalian, China
*/

///This will remove the minimum value from the storage arrays and insert the new value into the storage space. Returning the minimum value to the main insertion process
std::pair<int, std::string> TNode::insert_o(int val, std::string dat)
    {
    //Lock
    Ins.lock();

    std::map<int, std::string>::iterator temp_data;
    int temp_int;
    std::string temp_string;
    int temp_int_a;
    std::string temp_string_a;
    int count_return;



    ///We need to rewrite the insert code inside this in order to get back the smallest value
    /**

    */
    //If the value is inside the tail
    if(val <= Tail->VMax)
        {
#if debug
        std::cout << "Element Less than Tail->VMax" << std::endl;
#endif // debug
        //If the value exists within the tail
        if(Tail->exists(val))
            {
            Tail->Insert(val, dat, false);
            //Unlock
            Ins.unlock();

            return std::make_pair(val, dat);
            }
        else
            {
            //Insert case
            ///This all happens inside the tail
            ///Get the minimum value
            temp_data = Tail->Data.begin();
            temp_int = temp_data->first;
            temp_string = temp_data->second;
            ///Erase the old data
            Tail->Data.erase(temp_int);
            ///Insert the new data
            Tail->Insert(val, dat, true);
            //Don't forget to set the VMax/VMin based on the new inserted value

            actualize(this);
            actualize(this->Tail);

            //Unlock
            Ins.unlock();

            return std::make_pair(temp_int, temp_string);
            }
        }
    else
        {

#if debug
        std::cout << "Element Cascade overflow from the main->root->return" << std::endl;
#endif // debug

        //We now need to possibly move data from the node to the tail

        //If the data exists in the node
        count_return = Data.count(val);

#if debug
        std::cout <<count_return<< std::endl;
#endif

        if(count_return > 0)
            {
            ///Update the values

#if debug
            std::cout << "Element Update" << std::endl;
#endif

            Insert(val, dat, true);
            //Returning the same value that was put in ensures
            //that the tree insert knows the value was updated


#if debug
            std::cout << "Element Attempt at actualize" << std::endl;
            std::cout << "Values Actualized" << std::endl;
#endif // debug

            actualize(this);
            actualize(this->Tail);

            //Unlock

            Ins.unlock();

            return std::make_pair(val, dat);
            }
        else
            {

#if debug
            std::cout << "Inserting New Value into Overflow Process" << std::endl;
#endif // debug

            //Need to move the minimum value from the node to the tail
            ///Get the minimum value
            temp_data = Data.begin();
            //Break this out into the primitives so it can be reused
            temp_int = temp_data->first;
            temp_string = temp_data->second;
            //Erase and insert

            ///Need to consider the possibility that the new value is smaller than the old
            ///In which case we do not delete the old smallest value from the node
            if(val < temp_int)
                {
                temp_int = val;
                temp_string = dat;
                }
            else
                {
                Data.erase(temp_int);
                this->Insert(val, dat, true);
                }

            //Get the smallest value in the tail
            temp_data = Tail->Data.begin();
            temp_int_a = temp_data->first;
            temp_string_a = temp_data->second;

            //Erase and insert into the tail
            Tail->Data.erase(temp_int_a);
            Tail->Insert(temp_int, temp_string, true);

            actualize(this);
            actualize(this->Tail);

            //Unlock
            Ins.unlock();

            return std::make_pair(temp_int_a, temp_string_a);
            }
        }

    ///This is especially pertenant because we need to hard set VMin to the new VMin once the old one is evicted from the node
    ///Handle the case where we need to reconfigure VMax/VMin
    actualize(this);
    actualize(this->Tail);

    //Unlock
    Ins.unlock();

    //Cleanup
    return std::make_pair(val, dat);
    }



int TNode::Delete(int val)
    {
    if(exists(val))
        {

        //Lock
        Ins.lock();

        std::map<int, std::string>::reverse_iterator temp_data;
        std::map<int, std::string>::iterator temp_data1;
        int temp_int;
        std::string temp_string;


        if(Tail != nullptr)
            {
            if(Tail->exists(val))
                {
                Tail->Delete(val);
                }
            else
                {
                Data.erase(val);
                if(Tail->stored > 0)
                    {

                    //We need to move the maximum value from the tail into the node

                    //Tail->show();

                    if(Tail->stored == 1)
                        {
                        temp_data1 = Tail->Data.begin();

                        temp_int = temp_data1->first;
                        temp_string = temp_data1->second;

                        Tail->Data.erase(temp_data1->first);
                        Tail->stored--;
                        //Tail->show();

#if debug
                        std::cout << "Attempting to insert :" << temp_data1->first << " " << std::endl;
#endif

                        if(Tail->stored == 0)
                            {
                            NODES->erase(Tail);
                            delete(Tail);
                            Tail = nullptr;
                            }

                        Data.insert( std::make_pair( temp_int, temp_string) );
                        actualize(this);

                        Ins.unlock();

                        return 1;
                        }

                    temp_data = Tail->Data.rbegin();

                    temp_int = temp_data->first;
                    temp_string = temp_data->second;

                    Tail->Data.erase(temp_data->first);
                    Tail->stored--;

                    if(Tail->stored == 0)
                        {
                        NODES->erase(Tail);
                        delete(Tail);
                        Tail = nullptr;
                        }
                    else
                        {
                        actualize(this->Tail);
                        }

#if debug
                    std::cout << temp_data->first << " " << temp_data->second << std::endl;
#endif // debug

                    Data.insert(std::make_pair(temp_int, temp_string));

                    Ins.unlock();

                    actualize(this);
                    return 1;

                    }
                else
                    {
                    delete(Tail);
                    Tail = nullptr;
                    }
                }
            }
        else
            {

            Data.erase(val);
            stored--;
            if(stored > 0)
                actualize(this);

            Ins.unlock();

            return 1;
            }
        }
    return 0;
    }

//
bool TNode::is_space()
    {
#if debug
    std::cout << "Reached is_space" << std::endl;
#endif // debug


    if(stored < Data_Size)
        {
#if debug
        std::cout << "There is Space" << stored << " out of " << Data_Size << std::endl;
#endif // debug

        return 1;
        }
    else
        {

#if debug
        std::cout << "There is No Space" << stored << " out of " << Data_Size << std::endl;
#endif // debug
        return 0;
        }
    }

//
bool TNode::can_insert_b()
    {
    if( (stored + Tail->stored) < (2 * Data_Size))
        {
        return 1;
        }
    else
        {
        return 0;
        }
    }


bool TNode::can_merge()
    {
    if(Right != nullptr)
        {
        if(Right->Tail != nullptr)
            {
            return false;
            }

        if( (stored + Right->stored) <  Data_Size)
            {
            return true;
            }
        }
    else if(Left != nullptr)
        {
        if(Left->Tail != nullptr)
            {
            return false;
            }
        if( (stored + Left->stored) <  Data_Size)
            {
            return true;
            }
        }
    return false;
    }

void TNode::merge_data()
    {
#if debug
    std::cout << "Merging" << std::endl;
#endif // debug

    if(Left != nullptr)
        {
        Ins.lock();
        Left->Ins.lock();
        Exclude.lock();
        Left->Exclude.lock();
        Data.insert(Left->Data.begin(), Left->Data.end());
        Left->Ins.lock();
        Left->Exclude.unlock();



        delete(Left);
        Left = nullptr;
        Exclude.unlock();
        }
    if(Right != nullptr)
        {
        Data.insert(Right->Data.begin(), Right->Data.end());
        delete(Right);
        Right = nullptr;
        }
    }


bool TNode::underflow()
    {
    if(stored < D_Min)
        {
        return true;
        }
    return false;
    }


bool TNode::internal()
    {
    if((Left != nullptr) && (Right != nullptr))
        {
        return true;
        }
    return false;
    }

///This sets VMax/VMin to val if val exceedes/precedes them in value
///This will be used enough to justify breaking it out into a seperate function
void TNode::Reconf(int val)
    {
    if(val < VMin)
        {
        VMin = val;
        }
    if(val > VMax)
        {
        VMax = val;
        }
    }


///0: Insert the value into this node
///Possibility of inserting into tail, splitting into other node, ect
///If the node needs to be split or reconfigured
///1: Search the Left Subtree <-- Values Smaller than VMin
///2: Search the Right Subtree --> Values Greater than VMax
int TNode::isbound(int val)
    {
    ///If the value to be stored is larger than the minimum value
    if(val >= VMin)
        {
        ///If the value falls within the bounds of VMax and VMin
        if( val <= VMax)
            {
            return 0;
            }
        else
            {
            return 2;
            }
        }
    else
        {
        return 1;
        }
    }

bool TNode::has_tail(TNode* root)
    {
    if(root->Tail == nullptr)
        {
        return false;
        }
    return true;
    }

//
bool TNode::can_tail(TNode* root)
    {
    if(root->Tail == nullptr)
        {

#if debug
        std::cout << "Created Tail" << std::endl;
#endif // debug

        root->Tail = new TNode(Data_Size, NODES);
        root->Tail->t = true;
        root->Tail->Left = nullptr;
        root->Tail->Right = nullptr;
        root->Tail->Parent = root;
        return true;
        }
    return false;
    }

//
bool TNode::exists(int val)
    {
    int count_return;
    if(Tail != nullptr)
        {
        ///If the value is likely to be in the node
        if(val > Tail->VMax)
            {
            count_return = Data.count(val);
            }
        ///Otherwise the value is likely to be in the tail
        else
            {
            count_return = Tail->Data.count(val);
            }
        }
    else
        {
        ///We need to search only in the node
        count_return = Data.count(val);
        }
    return count_return;
    }


//This is meant to ensure that the VMax and VMin are properly set after values are removed from the node
void TNode::actualize(TNode* root)
    {
    std::map<int, std::string>::reverse_iterator temp_max;
    std::map<int, std::string>::iterator temp_min;

    if(root->Tail != nullptr)
        {
        root->Tail->stored = root->Tail->Data.size();
        if(root->Tail->stored > 0)
            {
            temp_min = root->Tail->Data.begin();
            temp_max = root->Data.rbegin();

            root->VMin = temp_min->first;
            root->VMax = temp_max->first;

            root->Tail->actualize(root->Tail);
            return;
            }
        }

    temp_min = root->Data.begin();
    temp_max = root->Data.rbegin();

    root->stored = root->Data.size();

    if(root->stored > 0)
        {
        root->VMin = temp_min->first;
        root->VMax = temp_max->first;
        }

    }

std::string TNode::get(int val)
    {
    //Wait for readers to complete
    //Ins.lock();

    std::string temp_string;

    if(Tail != nullptr)
        {
        if(Tail->exists(val))
            {
            return Tail->get(val);
            }
        }

    temp_string = Data.find(val)->second;

    //Ins.unlock();
    return temp_string;


    }

void TNode::borrow(TNode* &left)
    {
    std::map<int, std::string>::reverse_iterator temp_max;

    //Left->show();


    temp_max = left->Data.rbegin();

#if debug
    std::cout << temp_max->first << " " << temp_max->second << std::endl;
#endif // debug

    this->Data.insert(std::make_pair(temp_max->first, temp_max->second));
    Data.erase(temp_max->first);

    if(left->Tail != nullptr)
        {
        temp_max = left->Tail->Data.rbegin();

        left->Data.insert(std::make_pair(temp_max->first, temp_max->second));

        left->Tail->Data.erase(temp_max->first);

        actualize(left->Tail);

        left->Tail->stored--;
        }
    else
        {
        left->stored--;
        }
    actualize(this);
    actualize(left);

    if(left->underflow())
        {
        if(left->internal())
            {
            std::cout << "Chain Borrow" << std::endl;
            //left->Left->Read.lock();
            borrow(left->Left);
            //left->Left->Read.unlock();

            }
        }
    left = balance(left);
    //left->Read.unlock();
    return;
    }

bool TNode::half_leaf()
    {
    if((Left != nullptr) ^ (Right != nullptr))
        {
        return true;
        }
    return false;
    }


void TNode::show()
    {

    if(Tail != nullptr)
        {
        std::cout << "Showing values in nodes tail" << std::endl;
        Tail->show();
        }
    std::cout << "Values in Node:" << std::endl;
    for(std::map<int, std::string>::iterator it = Data.begin(); it != Data.end(); ++it)
        {
        std::cout << it->first << "    " << it->second << std::endl;
        }
    }



bool TNode::leaf()
    {

    if( (Left) || (Right) )
        {
        return 0;
        }
    return 1;
    }

    int TNode::adj_height()
    {
        if(Base_leaf)
        {
            height = 0;
            return 0;
        }
        if(Left != nullptr)
        {

        }



    }






void TNode::cleanup()
    {

    if(Tail != nullptr)
        {
        if(Tail->stored == 0)
            {
            delete(Tail);
            Tail = nullptr;
            }
        }
    if(Left != nullptr)
        {
        if(Left->stored == 0)
            {
            delete(Left);
            Left = nullptr;
            }
        }
    if(Right != nullptr)
        {
        if(Right->stored == 0)
            {
            delete(Right);
            Right = nullptr;
            }
        }
    if((Right == nullptr) && (Left == nullptr))
        {
        Base_leaf = true;
        }

    }






TNode* TNode::balance(TNode* root)
    {
#if debug
    std::cout << "Balancing:" << std::endl;
#endif
if(root == nullptr)
{
    return nullptr;
}

    int bal = Difference(root);

#if debug
    std::cout << bal << std::endl;
#endif


    TNode* temp;
    //This will set the locks
    temp = root->grab();

    #if debug
    std::cout << "Grabbed" << std::endl;
    #endif // debug

    if(bal > 1)
        {
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
        root->release(temp);
        root->release(root);

#if debug
    std::cout << root << std::endl;
#endif // debug

    return root;
    }


TNode* TNode::grab()
    {

    #if concur
    std::cout << "Waiting on Exclude Unlock" << std::endl;
    #endif // debug
    Exclude.lock();



    #if concur
    std::cout << "NO LONGER WAITING ON EXCLUDE UNLOCK" << std::endl;
    std::cout << "EXCLUDED: " << this << std::endl;
    #endif // debug

    if(Base_leaf)
        {
        Exclude.unlock();
        return this;
        }
    #if concur
    std::cout << C_Write << std::endl;
    #endif // debug

    if(this != nullptr)
        {
        T_Read.lock();
        if(C_Write > 1)
            {
            T_Read.unlock();
            #if concur
            std::cout << "Waiting on LK(READ)" << std::endl;
            #endif // debug
            std::unique_lock<std::mutex> lk(Read);
            Rea.wait(lk, std::bind(&TNode::rotcheck, this));
            #if concur
            std::cout << "FIN Waiting on LK(READ)" << std::endl;
            #endif // debug

            T_Exc.lock();
            BAL = true;
            T_Exc.unlock();
            }
        else
            {
            T_Read.unlock();
            #if concur
            std::cout << "Safelocking the BAL " << std::endl;
            #endif // debug
            T_Exc.lock();
            BAL = true;
            T_Exc.unlock();
            #if concur
            std::cout << "Safelocked the BAL " << std::endl;
            #endif // debug
            }
        }

    if(Left != nullptr)
        {
        if(!Left->Base_leaf)
            {
            #if concur
            std::cout << Left->C_Write << std::endl;
            #endif // debug
            Left->T_Read.lock();
            if(Left->C_Write > 1)
                {
                Left->T_Read.unlock();
                std::cout << "Waiting on LK(READ)->LEFT" << std::endl;
                std::unique_lock<std::mutex> lkl(Left->Read);
                Left->Rea.wait(lkl, std::bind(&TNode::rotcheck, Left));
                std::cout << "Fin Waiting on LK(READ)->LEFT" << std::endl;

                Left->T_Exc.lock();
                Left->BAL = true;
                Left->T_Exc.unlock();
                }
            else
                {
                Left->T_Read.unlock();

                #if concur
                std::cout << "Safelocking the LEFT BAL " << std::endl;
                #endif // debug

                Left->T_Exc.lock();
                Left->BAL = true;
                Left->T_Exc.unlock();

                #if concur
                std::cout << "Safelocked the LEFT BAL " << std::endl;
                #endif // debug

                //Grab LL and LR

                if(Left->Left != nullptr)
                    {
                    Left->Left->T_Read.lock();
                    if(Left->Left->C_Write > 1)
                        {
                        Left->Left->T_Read.unlock();
                        std::unique_lock<std::mutex> lkll(Left->Left->Read);
                        Left->Left->Rea.wait(lkll, std::bind(&TNode::rotcheck, Left->Left));

                        Left->Left->T_Exc.lock();
                        Left->Left->BAL = true;
                        Left->Left->T_Exc.unlock();
                        }
                    else
                        {
                        Left->Left->T_Read.unlock();

                        #if concur
                        std::cout << "Safelocking the LEFT LEFT BAL " << std::endl;
                        #endif // debug

                        Left->Left->T_Exc.lock();
                        Left->Left->BAL = true;
                        Left->Left->T_Exc.unlock();

                        #if concur
                        std::cout << "Safelocked the LEFT LEFT BAL " << std::endl;
                        #endif // debug
                        }
                    }
                if(Left->Right != nullptr)
                    {
                    Left->Right->T_Read.lock();
                    if(Left->Right->C_Write > 1)
                        {
                        Left->Right->T_Read.unlock();

                        std::unique_lock<std::mutex> lklr(Left->Right->Read);
                        Left->Right->Rea.wait(lklr, std::bind(&TNode::rotcheck, Left->Right));

                        Left->Right->T_Exc.lock();
                        Left->Right->BAL = true;
                        Left->Right->T_Exc.unlock();
                        }
                    else
                        {
                        Left->Right->T_Read.unlock();

                        #if concur
                        std::cout << "Safelocking the LEFT RIGHT BAL " << std::endl;
                        #endif // debug

                        Left->Right->T_Exc.lock();
                        Left->Right->BAL = true;
                        Left->Right->T_Exc.unlock();

                        #if concur
                        std::cout << "Safelocked the LEFT LEFT BAL " << std::endl;
                        #endif // debug
                        }
                    }
                }
            }
        }

    if(Right != nullptr)
        {
        if(!Right->Base_leaf)
            {

            #if concur
            std::cout << Right->C_Write << std::endl;
            #endif // debug

            Right->T_Read.lock();
            if(Right->C_Write > 1)
                {
                std::cout << "Waiting on LK(READ)->RIGHT" << std::endl;
                Right->T_Read.unlock();
                std::unique_lock<std::mutex> lkr(Right->Read);
                Right->Rea.wait(lkr, std::bind(&TNode::rotcheck, Right));
                std::cout << "Waiting on LK(READ)->RIGHT" << std::endl;

                Right->T_Exc.lock();
                Right->BAL = true;
                Right->T_Exc.unlock();
                }
            else
                {
                Right->T_Read.unlock();

                #if concur
                std::cout << "Safelocking the LEFT BAL " << std::endl;
                #endif // debug

                Right->T_Exc.lock();
                Right->BAL = true;
                Right->T_Exc.unlock();

                #if concur
                std::cout << "Safelocked the LEFT BAL " << std::endl;
                #endif // debug

                //Grab RL and RR

                if(Right->Left != nullptr)
                    {
                    Right->Left->T_Read.lock();
                    if(Right->Left->C_Write > 1)
                        {
                        Right->Left->T_Read.unlock();

                        std::cout << "Waiting on LK(READ)->RIGHT->LEFT" << std::endl;

                        std::unique_lock<std::mutex> lkrl(Right->Left->Read);
                        Right->Left->Rea.wait(lkrl, std::bind(&TNode::rotcheck, Right->Left));

                        std::cout << "FIN Waiting on LK(READ)->RIGHT->LEFT" << std::endl;

                        Right->Left->T_Exc.lock();
                        Right->Left->BAL = true;
                        Right->Left->T_Exc.unlock();
                        }
                    else
                        {
                        Right->Left->T_Read.unlock();

                        #if concur
                        std::cout << "Safelocking the LEFT LEFT BAL " << std::endl;
                        #endif // debug

                        Right->Left->T_Exc.lock();
                        Right->Left->BAL = true;
                        Right->Left->T_Exc.unlock();

                        #if concur
                        std::cout << "Safelocked the LEFT LEFT BAL " << std::endl;
                        #endif // debug
                        }
                    }
                if(Right->Right != nullptr)
                    {
                    Right->Right->T_Read.lock();
                    if(Right->Right->C_Write > 1)
                        {
                        Right->Right->T_Read.unlock();
                        std::cout << "Waiting on LK(READ)->RIGHT->RIGHT" << std::endl;
                        std::unique_lock<std::mutex> lkrr(Right->Right->Read);
                        Right->Right->Rea.wait(lkrr, std::bind(&TNode::rotcheck, Right->Right));
                        std::cout << "FIN Waiting on LK(READ)->RIGHT->RIGHT" << std::endl;

                        Right->Right->T_Exc.lock();
                        Right->Right->BAL = true;
                        Right->Right->T_Exc.unlock();
                        }
                    else
                        {
                        Right->Right->T_Read.unlock();

                        #if concur
                        std::cout << "Safelocking the LEFT RIGHT BAL " << std::endl;
                        #endif // debug

                        Right->Right->T_Exc.lock();
                        Right->Right->BAL = true;
                        Right->Right->T_Exc.unlock();

                        #if concur
                        std::cout << "Safelocked the LEFT LEFT BAL " << std::endl;
                        #endif // debug
                        }
                    }
                }
            }
        }
    Exclude.unlock();
    return this;

    }

void TNode::release(TNode* main)
    {

    #if debug
    std::cout << "Releasing Grab" << std::endl;
    #endif // debug

    if(Base_leaf)
        {

        T_Exc.lock();
        BAL = false;
        Excl.notify_all();
        T_Exc.unlock();

        main->Exclude.unlock();
        //Exclude.unlock();
        return;
        }

    if(this != nullptr)
        {

        T_Exc.lock();
        BAL = false;
        Excl.notify_all();
        T_Exc.unlock();

        }

    if(Left != nullptr)
        {
        if(!Left->Base_leaf)
            {

            //Left->Exclude.try_lock();
            Left->T_Exc.lock();
            Left->BAL = false;
            Left->Excl.notify_all();
            Left->T_Exc.unlock();
            //Left->Exclude.unlock();

            //Grab LL and LR

            if(Left->Left != nullptr)
                {

                //Left->Left->Exclude.try_lock();
                Left->Left->T_Exc.lock();
                Left->Left->BAL = false;
                Left->Left->Excl.notify_all();
                Left->Left->T_Exc.unlock();
                //Left->Left->Exclude.unlock();

                }

            if(Left->Right != nullptr)
                {

                //Left->Right->Exclude.try_lock();
                Left->Right->T_Exc.lock();
                Left->Right->BAL = false;
                Left->Right->Excl.notify_all();
                Left->Right->T_Exc.unlock();
                //Left->Right->Exclude.unlock();
                }
            }
        }

    if(Right != nullptr)
        {
        if(!Right->Base_leaf)
            {

            //Right->Exclude.try_lock();
            Right->T_Exc.lock();
            Right->BAL = false;
            Right->Excl.notify_all();
            Right->T_Exc.unlock();
           // Right->Exclude.unlock();

            //Release RL and RR

            if(Right->Left != nullptr)
                {

                //Right->Left->Exclude.try_lock();
                Right->Left->T_Exc.lock();
                Right->Left->BAL = false;
                Right->Left->Excl.notify_all();
                Right->Left->T_Exc.unlock();
                //Right->Left->Exclude.unlock();

                }
            if(Right->Right != nullptr)
                {

                //Right->Right->Exclude.try_lock();
                Right->Right->T_Exc.lock();
                Right->Right->BAL = false;
                Right->Right->Excl.notify_all();
                Right->Right->T_Exc.unlock();
                //Right->Right->Exclude.unlock();

                }
            }
        }

    #if debug
    std::cout << "Release Exclude Lock" << std::endl;
    #endif // debug

    main->Exclude.unlock();
    Exclude.unlock();

    #if debug
    std::cout  <<"Released Exclude Lock" << std::endl;
    #endif // debug

    return;

    }




bool TNode::can_write()
{
    return !WW;
}

bool TNode::rotcheck()
    {
    if(C_Write < 2)
        {
        return 1;
        }
    return 0;
    }

bool TNode::inscheck()
    {
    return BAL;
    }








//Algorithm for finding the height of a node
int TNode::Height(TNode* root)
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
int TNode::Difference(TNode* root)
    {
    if(root != nullptr)
        {
        if(root->Left != nullptr)
            {
            if(root->Right != nullptr)
                {
                int left_height = Height(root->Left);
                int right_height = Height(root->Right);

                std::cout << "Left: " << left_height << " Right: " << right_height << std::endl;

                return left_height - right_height;
                }
            }
        }
    return 0;

    }


//Rotation Functions

//Right Rotate
TNode* TNode::rr_rot(TNode* root)
    {

    TNode* temp;

    temp = root->Right;

    //if(temp->Left != nullptr)
    root->Right = temp->Left;

    temp->Left = root;

    return temp;

    }

//Left Rotate
TNode* TNode::ll_rot(TNode* root)
    {

    TNode* temp;

    temp = root->Left;

    //if(temp->Right != nullptr)
    root->Left = temp->Right;

    temp->Right = root;

    return temp;
    }

//

//Double Left Rotate
TNode* TNode::lr_rot(TNode* root)
    {
    TNode* temp;
    temp = root->Left;
    root->Left = rr_rot(temp);
    return ll_rot(root);
    }


//Double Right Rotate
TNode* TNode::rl_rot(TNode* root)
    {
    TNode* temp;
    temp = root->Right;
    root->Right = ll_rot(temp);
    return rr_rot(root);
    }
