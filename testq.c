#include <stdio.h>
#include <stdlib.h>

typedef struct Qnode{
    int process;
    struct Qnode* next;
} qnode;

typedef struct Queue{
  struct Qnode* front;
  struct Qnode* rear;
  int size;
} queue;

queue* createQueue(){
    queue* q = (queue*)malloc(sizeof(queue));
    q->front = (void*)0;
    q->rear = (void*)0;
    q->size = 0;
    return q;
}

void addToQueue(queue* q,int p){
  qnode* newn = (qnode*)malloc(sizeof(qnode));
  newn->process = p;
  newn->next = (void*)0;
  q->size++;
  if(q->size == 1){
    q->front = newn;
    q->rear = newn;
  }else{
    q->rear->next = newn;
    q->rear = newn;
  }
}

int pop(queue* q){
  if(q->size == 0)
    return -1;
  int r = q->front->process;
  qnode* temp = q->front;
  q->front = q->front->next;
  if(q->front == (void*)0)
    q->rear = (void*)0;
  free(temp);
  q->size--;
  return r;
}

int main(){
    queue*q = createQueue();
    addToQueue(q,1);
    addToQueue(q,2);
    addToQueue(q,3);
    addToQueue(q,4);
    int x = pop(q);
    int y = pop(q);
    int z = pop(q);
    int a = pop(q);
    printf("%d%d%d%d\n",x,y,z,a);
}