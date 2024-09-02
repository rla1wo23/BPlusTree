#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

void setId();
int BLOCKSIZE;
int blockCnt;

struct Entry {
  int key = -1, ptr = -1;
};

struct Block {
  int blockId = -1;
  int ptr = -1;
  Entry *entries;
  Block *parent = nullptr;
  vector<Block *> kids;

  Block() { entries = new Entry[BLOCKSIZE / 8]; }

  ~Block() { delete[] entries; }
};

struct Header {
  int blockSize = 0;
  int rootId = 0;
  int depth = 0;
};

struct BplusTree {
  Block *node = nullptr;
  Header header;
};

BplusTree root;

Block *searchForInsert(int key) {
  Block *curNode = root.node;
  while (curNode && !curNode->kids.empty()) {
    if (key < curNode->entries[0].key) {
      curNode = curNode->kids[0];
    } else {
      int i = 1;
      while (i < BLOCKSIZE / 8 && key >= curNode->entries[i].key &&
             curNode->entries[i].key != -1) {
        ++i;
      }
      curNode = curNode->kids[i];
    }
  }
  return curNode;
}

bool cmp(const pair<int, int> &a, const pair<int, int> &b) {
  return a.first < b.first;
}

bool cmp2(const pair<pair<int, int>, Block *> &a,
          const pair<pair<int, int>, Block *> &b) {
  return a.first.first < b.first.first;
}

void split(Block *c1, Block *c2, int upKey, int cnt) {
  Block *parNode = c1->parent;
  if (!parNode) {
    Block *newPar = new Block;
    newPar->kids = {c1, c2};
    newPar->ptr = c1->blockId;
    Block *tmp = c2;
    while (!tmp->kids.empty())
      tmp = tmp->kids[0];
    newPar->entries[0] = {cnt == 1 ? upKey : tmp->entries[0].key, c2->blockId};
    newPar->blockId = ++blockCnt;
    c1->parent = c2->parent = newPar;
    root.node = newPar;
    root.header.depth++;
  } else if (parNode->entries[BLOCKSIZE / 8 - 1].key == -1) {
    int newEntry = 0;
    while (parNode->entries[newEntry].key != -1)
      newEntry++;

    vector<pair<pair<int, int>, Block *>> arr;
    for (int i = 0; i < newEntry; ++i)
      arr.push_back({{parNode->entries[i].key, parNode->entries[i].ptr},
                     parNode->kids[i + 1]});

    Block *tmp = c2;
    while (!tmp->kids.empty())
      tmp = tmp->kids[0];
    arr.push_back({{cnt == 1 ? upKey : tmp->entries[0].key, c2->blockId}, c2});

    sort(arr.begin(), arr.end(), cmp2);

    parNode->kids = {parNode->kids[0]};
    for (int i = 0; i <= newEntry; ++i) {
      parNode->entries[i] = {arr[i].first.first, arr[i].first.second};
      parNode->kids.push_back(arr[i].second);
    }
    c2->parent = parNode;
  } else {
    Block *newNode = new Block;
    newNode->blockId = ++blockCnt;
    int overNum = BLOCKSIZE / 8 + 1;
    vector<pair<pair<int, int>, Block *>> arr(overNum + 1);
    arr[0] = {{-1, parNode->ptr}, parNode->kids[0]};

    for (int i = 1; i < overNum; ++i) {
      arr[i] = {{parNode->entries[i - 1].key, parNode->entries[i - 1].ptr},
                parNode->kids[i]};
    }
    Block *tmp = c2;
    while (!tmp->kids.empty())
      tmp = tmp->kids[0];
    arr[overNum] = {{cnt == 1 ? upKey : tmp->entries[0].key, c2->blockId}, c2};

    sort(arr.begin(), arr.end(), cmp2);

    int rest = (overNum + 1) / 2;
    parNode->kids = {arr[0].second};
    arr[0].second->parent = parNode;
    for (int i = 1; i < rest; ++i) {
      parNode->entries[i - 1] = {arr[i].first.first, arr[i].first.second};
      parNode->kids.push_back(arr[i].second);
      arr[i].second->parent = parNode;
    }

    newNode->ptr = arr[rest].first.second;
    newNode->kids.push_back(arr[rest].second);
    arr[rest].second->parent = newNode;
    for (int i = rest + 1; i <= overNum; ++i) {
      newNode->entries[i - (rest + 1)] = {arr[i].first.first,
                                          arr[i].first.second};
      newNode->kids.push_back(arr[i].second);
      arr[i].second->parent = newNode;
    }

    split(parNode, newNode, c2->entries[0].key, cnt + 1);
  }
}

void insert(int key, int value) {
  Block *curNode = searchForInsert(key);
  if (!curNode) {
    curNode = new Block;
    root.node = curNode;
    curNode->blockId = ++blockCnt;
  }

  if (curNode->entries[BLOCKSIZE / 8 - 1].key == -1) {
    vector<pair<int, int>> tmp;
    for (int i = 0; i < BLOCKSIZE / 8 && curNode->entries[i].key != -1; ++i) {
      tmp.push_back({curNode->entries[i].key, curNode->entries[i].ptr});
    }
    tmp.push_back({key, value});
    sort(tmp.begin(), tmp.end(), cmp);
    for (size_t i = 0; i < tmp.size(); ++i) {
      curNode->entries[i] = {tmp[i].first, tmp[i].second};
    }
  } else {
    int overNum = BLOCKSIZE / 8 + 1;
    vector<pair<int, int>> arr;
    for (int i = 0; i < BLOCKSIZE / 8; ++i) {
      arr.push_back({curNode->entries[i].key, curNode->entries[i].ptr});
    }
    arr.push_back({key, value});
    sort(arr.begin(), arr.end(), cmp);

    Block *newBlock = new Block;
    newBlock->blockId = ++blockCnt;

    int c1 = overNum / 2;
    for (int i = 0; i < c1; ++i) {
      curNode->entries[i] = {arr[i].first, arr[i].second};
    }
    for (int i = c1; i < BLOCKSIZE / 8; ++i) {
      curNode->entries[i] = {-1, -1};
    }
    for (int i = 0; i < overNum - c1; ++i) {
      newBlock->entries[i] = {arr[i + c1].first, arr[i + c1].second};
    }

    curNode->ptr = newBlock->blockId;
    split(curNode, newBlock, newBlock->entries[0].key, 1);
  }
}

void insertHeader(const string &fileName) {
  root.header.blockSize = BLOCKSIZE;
  if (root.node)
    root.header.rootId = root.node->blockId;

  fstream btree{fileName, ios::out | ios::binary};
  btree.write(reinterpret_cast<char *>(&root.header), sizeof(Header));
}

void fillBlanks(const string &fileOut) {
  fstream btree{fileOut, ios::out | ios::binary};
  int tmp = 0;
  for (int i = 0; i < blockCnt; ++i) {
    btree.seekp(i * BLOCKSIZE + 12);
    btree.write(reinterpret_cast<char *>(&tmp), BLOCKSIZE);
  }
}

void writeToFile(Block *b, fstream &btree) {
  if (b->kids.empty()) {
    btree.seekp((b->blockId - 1) * BLOCKSIZE + 12);
    for (int i = 0; i < BLOCKSIZE / 8; ++i)
      btree.write(reinterpret_cast<char *>(&b->entries[i]), sizeof(Entry));
    btree.write(reinterpret_cast<char *>(&b->ptr), sizeof(int));
  } else {
    btree.seekp((b->blockId - 1) * BLOCKSIZE + 12);
    btree.write(reinterpret_cast<char *>(&b->ptr), sizeof(int));
    for (int i = 0; i < BLOCKSIZE / 8; ++i)
      btree.write(reinterpret_cast<char *>(&b->entries[i]), sizeof(Entry));
    for (auto kid : b->kids)
      writeToFile(kid, btree);
  }
}

void insertNodes(const string &fileIn, const string &fileOut) {
  fstream input{fileIn};
  int key, value;
  char delimiter;

  fstream btree{fileOut, ios::in | ios::binary};
  btree.read(reinterpret_cast<char *>(&BLOCKSIZE), sizeof(int));
  btree.close();
  if (BLOCKSIZE == 0) {
    cout << "Block size is 0\n";
    exit(1);
  }

  while (input >> key >> delimiter >> value)
    insert(key, value);

  fillBlanks(fileOut);
  insertHeader(fileOut);
  setId();

  btree.open(fileOut, ios::in | ios::out | ios::binary);
  writeToFile(root.node, btree);
}

void PointSearch(const string &fileIn, const string &fileIn2,
                 const string &fileOut2) {
  fstream search{fileIn};
  fstream out{fileOut2, ios::out};
  int key;

  while (search >> key) {
    fstream f{fileIn2, ios::in | ios::binary};
    f.read(reinterpret_cast<char *>(&BLOCKSIZE), sizeof(int));
    int idx, depth;
    f.seekg(4);
    f.read(reinterpret_cast<char *>(&idx), sizeof(int));
    f.read(reinterpret_cast<char *>(&depth), sizeof(int));

    Block tmp;
    while (depth--) {
      f.seekg((idx - 1) * BLOCKSIZE + 12);
      f.read(reinterpret_cast<char *>(&tmp.ptr), sizeof(int));
      for (int i = 0; i < BLOCKSIZE / 8; ++i) {
        f.read(reinterpret_cast<char *>(&tmp.entries[i]), sizeof(Entry));
      }
      if (key < tmp.entries[0].key) {
        idx = tmp.ptr;
      } else {
        for (int i = 1; i < BLOCKSIZE / 8; ++i) {
          if (key < tmp.entries[i].key || tmp.entries[i].key == -1) {
            idx = tmp.entries[i - 1].ptr;
            break;
          }
          if (i == BLOCKSIZE / 8 - 1) {
            idx = tmp.entries[i].ptr;
          }
        }
      }
    }
    f.seekg((idx - 1) * BLOCKSIZE + 12);
    for (int i = 0; i < BLOCKSIZE / 8; ++i) {
      f.read(reinterpret_cast<char *>(&tmp.entries[i]), sizeof(Entry));
    }
    for (int i = 0; i < BLOCKSIZE / 8; ++i) {
      if (key == tmp.entries[i].key) {
        out << key << "|" << tmp.entries[i].ptr << '\n';
      }
    }
  }
}

void RangeSearch(const string &fileIn, const string &fileOut,
                 const string &fileOut2) {
  fstream search{fileIn};
  int a, b;
  char delimiter;
  fstream out{fileOut2, ios::out};

  while (search >> a >> delimiter >> b) {
    fstream f{fileOut, ios::in | ios::binary};
    f.read(reinterpret_cast<char *>(&BLOCKSIZE), sizeof(int));
    int idx, depth;
    f.seekg(4);
    f.read(reinterpret_cast<char *>(&idx), sizeof(int));
    f.read(reinterpret_cast<char *>(&depth), sizeof(int));

    Block tmp;
    while (depth--) {
      f.seekg((idx - 1) * BLOCKSIZE + 12);
      f.read(reinterpret_cast<char *>(&tmp.ptr), sizeof(int));
      for (int i = 0; i < BLOCKSIZE / 8; ++i) {
        f.read(reinterpret_cast<char *>(&tmp.entries[i]), sizeof(Entry));
      }
      if (a < tmp.entries[0].key) {
        idx = tmp.ptr;
      } else {
        for (int i = 1; i < BLOCKSIZE / 8; ++i) {
          if (a < tmp.entries[i].key || tmp.entries[i].key == -1) {
            idx = tmp.entries[i - 1].ptr;
            break;
          }
          if (i == BLOCKSIZE / 8 - 1) {
            idx = tmp.entries[i].ptr;
          }
        }
      }
    }
    f.seekg((idx - 1) * BLOCKSIZE + 12);
    for (int i = 0; i < BLOCKSIZE / 8; ++i) {
      f.read(reinterpret_cast<char *>(&tmp.entries[i]), sizeof(Entry));
    }
    f.read(reinterpret_cast<char *>(&tmp.ptr), sizeof(int));
    int probe = 0;
    for (int i = 0; i < BLOCKSIZE / 8; ++i) {
      if (a <= tmp.entries[i].key) {
        probe = i;
        break;
      }
    }
    while (tmp.entries[probe].key <= b) {
      if (tmp.entries[probe].key >= a) {
        out << tmp.entries[probe].key << "|" << tmp.entries[probe].ptr << ' ';
      }
      if (++probe == BLOCKSIZE / 8 || tmp.entries[probe].key == -1) {
        if (tmp.ptr == -1)
          break;
        f.seekg((tmp.ptr - 1) * BLOCKSIZE + 12);
        for (int i = 0; i < BLOCKSIZE / 8; ++i) {
          f.read(reinterpret_cast<char *>(&tmp.entries[i]), sizeof(Entry));
        }
        f.read(reinterpret_cast<char *>(&tmp.ptr), sizeof(int));
        probe = 0;
      }
    }
    out << '\n';
  }
}

void print2LVs(const string &fileIn, const string &fileOut) {
  fstream f{fileIn, ios::in | ios::binary};
  fstream out{fileOut, ios::out};

  int idx, depth;
  f.read(reinterpret_cast<char *>(&BLOCKSIZE), sizeof(int));
  f.seekg(4);
  f.read(reinterpret_cast<char *>(&idx), sizeof(int));
  f.read(reinterpret_cast<char *>(&depth), sizeof(int));

  Block block;
  int *ptrs = new int[BLOCKSIZE / 8 + 1];
  f.seekg((idx - 1) * BLOCKSIZE + 12);
  f.read(reinterpret_cast<char *>(&block.ptr), sizeof(int));
  ptrs[0] = block.ptr;

  out << "<0>\n\n";
  for (int i = 1; i <= BLOCKSIZE / 8; ++i) {
    f.read(reinterpret_cast<char *>(&block.entries[i - 1]), sizeof(Entry));
    if (block.entries[i - 1].key == -1)
      break;
    out << block.entries[i - 1].key << ',';
    ptrs[i] = block.entries[i - 1].ptr;
  }

  out << "\n\n<1>\n\n";
  int cnt = 0;
  while (cnt < BLOCKSIZE / 8 && ptrs[cnt] != -1) {
    f.seekg((ptrs[cnt] - 1) * BLOCKSIZE + 16);
    for (int i = 0; i < BLOCKSIZE / 8; ++i) {
      f.read(reinterpret_cast<char *>(&block.entries[i]), sizeof(Entry));
      if (block.entries[i].key == -1)
        break;
      out << block.entries[i].key << ", ";
    }
    ++cnt;
  }

  out << '\n';
  out.close();
  delete[] ptrs;
}

vector<Block *> totalKids;

void getKids(Block *b) {
  if (b->kids.empty()) {
    for (auto kid : b->parent->kids)
      totalKids.push_back(kid);
  } else {
    for (auto kid : b->kids)
      getKids(kid);
  }
}

void setId() {
  getKids(root.node);
  for (size_t i = 0; i < totalKids.size(); ++i) {
    totalKids[i]->ptr =
        (i + 1 < totalKids.size()) ? totalKids[i + 1]->blockId : -1;
  }
}

int main(int argc, char **argv) {
  string fileIn, btree, fileOut2, tmp;
  switch (argv[1][0]) {
  case 'c':
    btree = argv[2];
    BLOCKSIZE = stoi(argv[3]);
    insertHeader(btree);
    break;
  case 'i':
    btree = argv[2];
    fileIn = argv[3];
    insertNodes(fileIn, btree);
    break;
  case 's':
    btree = argv[2];
    fileIn = argv[3];
    fileOut2 = argv[4];
    PointSearch(fileIn, btree, fileOut2);
    break;
  case 'r':
    btree = argv[2];
    fileIn = argv[3];
    fileOut2 = argv[4];
    RangeSearch(fileIn, btree, fileOut2);
    break;
  case 'p':
    fileIn = argv[2];
    btree = argv[3];
    print2LVs(fileIn, btree);
    break;
  }
}
