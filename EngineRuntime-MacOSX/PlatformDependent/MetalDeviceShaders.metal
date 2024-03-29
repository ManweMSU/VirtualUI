#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

typedef struct {
	int width;
	int height;
	int offset_x;
	int offset_y;
} metal_device_viewport;
typedef struct {
	int left;
	int top;
	int right;
	int bottom;
} metal_device_box;
typedef struct {
	simd_packed_float2 from;
	simd_packed_float2 to;
	simd_packed_float2 side;
	simd_packed_float2 extents;
} metal_device_gradient_box;
typedef struct {
	int left;
	int top;
	int right;
	int bottom;
	int period_x;
	int period_y;
} metal_device_tile_info;
typedef struct {
	int left;
	int top;
	int right;
	int bottom;
	int size_x;
	int size_y;
	float alpha;
} metal_device_layer_info;
typedef struct {
	simd_packed_float2 position;
	simd_packed_float4 color;
	simd_packed_float2 tex_coord;
} metal_device_main_input;
typedef struct {
	float4 position [[position]];
	float4 color;
	float2 tex_coord;
} metal_device_main_output;
typedef struct {
	float4 position [[position]];
	float4 color;
	float2 tex_coord;
	int2 period;
} metal_device_tile_output;

vertex metal_device_main_output MetalDeviceMainVertexShader(
	uint id [[vertex_id]],
	constant metal_device_viewport * viewport [[buffer(0)]],
	constant metal_device_main_input * verticies [[buffer(1)]],
	constant metal_device_box * box [[buffer(2)]])
{
	metal_device_main_output v;
	float x = float(box->left) + float(box->right - box->left) * verticies[id].position.x - viewport->offset_x;
	float y = float(box->top) + float(box->bottom - box->top) * verticies[id].position.y - viewport->offset_y;
	v.position.x = 2.0f * (x / viewport->width) - 1.0f;
	v.position.y = -(2.0f * (y / viewport->height) - 1.0f);
	v.position.z = 0.0f;
	v.position.w = 1.0f;
	v.color = verticies[id].color;
	v.tex_coord = verticies[id].tex_coord;
	return v;
}
vertex metal_device_main_output MetalDeviceMainVertexShaderGradient(
	uint id [[vertex_id]],
	constant metal_device_viewport * viewport [[buffer(0)]],
	constant metal_device_main_input * verticies [[buffer(1)]],
	constant metal_device_gradient_box * box [[buffer(2)]])
{
	metal_device_main_output v;
	float p = verticies[id].position.x;
	float o = verticies[id].position.y;
	if (p < 0.0f) p = -box->extents.x;
	else if (p > 1.0f) p = 1.0f + box->extents.x;
	float2 c = mix(box->from, box->to, p) + o * box->extents.y * box->side;
	float x = c.x - viewport->offset_x;
	float y = c.y - viewport->offset_y;
	v.position.x = 2.0f * (x / viewport->width) - 1.0f;
	v.position.y = -(2.0f * (y / viewport->height) - 1.0f);
	v.position.z = 0.0f;
	v.position.w = 1.0f;
	v.color = verticies[id].color;
	v.tex_coord = verticies[id].tex_coord;
	return v;
}
fragment float4 MetalDeviceMainPixelShader(metal_device_main_output data [[stage_in]], texture2d<float> tex [[texture(0)]])
{
	return data.color * tex.read(uint2(data.tex_coord.x, data.tex_coord.y));
}
fragment float4 MetalDeviceMainPixelShaderNoAlpha(metal_device_main_output data [[stage_in]], texture2d<float> tex [[texture(0)]])
{
	float4 result = data.color * tex.read(uint2(data.tex_coord.x, data.tex_coord.y));
	result.a = 1.0f;
	return result;
}

vertex metal_device_tile_output MetalDeviceTileVertexShader(
	uint id [[vertex_id]],
	constant metal_device_viewport * viewport [[buffer(0)]],
	constant metal_device_main_input * verticies [[buffer(1)]],
	constant metal_device_tile_info * tile [[buffer(2)]])
{
	metal_device_tile_output v;
	float x = float(tile->left) + float(tile->right - tile->left) * verticies[id].position.x - viewport->offset_x;
	float y = float(tile->top) + float(tile->bottom - tile->top) * verticies[id].position.y - viewport->offset_y;
	v.position.x = 2.0f * (x / viewport->width) - 1.0f;
	v.position.y = -(2.0f * (y / viewport->height) - 1.0f);
	v.position.z = 0.0f;
	v.position.w = 1.0f;
	v.color = verticies[id].color;
	v.tex_coord = float2(x, y);
	v.period = int2(tile->period_x, tile->period_y);
	return v;
}
fragment float4 MetalDeviceTilePixelShader(metal_device_tile_output data [[stage_in]], texture2d<float> tex [[texture(0)]])
{
	return data.color * tex.read(uint2(uint(data.tex_coord.x) % data.period.x, uint(data.tex_coord.y) % data.period.y));
}

vertex metal_device_main_output MetalDeviceLayerVertexShader(
	uint id [[vertex_id]],
	constant metal_device_viewport * viewport [[buffer(0)]],
	constant metal_device_main_input * verticies [[buffer(1)]],
	constant metal_device_layer_info * box [[buffer(2)]])
{
	metal_device_main_output v;
	float x = float(box->left) + float(box->right - box->left) * verticies[id].position.x - viewport->offset_x;
	float y = float(box->top) + float(box->bottom - box->top) * verticies[id].position.y - viewport->offset_y;
	v.position.x = 2.0f * (x / viewport->width) - 1.0f;
	v.position.y = -(2.0f * (y / viewport->height) - 1.0f);
	v.position.z = 0.0f;
	v.position.w = 1.0f;
	v.color = float4(box->alpha, box->alpha, box->alpha, box->alpha);
	v.tex_coord = float2(float(box->size_x) * verticies[id].tex_coord.x, float(box->size_y) * verticies[id].tex_coord.y);
	return v;
}
fragment float4 MetalDeviceLayerPixelShader(metal_device_main_output data [[stage_in]], texture2d<float> tex [[texture(0)]])
{
	return data.color * tex.read(uint2(data.tex_coord.x, data.tex_coord.y));
}

vertex metal_device_main_output MetalDeviceBlurVertexShader(
	uint id [[vertex_id]],
	constant metal_device_viewport * viewport [[buffer(0)]],
	constant metal_device_main_input * verticies [[buffer(1)]],
	constant metal_device_layer_info * box [[buffer(2)]])
{
	metal_device_main_output v;
	float x = float(box->left) + float(box->right - box->left) * verticies[id].position.x - viewport->offset_x;
	float y = float(box->top) + float(box->bottom - box->top) * verticies[id].position.y - viewport->offset_y;
	v.position.x = 2.0f * (x / viewport->width) - 1.0f;
	v.position.y = -(2.0f * (y / viewport->height) - 1.0f);
	v.position.z = 0.0f;
	v.position.w = 1.0f;
	v.color = float4(float(box->size_x), float(box->size_y), 0.0f, box->alpha);
	v.tex_coord = float2(float(box->size_x) * verticies[id].tex_coord.x, float(box->size_y) * verticies[id].tex_coord.y);
	return v;
}
fragment float4 MetalDeviceBlurPixelShader(metal_device_main_output data [[stage_in]], texture2d<float> tex [[texture(0)]])
{
	constexpr sampler sam(mag_filter::linear, min_filter::linear);
	uint2 size = uint2(data.color.r, data.color.g);
	int radius = ceil(data.color.a * 3.0);
	float aa = data.color.a * data.color.a;
	float norm = 1.0f / (aa * 6.2831853071795864f);
	float denom = -1.0f / (2.0f * aa);
	float4 result = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 div = 0.0f;
	for (int y = -radius; y <= radius; y++) for (int x = -radius; x <= radius; x++) {
		float2 pos = float2((data.tex_coord.x + x) / data.color.r, (data.tex_coord.y + y) / data.color.g);
		if (pos.x >= size.x || pos.y >= size.y) continue;
		float4 color = tex.sample(sam, pos);
		float weight = norm * exp(denom * (x * x + y * y));
		result += color * weight;
		div += weight;
	}
	return result / div;
}