struct vertex_input
{
	float4 position;
	float4 color;
};
struct vertex_output
{
	float4 position : SV_Position;
	float4 color : TEXCOORD0;
};

StructuredBuffer<vertex_input> verticies : register(t0);

vertex_output vertex_shader(uint id : SV_VertexID)
{
	vertex_input v = verticies[id];
	vertex_output result;
	result.position = v.position;
	result.color = v.color;
	return result;
}

float4 pixel_shader(vertex_output input) : SV_Target
{
	return input.color;
}
