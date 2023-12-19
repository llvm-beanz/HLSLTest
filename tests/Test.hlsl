RWBuffer<float> In : register(u0);
RWBuffer<float> Out : register(u1);

[numthreads(8,1,1)]
void main(uint3 TID : SV_DispatchThreadID) {
  Out[TID.x] = TID.x;
  In[TID.x] = TID.x;
  //In[GID] * In[GID];
}
