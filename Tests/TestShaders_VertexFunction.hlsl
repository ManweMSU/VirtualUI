struct egsl_type_Vertex {
	float3 egsl_field_position : TEXCOORD0;
	float3 egsl_field_normal : TEXCOORD1;
	float3 egsl_field_color : TEXCOORD2;
};
struct egsl_type_World {
	float4x4 egsl_field_proj;
	float3 egsl_field_light : TEXCOORD0;
	float egsl_field_light_power : TEXCOORD1;
	float egsl_field_ambient_power : TEXCOORD2;
};
struct egsl_type_Data {
	float4 egsl_field_color : TEXCOORD0;
	float3 egsl_field_normal : TEXCOORD1;
	float3 egsl_field_source : TEXCOORD2;
};
StructuredBuffer<egsl_type_Vertex> egsl_arg_verticies : register(t0);
egsl_type_World egsl_arg_world : register(b0);
void egsl_shader_VertexFunction(in uint egsl_arg_id : SV_VertexID, out egsl_type_Data egsl_arg_data, out float4 egsl_arg_position : SV_Position)
{
	{
		egsl_type_Vertex egsl_local_0_input = egsl_arg_verticies[egsl_arg_id];
		egsl_arg_position = mul(egsl_arg_world.egsl_field_proj, float4(egsl_local_0_input.egsl_field_position, 1.0f));
		egsl_arg_data.egsl_field_color = float4(egsl_local_0_input.egsl_field_color, 1.0f);
		egsl_arg_data.egsl_field_normal = egsl_local_0_input.egsl_field_normal;
		egsl_arg_data.egsl_field_source = egsl_local_0_input.egsl_field_position;
	}
}
