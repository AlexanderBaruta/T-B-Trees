#ifndef BNODE_H
#define BNODE_H


class BNode
{
    public:

        int* Data;
        BNode** Children;
        bool leaf;



        BNode();
        virtual ~BNode();

    protected:

    private:
};

#endif // BNODE_H
