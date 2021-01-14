#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct vertex_input
{
	simd::packed_float3 position;
	simd::packed_float3 normal;
	simd::packed_float3 color;
};
struct transform_input
{
	float4x4 proj;
	simd::packed_float3 light;
	float light_power;
	float ambient_power;
};
struct vertex_output
{
	float4 position [[position]];
	float4 color;
	float3 normal;
	float3 source;
};

vertex vertex_output vertex_shader(
	uint id [[vertex_id]],
	constant transform_input * transform [[buffer(0)]],
	constant vertex_input * verticies [[buffer(1)]]
)
{
	vertex_input v = verticies[id];
	vertex_output result;
	result.position = float4(v.position, 1.0f) * transform->proj;
	result.color = float4(v.color, 1.0f);
	result.normal = v.normal;
	result.source = v.position;
	return result;
}

fragment float4 pixel_shader(vertex_output input [[stage_in]], constant transform_input * transform [[buffer(0)]])
{
	float power = saturate(-dot(transform->light, input.normal)) * transform->light_power + transform->ambient_power;
	return input.color * power;
}
