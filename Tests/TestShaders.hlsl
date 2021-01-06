struct vertex_input
{
	float3 position;
	float3 color;
};
struct transform_input
{
	float4x4 proj;
};
struct vertex_output
{
	float4 position : SV_Position;
	float4 color : COLOR0;
};

StructuredBuffer<vertex_input> verticies : register(t0);
StructuredBuffer<transform_input> transform : register(t1);

vertex_output vertex_shader(uint id : SV_VertexID)
{
	vertex_input v = verticies[id];
	vertex_output result;
	result.position = mul(float4(v.position, 1.0f), transform[0].proj);
	result.color = float4(v.color, 1.0f);
	return result;
}

float4 pixel_shader(vertex_output input) : SV_Target
{
	return input.color;
}
