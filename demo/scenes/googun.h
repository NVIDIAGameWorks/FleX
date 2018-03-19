
class GooGun : public Scene
{
public:

	GooGun(const char* name, bool viscous) : Scene(name), mViscous(viscous) {}

	virtual void Initialize()
	{
		float minSize = 0.5f;
		float maxSize = 1.0f;

		float radius = 0.1f;

		for (int i = 0; i < 5; i++)
			AddRandomConvex(10, Vec3(i*2.0f, 0.0f, Randf(0.0f, 2.0f)), minSize, maxSize, Vec3(0.0f, 1.0f, 0.0f), Randf(0.0f, k2Pi*10.0f));

		g_params.radius = radius;

		g_params.numIterations = 3;
		g_params.vorticityConfinement = 0.0f;
		g_params.fluidRestDistance = g_params.radius*0.55f;
		g_params.smoothing = 0.5f;
		g_params.relaxationFactor = 1.f;
		g_params.restitution = 0.0f;
		g_params.collisionDistance = 0.01f;
		g_params.shapeCollisionMargin = g_params.collisionDistance*0.25f;

		if (mViscous)
		{
			g_fluidColor = Vec4(0.0f, 0.8f, 0.2f, 1.0f);

			g_params.dynamicFriction = 1.0f;
			g_params.viscosity = 50.0f;
			g_params.adhesion = 0.5f;
			g_params.cohesion = 0.3f;
			g_params.surfaceTension = 0.0f;
		}
		else
		{
			g_params.dynamicFriction = 0.25f;
			g_params.viscosity = 0.5f;
			g_params.cohesion = 0.05f;
			g_params.adhesion = 0.0f;
		}

		g_numExtraParticles = 64 * 1024;

#if 1
		NvFlexDistanceFieldId sdf = CreateSDF(GetFilePathByPlatform("../../data/armadillo.ply").c_str(), 128);
		AddSDF(sdf, Vec3(2.0f, 0.0f, -1.0f), Quat(), 2.0f);
#else
		// Test multiple SDFs
		NvFlexDistanceFieldId sdf = CreateSDF(GetFilePathByPlatform("../../data/armadillo.ply").c_str(), 128);
		AddSDF(sdf, Vec3(2.0f, 0.0f, 0.0f), Quat(), 2.0f);

		NvFlexDistanceFieldId sdf2 = CreateSDF(GetFilePathByPlatform("../../data/bunny.ply").c_str(), 128);
		AddSDF(sdf2, Vec3(4.0f, 0.0f, 0.0f), Quat(), 2.0f);
#endif

		g_emitters[0].mEnabled = true;
		g_emitters[0].mSpeed = (g_params.fluidRestDistance*2.f / g_dt);

		// draw options		
		g_drawEllipsoids = true;
		g_pause = false;
	}

	bool mViscous;
};
