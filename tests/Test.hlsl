RWBuffer<float> In : register(u0);
RWBuffer<float> Out : register(u1);

[numthreads(8,1,1)]
void main(uint GID : SV_GroupIndex) {
  Out[GID] = In[GID];
}
