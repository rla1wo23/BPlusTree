#include <iostream>
#include <cstdio>
#include <fstream>
#include <vector>
#include <cstdlib>

int n; // number of maximum entries;
int blockCnt = 1;
using namespace std;
class Node;
std::vector<Node *> blocks;

class Node
{
public:
    bool isLeaf;
    vector<int> keys;
    int BID;
    vector<int> values;
    Node *next;
    vector<Node *> pointers; // 리프노드에만 쓰임

    Node(bool leaf) : isLeaf(leaf), next(nullptr)
    {
        BID = blockCnt;
        blockCnt++;
        blocks.push_back(this);
    }

    ~Node()
    {
        if (isLeaf)
        {
            values.clear();
            next = nullptr;
        }
        else
        {
            pointers.clear();
        }
        keys.clear();
    }
};

class BPlusTree
{
private:
    Node *root;
    int n;

public:
    BPlusTree(int n) : root(nullptr), n(n) {}

    void insert(int key, int value)
    {
        if (root == nullptr)
        {
            root = new Node(true);
            root->keys.push_back(key);
            root->values.push_back(value);
            return;
        }
        else if (root->isLeaf)
        {
            if (root->keys.size() < n)
            {
                insertIntoLeaf(root, key, value);
            }
            else
            {
                Node *newRoot = new Node(false);
                newRoot->pointers.push_back(root);

                Node *newLeaf = new Node(true);
                newRoot->pointers.push_back(newLeaf);
                splitLeafNode(root, newLeaf, key, value);
                newRoot->keys.push_back(newLeaf->keys[0]);

                root = newRoot;
            }
        }
        else
        {
            Node *currNode = root;
            Node *parent = nullptr;

            while (!currNode->isLeaf)
            {
                parent = currNode;

                int childIndex = 0;
                while (childIndex < currNode->keys.size() && key >= currNode->keys[childIndex])
                {
                    childIndex++;
                }

                currNode = currNode->pointers[childIndex];
            }

            if (currNode->keys.size() < n)
            {
                insertIntoLeaf(currNode, key, value);
            }
            else
            {
                Node *newLeaf = new Node(true);
                splitLeafNode(currNode, newLeaf, key, value);

                int index = 0;
                while (index < parent->pointers.size() && parent->pointers[index] != currNode)
                {
                    index++;
                }

                parent->pointers.insert(parent->pointers.begin() + index + 1, newLeaf);
                parent->keys.insert(parent->keys.begin() + index, newLeaf->keys[0]);

                if (parent->keys.size() > n)
                {
                    splitNonLeafNode(parent);
                }
            }
        }
    }

    void insertIntoLeaf(Node *leaf, int key, int value)
    {
        int index = 0;
        while (index < leaf->keys.size() && key >= leaf->keys[index])
        {
            index++;
        }
        leaf->keys.insert(leaf->keys.begin() + index, key);
        leaf->values.insert(leaf->values.begin() + index, value);
    }

    void splitLeafNode(Node *oldLeaf, Node *newLeaf, int key, int value)
    {
        insertIntoLeaf(oldLeaf, key, value);
        int midIndex = (oldLeaf->keys.size() + 1) / 2;

        for (int i = midIndex; i < oldLeaf->keys.size(); i++)
        {
            newLeaf->keys.push_back(oldLeaf->keys[i]);
            newLeaf->values.push_back(oldLeaf->values[i]);
        }

        oldLeaf->keys.resize(midIndex);
        oldLeaf->values.resize(midIndex);

        newLeaf->next = oldLeaf->next;
        oldLeaf->next = newLeaf;
    }

    void splitNonLeafNode(Node *node) // spliting
    {
        int midIndex = node->keys.size() / 2;
        Node *newChild = new Node(false);

        for (int i = midIndex + 1; i < node->keys.size(); i++)
        {
            newChild->keys.push_back(node->keys[i]);
            newChild->pointers.push_back(node->pointers[i]);
        }
        newChild->pointers.push_back(node->pointers.back());

        node->keys.resize(midIndex);
        node->pointers.resize(midIndex + 1);

        Node *parent = findParent(root, node);
        if (parent == nullptr)
        {
            Node *newRoot = new Node(false);
            newRoot->keys.push_back(node->keys[midIndex]);
            newRoot->pointers.push_back(node);
            newRoot->pointers.push_back(newChild);
            root = newRoot;
        }
        else
        {
            int index = 0;
            while (index < parent->pointers.size() && parent->pointers[index] != node)
            {
                index++;
            }
            parent->keys.insert(parent->keys.begin() + index, node->keys[midIndex]);
            parent->pointers.insert(parent->pointers.begin() + index + 1, newChild);

            if (parent->keys.size() > n)
            {
                splitNonLeafNode(parent);
            }
        }
    }

    Node *findParent(Node *parentNode, Node *childNode)
    {
        if (parentNode == nullptr || parentNode->isLeaf)
        {
            return nullptr;
        }

        for (Node *pointer : parentNode->pointers)
        {
            if (pointer == childNode)
            {
                return parentNode;
            }

            Node *fromLowerLevel = findParent(pointer, childNode);
            if (fromLowerLevel != nullptr)
            {
                return fromLowerLevel;
            }
        }
        return nullptr;
    }
    int calcDepth()
    {
        int depth = 0;
        Node *curNode = root;
        while (curNode->isLeaf != true)
        {
            curNode = curNode->pointers[0];
            depth++;
        }
        return depth;
    }
    void check()
    {
        ofstream outputFile("print.txt");

        if (outputFile.is_open())
        {
            outputFile << "<0>" << endl;
            for (int i : root->keys)
            {
                outputFile << i << ", ";
            }
            outputFile << "\n<1>" << endl;
            for (Node *p : root->pointers)
            {
                for (int i : p->keys)
                {
                    outputFile << i << ", ";
                }
            }

            outputFile.close();
        }
        else
        {
            cout << "error" << endl;
        }
    }
    void insertBin(const char *fileName)
    {
        int blockSize = n * 8 + 4;
        int rootBid = root->BID;
        int depth = calcDepth();
        int zero = 0;
        cout << "총 블럭 수는:" << blockCnt;
        FILE *file = fopen(fileName, "wb");
        if (file != nullptr)
        {
            fwrite(&blockSize, sizeof(int), 1, file);
            fwrite(&rootBid, sizeof(int), 1, file);
            fwrite(&depth, sizeof(int), 1, file);
            Node *curNode;
            for (int i = 0; i < blocks.size(); i++)
            {
                curNode = blocks[i];

                for (int k = 0; k < n; k++)
                {
                    if (curNode->isLeaf)
                    {
                        if (k < curNode->keys.size() && curNode->keys[k] != 0) // 값이 있는 경우
                        {
                            fwrite(&curNode->keys[k], sizeof(int), 1, file);
                            fwrite(&curNode->values[k], sizeof(int), 1, file);
                        }
                        else
                        {
                            fwrite(&zero, sizeof(int), 2, file);
                        }
                    }
                    else
                    {
                        if (k < curNode->keys.size() && curNode->pointers[k] != nullptr)
                        {
                            fwrite(&curNode->pointers[k]->BID, sizeof(int), 1, file);
                            fwrite(&curNode->keys[k], sizeof(int), 1, file);
                        }
                        else
                        {
                            fwrite(&zero, sizeof(int), 2, file);
                        }
                    }
                }
                if (curNode->isLeaf)
                {
                    if (curNode->next != nullptr)
                    {
                        fwrite(&curNode->next->BID, sizeof(int), 1, file);
                    }
                    else
                    {
                        fwrite(&zero, sizeof(int), 1, file);
                    }
                }
                else
                {
                    if (curNode->pointers[n - 1] != nullptr)
                    {
                        fwrite(&curNode->pointers[n - 1]->BID, sizeof(int), 1, file);
                    }
                    else
                    {
                        fwrite(&zero, sizeof(int), 1, file);
                    }
                }
            }

            fclose(file);
        }
        else
        {
            cout << "Error1" << endl;
        }
    }
    int printing(const char *fileName)
    {
        ofstream outputFile("print.txt");

        int blockSize;
        int rootBid;
        int depth;
        ifstream binfile(fileName, std::ios::binary);
        binfile.seekg(0, std::ios::beg);
        binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
        binfile.read(reinterpret_cast<char *>(&rootBid), sizeof(rootBid));
        binfile.read(reinterpret_cast<char *>(&depth), sizeof(depth));

        n = (blockSize - 4) / 8;
        binfile.seekg(12 + blockSize * (rootBid), std::ios::beg);
        root = new Node(false);
        int last = rootBid;
        vector<int> keys;
        vector<int> pointerBlock;
        outputFile << "<0>" << endl;
        int fix = 0;
        int v, p;
        binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
        pointerBlock.push_back(p);
        for (int k = 0; k < n; k++)
        {
            binfile.read(reinterpret_cast<char *>(&v), sizeof(v));
            binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
            if (v != 0 && v != 2)
            {
                outputFile << v << ",";
            }
            keys.push_back(v);
            pointerBlock.push_back(p);
        }
        int warp;
        outputFile << endl
                   << "<1>" << endl;
        for (int k = 0; k < pointerBlock.size(); k++)
        {
            warp = pointerBlock[k];
            binfile.seekg(12 + blockSize * (warp), std::ios::beg);
            vector<int> ks;
            vector<int> pb;
            binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
            pb.push_back(p);
            for (int k = 0; k < n; k++)
            {
                binfile.read(reinterpret_cast<char *>(&v), sizeof(v));
                binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
                ks.push_back(v);
                pb.push_back(p);
                if (v != 0)
                {
                    outputFile << v << ",";
                }
            }
        }
    }
    int reading(const char *fileName, int searchKey)
    {
        int blockSize;
        int rootBid;
        int depth;
        ifstream binfile(fileName, std::ios::binary);
        binfile.seekg(0, std::ios::beg);
        binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
        binfile.read(reinterpret_cast<char *>(&rootBid), sizeof(rootBid));
        binfile.read(reinterpret_cast<char *>(&depth), sizeof(depth));

        n = (blockSize - 4) / 8;
        binfile.seekg(12 + blockSize * (rootBid), std::ios::beg);
        root = new Node(false);
        int last = rootBid;
        for (int i = 0; i < depth; i++) // nonleaf까지 수행
        {
            vector<int> keys;
            vector<int> pointerBlock;
            int fix = 0;
            int p;
            binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
            pointerBlock.push_back(p);
            for (int k = 0; k < n; k++)
            {
                int v;
                binfile.read(reinterpret_cast<char *>(&v), sizeof(v));
                binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
                keys.push_back(v);
                pointerBlock.push_back(p);
            }

            int k = 0;
            for (k = 0; k < n; k++)
            {
                if (keys[k] > searchKey)
                {
                    if (keys[k] == searchKey)
                    {
                        fix = 1;
                    }
                    break;
                }
            }
            if (fix == 1)
            {
                k = 0;
            }
            int target = pointerBlock[k];
            last = target;
            binfile.seekg(12 + blockSize * target, std::ios::beg);
        }
        binfile.seekg(12 + blockSize * (last), std::ios::beg);
        vector<int> keys;
        vector<int> values;
        for (int k = 0; k < n; k++)
        {
            int ke, v;
            binfile.read(reinterpret_cast<char *>(&ke), sizeof(ke));
            binfile.read(reinterpret_cast<char *>(&v), sizeof(v));
            values.push_back(v);
            keys.push_back(ke);
        }
        int k;
        for (k = 0; k < n; k++)
        {
            if (keys[k] == searchKey)
            {
                break;
            }
        }
        int ret = values[k];
        binfile.close();
        if (k == n)
        {
            return -1;
        }
        return ret;
    }
};

int main(int argc, char *argv[])
{
    blockCnt = 0;
    char command = argv[1][0];
    const char *fileName = argv[2];
    switch (command)
    {
    case 'c':
    {
        int blockSize = atoi(argv[3]);
        FILE *file = fopen(fileName, "wb");
        if (file != nullptr)
        {
            int zero = 0;
            fwrite(&blockSize, sizeof(int), 1, file);
            fwrite(&zero, sizeof(int), 2, file);
            fclose(file);
        }
        else
        {
            cout << "Error1" << endl;
        }
        break;
    }
    case 'i':
    {
        ifstream binfile(fileName, ios::binary);
        int blockSize;
        binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
        n = (blockSize - 4) / 8;
        cout << "n:" << n << endl;
        ifstream input(argv[3]);
        if (!input)
        {
            cerr << "Error2";
            return 1;
        }
        BPlusTree btree(n);

        char k[100];
        char v[100];
        while (input.getline(k, sizeof(k), '|') && input.getline(v, sizeof(v)))
        {
            int key = atoi(k);
            int value = atoi(v);

            btree.insert(key, value);
        }
        btree.check(); // make output
        binfile.close();
        btree.insertBin(fileName);

        break;
    }
    case 's':
    {
        ifstream binfile(fileName, std::ios::binary);
        int blockSize;
        binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
        n = (blockSize - 4) / 8;
        binfile.close();
        BPlusTree btree(n);
        ifstream input(argv[3]);
        int numb;
        ofstream outputFile(argv[4]);
        while (input >> numb)
        {
            int val;
            val = btree.reading(fileName, numb);
            outputFile << numb << "|" << val << endl;
        }
        outputFile.close();
        break;
    }
    case 'r':
    {
        break;
        // search keys in [input file] and print results to [output file]
    }
    case 'p':
    {
        ifstream binfile(fileName, std::ios::binary);
        int blockSize;
        binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
        n = (blockSize - 4) / 8;
        binfile.close();
        BPlusTree btree(n);
        ifstream input(argv[3]);
        btree.printing(fileName);
        // print B+-Tree structure to [output file]
        break;
    }
    }

    return 0;
}