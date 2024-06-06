RWBuffer<float4> Tex;

const static float3 Palette[8] = {float3(0.0, 0.0, 0.0), float3(0.5, 0.5, 0.5),
                                  float3(1.0, 0.5, 0.5), float3(0.5, 1.0, 0.5),
                                  float3(0.5, 0.5, 1.0), float3(0.5, 1.0, 1.0),
                                  float3(1.0, 0.5, 1.0), float3(1.0, 1.0, 0.5)};

[numthreads(32, 32, 1)] void main(uint3 index
                                  : SV_DispatchThreadID) {
  uint2 dispatchSize = 1024.xx;
  float x0 = 2.0 * index.x / dispatchSize.x - 1.5;
  float y0 = 2.0 * index.y / dispatchSize.y - 1.0;

  // Implement Mandelbrot set
  float x = x0;
  float y = y0;
  uint iteration = 0;
  uint max_iteration = 200;
  float xtmp = 0.0;
  bool diverged = false;
  for (; iteration < max_iteration; ++iteration) {
    if (x * x + y * y > 2000 * 2000) {
      diverged = true;
      break;
    }
    xtmp = x * x - y * y + x0;
    y = 2 * x * y + y0;
    x = xtmp;
  }

  float3 Color = float3(0, 0, 0);
  if (diverged) {
    float Gradient = 1.0;
    float Smooth = log2(log2(x * x + y * y) / 2.0);
    float ColorIdx = sqrt((float)+10.0 - Smooth) * Gradient;
    float LerpSize = frac(ColorIdx);
    LerpSize = LerpSize * LerpSize * (3.0 - 2.0 * LerpSize);
    int ColorIdx1 = (int)ColorIdx % 8;
    int ColorIdx2 = (ColorIdx1 + 1) % 8;
    Color = lerp(Palette[ColorIdx1], Palette[ColorIdx2], LerpSize.xxx);
  }

  Tex[(index.y * 1024) + index.x] = float4(Color, 1.0);
}
