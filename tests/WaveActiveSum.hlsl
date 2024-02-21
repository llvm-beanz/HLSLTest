RWBuffer<int> value;

[numthreads(4, 1, 1)]
void main(uint3 threadID : SV_DispatchThreadID) {
  uint sum = 0;
  switch (value[threadID.x]) {
    case 0:
      sum += WaveActiveSum(1);
    default:
      sum += WaveActiveSum(10);
      break;
  }
  value[threadID.x] = sum;
}
