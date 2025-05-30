#ifndef pvtree_includeu
#define pvtree_includeu

#include "chess.h"

/*
  ������ ������ ���������.
  ������������ ����� ������ + ���

      �������  -> �������� �������
         \
         ������ �����

  ������ ������� ����� ��� ���� � �� ����� �����
  1. �� ����� - ��� ������, �� �������
  2. ������������ ������ �� ����� - ��������, �� ����� ( �� �������� ���� ���� )
*/

typedef HashKey HKey;
typedef Move move;


/*���� ������*/
struct PVNode{
  //// tree /////
  struct PVNode *moves, *nextPly;
  move mv;
  int depth,score,side,check;
  //// hash entry //////
  HKey key;
  struct PVNode *hashList;
  int prioritet;
};

typedef struct PVNode  PVNode;

#define PV_MAX_NODES 10000
#define PV_MAX_HASH  (1<<10)

/* ��� ������ */
struct PVTree{
  //allocator
  PVNode pool[PV_MAX_NODES];
  int    pool_cnt;
  //hash table
  PVNode* hash[PV_MAX_HASH];
  //tree
  PVNode *root;

  
};
typedef struct PVTree PVTree;



void pvReset(PVTree *p);
PVNode *pvNew(PVTree *p);

PVNode** pvGetHashEntry(PVTree *tr,  HKey key);
PVNode* pvFindPosWithHash(PVTree *tr, HKey key, int side);

PVNode **pvFindMove(PVNode **list, move mv);
PVNode  **pvToFront(PVNode **p, PVNode **first);
PVNode *pvNewNode(PVTree *tr, int depth, int side, int score, int check, move mv, HKey key);
PVNode **pvInsertToFront( PVTree* tr, PVNode** first, PVNode* p);
PVNode **pvInsertToEnd( PVTree* tr, PVNode** first, PVNode* p);
PVNode **pvFindNodeTree( PVTree* tr, move game_list[], int cnt );
void pvPrint(PVNode *p, FILE *f);





#endif
