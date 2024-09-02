#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

int n; // number of maximum entries;
int blockCnt = 1;
using namespace std;

class Node;
std::vector<Node *> blocks;

class Node {
public:
  bool isLeaf;
  vector<int> keys;
  int BID;
  vector<int> values;
  Node *next;
  vector<Node *> pointers;

  Node(bool leaf) : isLeaf(leaf), next(nullptr) {
    BID = blockCnt++;
    blocks.push_back(this);
  }
};

class BPlusTree {
private:
  Node *root;
  int n;

public:
  BPlusTree(int n) : root(nullptr), n(n) {}

  void insert(int key, int value) {
    if (!root) {
      root = new Node(true);
      root->keys.push_back(key);
      root->values.push_back(value);
      return;
    }

    Node *leafNode = findLeafNode(root, key);

    if (leafNode->keys.size() < n) {
      insertIntoLeaf(leafNode, key, value);
    } else {
      Node *newLeaf = new Node(true);
      splitLeafNode(leafNode, newLeaf, key, value);

      if (leafNode == root) {
        Node *newRoot = new Node(false);
        newRoot->pointers.push_back(leafNode);
        newRoot->pointers.push_back(newLeaf);
        newRoot->keys.push_back(newLeaf->keys[0]);
        root = newRoot;
      } else {
        insertIntoParent(leafNode, newLeaf->keys[0], newLeaf);
      }
    }
  }

  Node *findLeafNode(Node *node, int key) {
    while (!node->isLeaf) {
      int i = 0;
      while (i < node->keys.size() && key >= node->keys[i])
        i++;
      node = node->pointers[i];
    }
    return node;
  }

  void insertIntoLeaf(Node *leaf, int key, int value) {
    auto it = lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    int index = distance(leaf->keys.begin(), it);
    leaf->keys.insert(it, key);
    leaf->values.insert(leaf->values.begin() + index, value);
  }

  void splitLeafNode(Node *oldLeaf, Node *newLeaf, int key, int value) {
    insertIntoLeaf(oldLeaf, key, value);
    int midIndex = oldLeaf->keys.size() / 2;

    newLeaf->keys.assign(oldLeaf->keys.begin() + midIndex, oldLeaf->keys.end());
    newLeaf->values.assign(oldLeaf->values.begin() + midIndex,
                           oldLeaf->values.end());

    oldLeaf->keys.resize(midIndex);
    oldLeaf->values.resize(midIndex);

    newLeaf->next = oldLeaf->next;
    oldLeaf->next = newLeaf;
  }

  void insertIntoParent(Node *oldNode, int key, Node *newNode) {
    Node *parent = findParent(root, oldNode);
    if (!parent) {
      Node *newRoot = new Node(false);
      newRoot->keys.push_back(key);
      newRoot->pointers.push_back(oldNode);
      newRoot->pointers.push_back(newNode);
      root = newRoot;
      return;
    }

    auto it = lower_bound(parent->keys.begin(), parent->keys.end(), key);
    int index = distance(parent->keys.begin(), it);
    parent->keys.insert(it, key);
    parent->pointers.insert(parent->pointers.begin() + index + 1, newNode);

    if (parent->keys.size() > n) {
      splitNonLeafNode(parent);
    }
  }

  void splitNonLeafNode(Node *node) {
    int midIndex = node->keys.size() / 2;
    Node *newChild = new Node(false);

    newChild->keys.assign(node->keys.begin() + midIndex + 1, node->keys.end());
    newChild->pointers.assign(node->pointers.begin() + midIndex + 1,
                              node->pointers.end());

    node->keys.resize(midIndex);
    node->pointers.resize(midIndex + 1);

    Node *parent = findParent(root, node);
    if (!parent) {
      Node *newRoot = new Node(false);
      newRoot->keys.push_back(node->keys[midIndex]);
      newRoot->pointers.push_back(node);
      newRoot->pointers.push_back(newChild);
      root = newRoot;
    } else {
      insertIntoParent(node, node->keys[midIndex], newChild);
    }
  }

  Node *findParent(Node *parentNode, Node *childNode) {
    if (!parentNode || parentNode->isLeaf) {
      return nullptr;
    }

    for (Node *pointer : parentNode->pointers) {
      if (pointer == childNode) {
        return parentNode;
      }

      Node *fromLowerLevel = findParent(pointer, childNode);
      if (fromLowerLevel) {
        return fromLowerLevel;
      }
    }
    return nullptr;
  }

  int calcDepth() {
    int depth = 0;
    Node *curNode = root;
    while (!curNode->isLeaf) {
      curNode = curNode->pointers[0];
      depth++;
    }
    return depth;
  }

  void check() {
    ofstream outputFile("print.txt");

    if (outputFile.is_open()) {
      outputFile << "<0>" << endl;
      for (int i : root->keys) {
        outputFile << i << ", ";
      }
      outputFile << "\n<1>" << endl;
      for (Node *p : root->pointers) {
        for (int i : p->keys) {
          outputFile << i << ", ";
        }
      }

      outputFile.close();
    } else {
      cout << "error" << endl;
    }
  }

  void insertBin(const char *fileName) {
    int blockSize = n * 8 + 4;
    int rootBid = root->BID;
    int depth = calcDepth();
    int zero = 0;
    cout << "총 블럭 수는:" << blockCnt;
    FILE *file = fopen(fileName, "wb");
    if (file != nullptr) {
      fwrite(&blockSize, sizeof(int), 1, file);
      fwrite(&rootBid, sizeof(int), 1, file);
      fwrite(&depth, sizeof(int), 1, file);

      for (Node *curNode : blocks) {
        for (int k = 0; k < n; k++) {
          if (curNode->isLeaf) {
            if (k < curNode->keys.size()) {
              fwrite(&curNode->keys[k], sizeof(int), 1, file);
              fwrite(&curNode->values[k], sizeof(int), 1, file);
            } else {
              fwrite(&zero, sizeof(int), 2, file);
            }
          } else {
            if (k < curNode->keys.size()) {
              fwrite(&curNode->pointers[k]->BID, sizeof(int), 1, file);
              fwrite(&curNode->keys[k], sizeof(int), 1, file);
            } else {
              fwrite(&zero, sizeof(int), 2, file);
            }
          }
        }
        if (curNode->isLeaf) {
          if (curNode->next) {
            fwrite(&curNode->next->BID, sizeof(int), 1, file);
          } else {
            fwrite(&zero, sizeof(int), 1, file);
          }
        } else {
          fwrite(&curNode->pointers.back()->BID, sizeof(int), 1, file);
        }
      }

      fclose(file);
    } else {
      cout << "Error1" << endl;
    }
  }

  int printing(const char *fileName) {
    ofstream outputFile("print.txt");

    int blockSize, rootBid, depth;
    ifstream binfile(fileName, std::ios::binary);
    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    binfile.read(reinterpret_cast<char *>(&rootBid), sizeof(rootBid));
    binfile.read(reinterpret_cast<char *>(&depth), sizeof(depth));

    n = (blockSize - 4) / 8;
    binfile.seekg(12 + blockSize * (rootBid), std::ios::beg);

    outputFile << "<0>" << endl;
    int pointer, key;
    binfile.read(reinterpret_cast<char *>(&pointer), sizeof(pointer));
    outputFile << pointer << ", ";
    for (int k = 0; k < n; k++) {
      binfile.read(reinterpret_cast<char *>(&key), sizeof(key));
      binfile.read(reinterpret_cast<char *>(&pointer), sizeof(pointer));
      if (key != 0 && key != 2) {
        outputFile << key << ",";
      }
    }

    outputFile << endl << "<1>" << endl;
    binfile.seekg(12 + blockSize * pointer, std::ios::beg);
    for (int k = 0; k < n; k++) {
      binfile.read(reinterpret_cast<char *>(&key), sizeof(key));
      binfile.read(reinterpret_cast<char *>(&pointer), sizeof(pointer));
      if (key != 0) {
        outputFile << key << ",";
      }
    }

    binfile.close();
    outputFile.close();
    return 0;
  }

  int reading(const char *fileName, int searchKey) {
    int blockSize, rootBid, depth;
    ifstream binfile(fileName, std::ios::binary);
    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    binfile.read(reinterpret_cast<char *>(&rootBid), sizeof(rootBid));
    binfile.read(reinterpret_cast<char *>(&depth), sizeof(depth));

    n = (blockSize - 4) / 8;
    int nodeBid = rootBid;
    for (int i = 0; i < depth; i++) {
      vector<int> keys, pointers;
      int p, v;

      binfile.seekg(12 + blockSize * nodeBid, std::ios::beg);
      binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
      pointers.push_back(p);

      for (int k = 0; k < n; k++) {
        binfile.read(reinterpret_cast<char *>(&v), sizeof(v));
        binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
        keys.push_back(v);
        pointers.push_back(p);
      }

      int k;
      for (k = 0; k < n; k++) {
        if (keys[k] >= searchKey) {
          break;
        }
      }
      nodeBid = pointers[k];
    }

    vector<int> keys, values;
    binfile.seekg(12 + blockSize * nodeBid, std::ios::beg);
    for (int k = 0; k < n; k++) {
      int key, value;
      binfile.read(reinterpret_cast<char *>(&key), sizeof(key));
      binfile.read(reinterpret_cast<char *>(&value), sizeof(value));
      keys.push_back(key);
      values.push_back(value);
    }

    for (int k = 0; k < n; k++) {
      if (keys[k] == searchKey) {
        binfile.close();
        return values[k];
      }
    }

    binfile.close();
    return -1;
  }
};

int main(int argc, char *argv[]) {
  blockCnt = 0;
  char command = argv[1][0];
  const char *fileName = argv[2];
  switch (command) {
  case 'c': {
    int blockSize = atoi(argv[3]);
    FILE *file = fopen(fileName, "wb");
    if (file != nullptr) {
      int zero = 0;
      fwrite(&blockSize, sizeof(int), 1, file);
      fwrite(&zero, sizeof(int), 2, file);
      fclose(file);
    } else {
      cout << "Error1" << endl;
    }
    break;
  }
  case 'i': {
    ifstream binfile(fileName, ios::binary);
    int blockSize;
    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    n = (blockSize - 4) / 8;
    ifstream input(argv[3]);
    if (!input) {
      cerr << "Error2";
      return 1;
    }
    BPlusTree btree(n);

    char k[100], v[100];
    while (input.getline(k, sizeof(k), '|') && input.getline(v, sizeof(v))) {
      int key = atoi(k);
      int value = atoi(v);
      btree.insert(key, value);
    }
    btree.check();
    binfile.close();
    btree.insertBin(fileName);
    break;
  }
  case 's': {
    ifstream binfile(fileName, std::ios::binary);
    int blockSize;
    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    n = (blockSize - 4) / 8;
    binfile.close();
    BPlusTree btree(n);
    ifstream input(argv[3]);
    int numb;
    ofstream outputFile(argv[4]);
    while (input >> numb) {
      int val = btree.reading(fileName, numb);
      outputFile << numb << "|" << val << endl;
    }
    outputFile.close();
    break;
  }
  case 'r': {
    break;
  }
  case 'p': {
    ifstream binfile(fileName, std::ios::binary);
    int blockSize;
    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    n = (blockSize - 4) / 8;
    binfile.close();
    BPlusTree btree(n);
    btree.printing(fileName);
    break;
  }
  }

  return 0;
}
