RWBuffer<float4> Tex;

const static float3 Palette[8] = {float3(0.0, 0.0, 0.0), float3(0.5, 0.5, 0.5),
                                  float3(1.0, 0.5, 0.5), float3(0.5, 1.0, 0.5),
                                  float3(0.5, 0.5, 1.0), float3(0.5, 1.0, 1.0),
                                  float3(1.0, 0.5, 1.0), float3(1.0, 1.0, 0.5)};

[numthreads(1024, 1, 1)] void main(uint3 DID : SV_DispatchThreadID) {
  uint2 Index = uint2(DID.x%1024, DID.x/1024);
  uint2 DispatchSize = 1024.xx;
  float X0 = 2.0 * (float)Index.x / (float)DispatchSize.x - 1.5;
  float Y0 = 2.0 * (float)Index.y / (float)DispatchSize.y - 1.0;

  // Implement Mandelbrot set
  float X = X0;
  float Y = Y0;
  uint Iteration = 0;
  uint MaxIteration = 2000;
  float XTmp = 0.0;
  bool Diverged = false;
  for (; Iteration < MaxIteration; ++Iteration) {
    if (X * X + Y * Y > 2000 * 2000) {
      Diverged = true;
      break;
    }
    XTmp = X * X - Y * Y + X0;
    Y = 2 * X * Y + Y0;
    X = XTmp;
  }

  float3 Color = float3(0, 0, 0);
  if (Diverged) {
    float Gradient = 1.0;
    float Smooth = log2(log2(X * X + Y * Y) / 2.0);
    float ColorIdx = sqrt((float)+10.0 - Smooth) * Gradient;
    float LerpSize = frac(ColorIdx);
    LerpSize = LerpSize * LerpSize * (3.0 - 2.0 * LerpSize);
    int ColorIdx1 = (int)ColorIdx % 8;
    int ColorIdx2 = (ColorIdx1 + 1) % 8;
    Color = lerp(Palette[ColorIdx1], Palette[ColorIdx2], LerpSize.xxx);
  }

  Tex[DID.x] = float4(Color, 1.0);
}
