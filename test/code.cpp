void func1(int *x,int *B,int i){
  *x=B[i];
}

int func2(int *B, int i){
  return B[i];
}

int func3(int *B, int i, int flag){
  if(flag){
    return B[i];
  }
  return 0;
}


int main(){
  int x,i,flag;
  int *B,*A;

  // case 1
  func1(&x,B,i);
  A[x];


  // case 2
  A[func2(B,i)];

  // case 3
  A[func3(B,i,flag)];
}