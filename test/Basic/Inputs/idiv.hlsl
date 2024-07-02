RWBuffer<int> Buf : register(u0);
RWBuffer<int> Zeros : register(u1);
RWBuffer<int> NegOnes : register(u2);

[numthreads(8,1,1)]
void main(uint3 TID : SV_GroupThreadID) {
  if (TID.x >= 5)
    return;

  Zeros[TID.x] = Buf[TID.x] / Zeros[TID.x];
  NegOnes[TID.x] = Buf[TID.x] / NegOnes[TID.x];
}
