#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;

class Node;
std::vector<std::shared_ptr<Node>> blocks;

class Node {
public:
  bool isLeaf;
  vector<int> keys;
  int BID;
  vector<int> values;
  std::shared_ptr<Node> next;
  vector<std::shared_ptr<Node>> pointers;

  Node(bool leaf, int &blockCnt) : isLeaf(leaf), BID(blockCnt++) {
    blocks.push_back(shared_from_this());
  }

  ~Node() {
    keys.clear();
    if (isLeaf) {
      values.clear();
      next = nullptr;
    } else {
      pointers.clear();
    }
  }
};

class BPlusTree {
private:
  std::shared_ptr<Node> root;
  int n;
  int blockCnt;

public:
  BPlusTree(int n) : root(nullptr), n(n), blockCnt(1) {}

  void insert(int key, int value) {
    if (!root) {
      root = std::make_shared<Node>(true, blockCnt);
      root->keys.push_back(key);
      root->values.push_back(value);
    } else if (root->isLeaf) {
      if (root->keys.size() < n) {
        insertIntoLeaf(root, key, value);
      } else {
        auto newRoot = std::make_shared<Node>(false, blockCnt);
        auto newLeaf = std::make_shared<Node>(true, blockCnt);
        newRoot->pointers.push_back(root);
        newRoot->pointers.push_back(newLeaf);
        splitLeafNode(root, newLeaf, key, value);
        newRoot->keys.push_back(newLeaf->keys[0]);
        root = newRoot;
      }
    } else {
      insertInternal(root, key, value);
    }
  }

  void insertIntoLeaf(std::shared_ptr<Node> leaf, int key, int value) {
    int index = 0;
    while (index < leaf->keys.size() && key >= leaf->keys[index]) {
      index++;
    }
    leaf->keys.insert(leaf->keys.begin() + index, key);
    leaf->values.insert(leaf->values.begin() + index, value);
  }

  void splitLeafNode(std::shared_ptr<Node> oldLeaf,
                     std::shared_ptr<Node> newLeaf, int key, int value) {
    insertIntoLeaf(oldLeaf, key, value);
    int midIndex = (oldLeaf->keys.size() + 1) / 2;

    newLeaf->keys.assign(oldLeaf->keys.begin() + midIndex, oldLeaf->keys.end());
    newLeaf->values.assign(oldLeaf->values.begin() + midIndex,
                           oldLeaf->values.end());

    oldLeaf->keys.resize(midIndex);
    oldLeaf->values.resize(midIndex);

    newLeaf->next = oldLeaf->next;
    oldLeaf->next = newLeaf;
  }

  void insertInternal(std::shared_ptr<Node> currNode, int key, int value) {
    std::shared_ptr<Node> parent = nullptr;

    while (!currNode->isLeaf) {
      parent = currNode;
      int childIndex = 0;
      while (childIndex < currNode->keys.size() &&
             key >= currNode->keys[childIndex]) {
        childIndex++;
      }
      currNode = currNode->pointers[childIndex];
    }

    if (currNode->keys.size() < n) {
      insertIntoLeaf(currNode, key, value);
    } else {
      auto newLeaf = std::make_shared<Node>(true, blockCnt);
      splitLeafNode(currNode, newLeaf, key, value);

      int index = 0;
      while (index < parent->pointers.size() &&
             parent->pointers[index] != currNode) {
        index++;
      }

      parent->pointers.insert(parent->pointers.begin() + index + 1, newLeaf);
      parent->keys.insert(parent->keys.begin() + index, newLeaf->keys[0]);

      if (parent->keys.size() > n) {
        splitNonLeafNode(parent);
      }
    }
  }

  void splitNonLeafNode(std::shared_ptr<Node> node) {
    int midIndex = node->keys.size() / 2;
    auto newChild = std::make_shared<Node>(false, blockCnt);

    newChild->keys.assign(node->keys.begin() + midIndex + 1, node->keys.end());
    newChild->pointers.assign(node->pointers.begin() + midIndex + 1,
                              node->pointers.end());

    node->keys.resize(midIndex);
    node->pointers.resize(midIndex + 1);

    auto parent = findParent(root, node);
    if (!parent) {
      auto newRoot = std::make_shared<Node>(false, blockCnt);
      newRoot->keys.push_back(node->keys[midIndex]);
      newRoot->pointers.push_back(node);
      newRoot->pointers.push_back(newChild);
      root = newRoot;
    } else {
      int index = 0;
      while (index < parent->pointers.size() &&
             parent->pointers[index] != node) {
        index++;
      }
      parent->keys.insert(parent->keys.begin() + index, node->keys[midIndex]);
      parent->pointers.insert(parent->pointers.begin() + index + 1, newChild);

      if (parent->keys.size() > n) {
        splitNonLeafNode(parent);
      }
    }
  }

  std::shared_ptr<Node> findParent(std::shared_ptr<Node> parentNode,
                                   std::shared_ptr<Node> childNode) {
    if (!parentNode || parentNode->isLeaf) {
      return nullptr;
    }

    for (auto &pointer : parentNode->pointers) {
      if (pointer == childNode) {
        return parentNode;
      }

      auto fromLowerLevel = findParent(pointer, childNode);
      if (fromLowerLevel) {
        return fromLowerLevel;
      }
    }
    return nullptr;
  }

  int calcDepth() const {
    int depth = 0;
    auto curNode = root;
    while (!curNode->isLeaf) {
      curNode = curNode->pointers[0];
      depth++;
    }
    return depth;
  }

  void check() const {
    ofstream outputFile("print.txt");

    if (outputFile.is_open()) {
      outputFile << "<0>" << endl;
      for (int i : root->keys) {
        outputFile << i << ", ";
      }
      outputFile << "\n<1>" << endl;
      for (auto &p : root->pointers) {
        for (int i : p->keys) {
          outputFile << i << ", ";
        }
      }

      outputFile.close();
    } else {
      cerr << "Error: Unable to open output file" << endl;
    }
  }

  void insertBin(const char *fileName) const {
    int blockSize = n * 8 + 4;
    int rootBid = root->BID;
    int depth = calcDepth();
    int zero = 0;
    cout << "Total blocks: " << blockCnt << endl;
    FILE *file = fopen(fileName, "wb");
    if (file) {
      fwrite(&blockSize, sizeof(int), 1, file);
      fwrite(&rootBid, sizeof(int), 1, file);
      fwrite(&depth, sizeof(int), 1, file);

      for (const auto &curNode : blocks) {
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
          fwrite(curNode->next ? &curNode->next->BID : &zero, sizeof(int), 1,
                 file);
        } else {
          fwrite(curNode->pointers[n - 1] ? &curNode->pointers[n - 1]->BID
                                          : &zero,
                 sizeof(int), 1, file);
        }
      }

      fclose(file);
    } else {
      cerr << "Error: Unable to open binary file for writing" << endl;
    }
  }

  int printing(const char *fileName) const {
    ofstream outputFile("print.txt");

    int blockSize, rootBid, depth;
    ifstream binfile(fileName, ios::binary);

    if (!binfile.is_open()) {
      cerr << "Error: Unable to open binary file" << endl;
      return -1;
    }

    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    binfile.read(reinterpret_cast<char *>(&rootBid), sizeof(rootBid));
    binfile.read(reinterpret_cast<char *>(&depth), sizeof(depth));

    n = (blockSize - 4) / 8;
    binfile.seekg(12 + blockSize * rootBid, ios::beg);
    root = std::make_shared<Node>(false, blockCnt);

    vector<int> keys;
    vector<int> pointerBlock;

    outputFile << "<0>" << endl;
    int p, v;
    binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
    pointerBlock.push_back(p);

    for (int k = 0; k < n; k++) {
      binfile.read(reinterpret_cast<char *>(&v), sizeof(v));
      binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
      if (v != 0 && v != 2) {
        outputFile << v << ",";
      }
      keys.push_back(v);
      pointerBlock.push_back(p);
    }

    outputFile << "\n<1>" << endl;
    for (int warp : pointerBlock) {
      binfile.seekg(12 + blockSize * warp, ios::beg);
      vector<int> ks;
      vector<int> pb;
      binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
      pb.push_back(p);

      for (int k = 0; k < n; k++) {
        binfile.read(reinterpret_cast<char *>(&v), sizeof(v));
        binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
        ks.push_back(v);
        pb.push_back(p);
        if (v != 0) {
          outputFile << v << ",";
        }
      }
    }

    return 0;
  }

  int reading(const char *fileName, int searchKey) {
    int blockSize, rootBid, depth;
    ifstream binfile(fileName, ios::binary);

    if (!binfile.is_open()) {
      cerr << "Error: Unable to open binary file" << endl;
      return -1;
    }

    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    binfile.read(reinterpret_cast<char *>(&rootBid), sizeof(rootBid));
    binfile.read(reinterpret_cast<char *>(&depth), sizeof(depth));

    n = (blockSize - 4) / 8;
    binfile.seekg(12 + blockSize * rootBid, ios::beg);
    root = std::make_shared<Node>(false, blockCnt);

    int last = rootBid;

    for (int i = 0; i < depth; i++) {
      vector<int> keys;
      vector<int> pointerBlock;
      int p, v;

      binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
      pointerBlock.push_back(p);

      for (int k = 0; k < n; k++) {
        binfile.read(reinterpret_cast<char *>(&v), sizeof(v));
        binfile.read(reinterpret_cast<char *>(&p), sizeof(p));
        keys.push_back(v);
        pointerBlock.push_back(p);
      }

      int k = 0;
      for (k = 0; k < n; k++) {
        if (keys[k] >= searchKey) {
          if (keys[k] == searchKey) {
            break;
          }
          k = 0;
        }
      }

      last = pointerBlock[k];
      binfile.seekg(12 + blockSize * last, ios::beg);
    }

    vector<int> keys, values;

    for (int k = 0; k < n; k++) {
      int ke, v;
      binfile.read(reinterpret_cast<char *>(&ke), sizeof(ke));
      binfile.read(reinterpret_cast<char *>(&v), sizeof(v));
      values.push_back(v);
      keys.push_back(ke);
    }

    auto it = find(keys.begin(), keys.end(), searchKey);
    if (it != keys.end()) {
      return values[distance(keys.begin(), it)];
    }

    return -1;
  }
};

int main(int argc, char *argv[]) {
  char command = argv[1][0];
  const char *fileName = argv[2];

  switch (command) {
  case 'c': {
    int blockSize = atoi(argv[3]);
    FILE *file = fopen(fileName, "wb");
    if (file) {
      int zero = 0;
      fwrite(&blockSize, sizeof(int), 1, file);
      fwrite(&zero, sizeof(int), 2, file);
      fclose(file);
    } else {
      cerr << "Error: Unable to create file" << endl;
    }
    break;
  }
  case 'i': {
    ifstream binfile(fileName, ios::binary);
    int blockSize;
    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    int n = (blockSize - 4) / 8;
    BPlusTree btree(n);

    ifstream input(argv[3]);
    if (!input) {
      cerr << "Error: Unable to open input file" << endl;
      return 1;
    }

    string line;
    while (getline(input, line)) {
      size_t pos = line.find('|');
      if (pos != string::npos) {
        int key = stoi(line.substr(0, pos));
        int value = stoi(line.substr(pos + 1));
        btree.insert(key, value);
      }
    }

    btree.check();
    binfile.close();
    btree.insertBin(fileName);
    break;
  }
  case 's': {
    ifstream binfile(fileName, ios::binary);
    int blockSize;
    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    int n = (blockSize - 4) / 8;
    BPlusTree btree(n);

    ifstream input(argv[3]);
    ofstream outputFile(argv[4]);

    if (!input || !outputFile) {
      cerr << "Error: Unable to open input/output file" << endl;
      return 1;
    }

    int numb;
    while (input >> numb) {
      int val = btree.reading(fileName, numb);
      outputFile << numb << "|" << val << endl;
    }

    break;
  }
  case 'p': {
    ifstream binfile(fileName, ios::binary);
    int blockSize;
    binfile.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
    int n = (blockSize - 4) / 8;
    BPlusTree btree(n);
    btree.printing(fileName);
    break;
  }
  default:
    cerr << "Invalid command" << endl;
  }

  return 0;
}
