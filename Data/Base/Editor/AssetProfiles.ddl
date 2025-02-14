AssetProfiles
{
	Config %PC
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{8085892115830203315,3737221074888337082}}
				string %t{"plPlatformProfile"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					VarArray %Configs
					{
						Uuid{uint64{11754538365307859884,8572204067211736280}}
						Uuid{uint64{18165925277888737543,17003377905246685588}}
						Uuid{uint64{10688866045028468411,17021851764261564183}}
						Uuid{uint64{12803743945353238805,11270308862817989453}}
					}
					string %Name{"PC"}
					string %Platform{"plProfileTargetPlatform::PC"}
				}
			}
			o
			{
				Uuid %id{uint64{11754538365307859884,8572204067211736280}}
				string %t{"plCoreRenderProfileConfig"}
				uint32 %v{1}
				p
				{
					uint32 %MaxShadowMapSize{1024}
					uint32 %MinShadowMapSize{64}
					uint32 %ShadowAtlasTextureSize{4096}
				}
			}
			o
			{
				Uuid %id{uint64{12803743945353238805,11270308862817989453}}
				string %t{"plXRConfig"}
				uint32 %v{2}
				p
				{
					bool %EnableXR{false}
					string %XRRenderPipeline{"{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"}
				}
			}
			o
			{
				Uuid %id{uint64{18165925277888737543,17003377905246685588}}
				string %t{"plRenderPipelineProfileConfig"}
				uint32 %v{1}
				p
				{
					VarDict %CameraPipelines{}
					string %MainRenderPipeline{"{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"}
				}
			}
			o
			{
				Uuid %id{uint64{10688866045028468411,17021851764261564183}}
				string %t{"plTextureAssetProfileConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{16384}
				}
			}
		}
	}
}
