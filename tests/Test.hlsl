Buffer<float> In : register(t0);
RWBuffer<float> Out : register(u0);

[numthreads(8,1,1)]
void main(uint GID : SV_GroupIndex) {
  Out[GID] = In[GID];
}
