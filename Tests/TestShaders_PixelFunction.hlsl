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
egsl_type_World egsl_arg_world : register(b0);
Texture2D<float4> egsl_arg_tex : register(t0);
SamplerState egsl_arg_sam : register(s0);
void egsl_shader_PixelFunction(in egsl_type_Data egsl_arg_input, out float4 egsl_arg_color : SV_Target0)
{
	{
		float egsl_local_0_power = ((saturate(-(dot(egsl_arg_world.egsl_field_light, egsl_arg_input.egsl_field_normal)))) * (egsl_arg_world.egsl_field_light_power)) + (egsl_arg_world.egsl_field_ambient_power);
		egsl_arg_color = (egsl_arg_input.egsl_field_color) * (float4(egsl_local_0_power, egsl_local_0_power, egsl_local_0_power, egsl_local_0_power));
	}
}
