
class PlasticStack : public Scene
{
public:

	PlasticStack(const char* name) : Scene(name) {}
	
	virtual void Initialize()
	{
		g_params.radius = 0.225f;
		
		g_params.numIterations = 2;
		g_params.dynamicFriction = 0.5f;
		g_params.particleFriction = 0.15f;
		g_params.dissipation = 0.0f;
		g_params.viscosity = 0.0f;

		AddPlinth();

		const float rotation = -kPi*0.5f;
		const float spacing = g_params.radius*0.5f;

		// alternative box and sphere shapes
		const char* mesh[] = 
		{ 
			"../../data/box_high.ply",
			"../../data/sphere.ply" 
		};

		Vec3 lower = Vec3(4.0f, 1.0f, 0.0f);
		float sizeInc = 0.0f;
		float size = 1.0f;
		int group = 0;

		for (int i=0; i < 8; ++i)
		{
			CreateParticleShape(GetFilePathByPlatform(mesh[i%2]).c_str(), lower, size + i*sizeInc, rotation, spacing, Vec3(.0f, 0.0f, 0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f, 0.0f, g_params.radius*0.5f);

			lower += Vec3(0.0f, size + i*sizeInc + 0.2f, 0.0f);
		}

		g_params.plasticThreshold = 0.00025f;
		g_params.plasticCreep = 0.165f;

		g_numSubsteps = 4;

		// draw options		
		g_drawPoints = false;
	}
};
