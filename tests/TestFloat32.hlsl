RWBuffer<float> In : register(u0);
RWBuffer<float> Out : register(u1);

[numthreads(8,1,1)]
void main(uint3 TID : SV_GroupThreadID) {
  Out[TID.x] = In[TID.x] + 4.3;
}
