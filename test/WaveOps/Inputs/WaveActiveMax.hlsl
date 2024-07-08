RWBuffer<float> Nans : register(u0);
RWBuffer<float> Infs : register(u1);
RWBuffer<float> NegInfs : register(u2);
RWBuffer<float> Mix : register(u3);

[numthreads(32,1,1)]
void main(uint3 TID : SV_GroupThreadID) {
  Nans[TID.x % 8] = WaveActiveMax(Nans[TID.x % 8]);
  Infs[TID.x % 8] = WaveActiveMax(Infs[TID.x % 8]);
  NegInfs[TID.x % 8] = WaveActiveMax(NegInfs[TID.x % 8]);
  Mix[TID.x % 8] = WaveActiveMax(Mix[TID.x % 8]);
}
