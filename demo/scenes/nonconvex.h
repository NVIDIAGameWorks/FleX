

class NonConvex : public Scene
{
public:

	NonConvex(const char* name) : Scene(name)
	{
	}

	virtual void Initialize()
	{
		float radius = 0.15f;
		int group = 0;

		for (int i = 0; i < 1; ++i)
			CreateParticleShape(GetFilePathByPlatform("../../data/bowl.obj").c_str(), Vec3(0.0f, 1.0f + 0.5f*i + radius*0.5f, 0.0f), Vec3(1.5f), 0.0f, radius*0.8f, Vec3(0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f, Vec3(0.0f));

		for (int i = 0; i < 50; ++i)
			CreateParticleShape(GetFilePathByPlatform("../../data/banana.obj").c_str(), Vec3(0.4f, 2.5f + i*0.25f, 0.25f) + RandomUnitVector()*radius*0.25f, Vec3(1), 0.0f, radius, Vec3(0.0f), 1.0f, true, 0.5f, NvFlexMakePhase(group++, 0), true, radius*0.1f, 0.0f, 0.0f, 1.25f*Vec4(0.875f, 0.782f, 0.051f, 1.0f));

		AddBox();

		g_numSubsteps = 3;
		g_params.numIterations = 3;

		g_params.radius *= 1.0f;
		g_params.dynamicFriction = 0.35f;
		g_params.dissipation = 0.0f;
		g_params.particleCollisionMargin = g_params.radius*0.05f;
		g_params.sleepThreshold = g_params.radius*0.2f;
		g_params.shockPropagation = 3.0f;
		g_params.gravity[1] *= 1.0f;
		g_params.restitution = 0.01f;
		g_params.damping = 0.25f;

		// draw options
		g_drawPoints = false;

		g_emitters[0].mEnabled = true;
		g_emitters[0].mSpeed = (g_params.radius*2.0f / g_dt);
	}

	virtual void Update()
	{
	}
};