struct vertex_input
{
	float3 position;
	float3 normal;
	float3 color;
};
struct transform_input
{
	float4x4 proj;
	float3 light;
	float light_power;
	float ambient_power;
};
struct vertex_output
{
	float4 position : SV_Position;
	float4 color : COLOR0;
	float3 normal : TEXCOORD0;
};

StructuredBuffer<vertex_input> verticies : register(t0);
transform_input transform : register(b0);

vertex_output vertex_shader(uint id : SV_VertexID)
{
	vertex_input v = verticies[id];
	vertex_output result;
	result.position = mul(float4(v.position, 1.0f), transform.proj);
	result.color = float4(v.color, 1.0f);
	result.normal = v.normal;
	return result;
}

float4 pixel_shader(vertex_output input) : SV_Target
{
	float power = saturate(-dot(transform.light, input.normal)) * transform.light_power + transform.ambient_power;
	return input.color * power;
}
