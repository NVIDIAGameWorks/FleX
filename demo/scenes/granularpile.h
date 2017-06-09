

class GranularPile : public Scene
{
public:

	GranularPile(const char* name) : Scene(name) {}

	virtual void Initialize()
	{
		// granular pile
		float radius = 0.075f;

		Vec3 lower(8.0f, 4.0f, 2.0f);

		CreateParticleShape(GetFilePathByPlatform("../../data/sphere.ply").c_str(), lower, 1.0f, 0.0f, radius, 0.0f, 0.f, true, 1.0f, NvFlexMakePhase(1, 0), true, 0.00f);
		g_numSolidParticles = g_buffers->positions.size();

		CreateParticleShape(GetFilePathByPlatform("../../data/sandcastle.obj").c_str(), Vec3(-2.0f, -radius*0.15f, 0.0f), 4.0f, 0.0f, radius*1.0001f, 0.0f, 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide), false, 0.00f);

		g_numSubsteps = 2;
	
		g_params.radius = radius;
		g_params.staticFriction = 1.0f;
		g_params.dynamicFriction = 0.5f;
		g_params.viscosity = 0.0f;
		g_params.numIterations = 12;
		g_params.particleCollisionMargin = g_params.radius*0.25f;	// 5% collision margin
		g_params.sleepThreshold = g_params.radius*0.25f;
		g_params.shockPropagation = 6.f;
		g_params.restitution = 0.2f;
		g_params.relaxationFactor = 1.f;
		g_params.damping = 0.14f;
		g_params.numPlanes = 1;
		
		// draw options
		g_drawPoints = true;		
		g_warmup = false;

		// hack, change the color of phase 0 particles to 'sand'		
		g_colors[0] = Colour(0.805f, 0.702f, 0.401f);		
	}

	void Update()
	{
		// launch ball after 3 seconds
		if (g_frame == 180)
		{
			for (int i=0; i < g_numSolidParticles; ++i)
			{
				g_buffers->positions[i].w = 0.9f;
				g_buffers->velocities[i] = Vec3(-15.0f, 0.0f, 0.0f);
			}
		}
	}
};