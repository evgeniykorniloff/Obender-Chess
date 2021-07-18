
#include "chess.h"
#include "pvtree.h"

void pvReset(PVTree *p) { memset(p,0,sizeof(PVTree)); }
PVNode *pvNew(PVTree *p) { return p->pool_cnt < PV_MAX_NODES ? &p->pool[p->pool_cnt++] : NULL; }

/*������ ������ ��� �������*/
PVNode** pvGetHashEntry(PVTree *tr,  HKey key){

   return &tr->hash[ (int)key & (PV_MAX_HASH-1) ];

}


/*
 ���� �� ���-����� ������ ������ ��� ������� ����
 (��������� ������ �������� !!!  ��-�� �������������� ��� �����)
 �������� ������������� - ����� ����� ��� ���� ��� �������
 ���� � ����������� �� (���� ������ ������ ������������ ��� ����)
   for( p = pvFindPosWithHash(tree,key,side); p; p = p->moves )
   {
     if( p->mv == ...

   }

*/
PVNode* pvFindPosWithHash(PVTree *tr, HKey key, int side){

   PVNode *p, *find = 0;
   int max = -1000000;

   for( p = *pvGetHashEntry(tr,key); p; p = p->hashList )
       if( p->key == key  &&  p->side == side ){
         if(p->prioritet > max){
           max = p->prioritet;
           find = p;
         }
       }
   return find;
}




/*  ����� ���� � ����� ������ (�� ����� �������)  */
PVNode **pvFindMove(PVNode **list, move mv){
   if( list )
    while( *list ){
      if( (*list)->mv == mv )
          return list;
      list = &(*list)->moves;
    }
   return NULL;
}

/*������������ ���� p ������ � ������ moves, root - ������ ������
 ���-������ �� ��������������, �� ����� ��� ���� �������,
 �.�. ������������ ���� � ������ moves  �� �������� ���������
 ���-�������
*/
PVNode  **pvToFront(PVNode **p, PVNode **first){

  if( p && first && (*p) != (*first) ){
    PVNode *tmp = *p;

    if(*first) tmp->prioritet = (*first)->prioritet + 1;
    else tmp->prioritet = 1;

    *p = tmp->moves;  //��������
    tmp->moves = *first;
    *first = tmp;     //������� � ������
  }
  return first;
}

/*
  ������ ����.
  ��� ���� ���������� ��� ��������.
*/
PVNode *pvNewNode(PVTree *tr, int depth, int side, int score, int check, move mv, HKey key){

   if(tr)
    if(tr->pool_cnt < PV_MAX_NODES){
      PVNode  *p = &tr->pool[ tr->pool_cnt++ ];

      p->mv = mv;
      p->score = score;
      p->depth = depth;
      p->side  = side;
      p->check = check;
      p->key = key;
     // p->prioritet = 0;
      return p;
    }

   return NULL;
}

/*
  ��������� ����� ���� p � ������ ������ first.
  ������������ ��������� ���� p � ��������������� ������ ��� �������
  ��� ���� ���������  p ������ ���� �������� (���� �� ���� mv,key)
*/
PVNode **pvInsertToFront( PVTree* tr, PVNode** first, PVNode* p){

  if( first && p && (*first) != p){
//    PVNode *tmp;

    if(*first)  p->prioritet = (*first)->prioritet+1;
    else p->prioritet = 1;

    p->moves = *first;
    *first = p;
    do{
      PVNode **h = pvGetHashEntry(tr,p->key);
      p->hashList = *h;
      *h = p;
    }while(0);
  }

  return first;
}

/*
  ��������� ����� ���� � ����� ������
*/
PVNode **pvInsertToEnd( PVTree* tr, PVNode** first, PVNode* p){

  if( first && p && (*first) != p){
     PVNode **list = first;
     int min_prioritet = 1000000;
     int find = 0;

     while( *list ){
      if( (*list)->key == p->key  &&  (*list)->side == p->side )
      {
       if( (*list)->prioritet < min_prioritet){
         min_prioritet = (*list)->prioritet;
         find = 1;
       }
      }
      list = &(*list)->moves;
     }

     if(find) p->prioritet = min_prioritet - 1;
     else p->prioritet = 1;

     p->moves = *list;
     *list = p;

     do{
       PVNode **h = pvGetHashEntry(tr,p->key);
       p->hashList = *h;
       *h = p;
     }while(0);

     return list;
  }

  return first;
}

/*
  ���� ������ ���������� �� ����������� ������
  ���������� ������ ������ ��� ������ �������.
  ���� PVTree ������������ ������ � ������� ������,
  �� ����� ����� ���� ���������:

  p = pvFindNodeTree(tree, &game_list[game_cnt], ply);

     ply �� 0

 ��� ��� ������ ������� ������������� �� �������, �� � �������
 ������ �� ����� ����� �������� ������ ����
  pvFindPosWithHash(PVTree *tr, HKey key, int side) ������ �� 0,
  �.�. ���� ����������� ���������� ������ ������� ���������� ������
*/
PVNode **pvFindNodeTree( PVTree* tr, move game_list[], int cnt ){
  int j;
  PVNode **p = &tr->root;
  if(*p){
   if(cnt == 0) return  p;
   for( j = 0; j < cnt; j++ ){
     if( (p = pvFindMove(p, game_list[j])) == NULL )
         return NULL;
     p = &(*p)->nextPly;
   }
  }
  return p;

}

/* ����������� ������ ������ � ���� */

#define SIZE_X 10
#define SIZE_Y 90
PVNode *pvTable[SIZE_Y][SIZE_X];

void pvPrint(PVNode *p, FILE *f){
  char* PrintMove( char buf[], move mv, int check );
  void pvTreeToTable(PVNode *p, int x, int y);
  int x,y;
  char s[16];

  memset( pvTable, 0, sizeof(pvTable));
  pvTreeToTable( p, 0, 0);

  for( y = 0; y < SIZE_Y; y++){
    int find = 0;

    for( x = 0; x < SIZE_X; x++ ){
      if( pvTable[y][x] ){
         find = 1;
         break;
      }
    }
    if(find == 0) break;
    for( x = 0; x < SIZE_X; x++ ){
      if( pvTable[y][x] )
        fprintf( f, "%-7s",  PrintMove(s, pvTable[y][x]->mv, pvTable[y][x]->check));
      else
        fprintf( f, "%-7s","-");
    }
    fprintf(f,"\n");
  }
}

/*
  ������ � �������.


*/
void pvTreeToTable(PVNode *p, int x, int y){
   int GetFirstY(void);                 // ���� ���� ���������� �� ����� X
   int cnt = 0;

   while( p  &&  cnt < 3){
     if(x < SIZE_X  && y < SIZE_Y){
       pvTable[y][x] = p;               //�������� ������ ��� �� ������� Y
     }
     pvTreeToTable(p->nextPly, x + 1, y); // �������� x + 1, y
     p = p->moves;                        //��������� ���
     y = GetFirstY();                     //��������� ������ ��������� Y
     cnt++;
   }
}

int GetFirstY(void){
  int x,y;
   for(y = SIZE_Y-1; y >= 0; y--)
    for(x = SIZE_X-1; x >= 0; x--)
     if( pvTable[y][x] ) return y + 1;
  return 0;
}


char* PrintMove( char buf[], move mv, int check ){

   char *MoveToStr(Move mv, char* str);

   MoveToStr(mv, buf);
   if(check) strcat(buf,"+");
   return buf;
}
