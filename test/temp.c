
const int N=100000000;

int main(){
    int A[N],B[N];

    int c=A[1];
    int d=B[c];

    for(int i=0;i+1<N;i++){
        int x = B[A[i]];
        A[i+1] = x;
    }
    
    return 0;
}