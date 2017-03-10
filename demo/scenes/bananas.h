

class BananaPile : public Scene
{
public:

	BananaPile(const char* name) : Scene(name)
	{
	}

	virtual void Initialize()
	{
		float s = 1.0f;

		Vec3 lower(0.0f, 1.0f + g_params.radius*0.25f, 0.0f);

		int dimx = 3;
		int dimy = 40;
		int dimz = 2;

		float radius = g_params.radius;
		int group = 0;

		// create a basic grid
		for (int x = 0; x < dimx; ++x)
		{
			for (int y = 0; y < dimy; ++y)
			{
				for (int z = 0; z < dimz; ++z)
				{
					CreateParticleShape(GetFilePathByPlatform("../../data/banana.obj").c_str(), lower + (s*1.1f)*Vec3(float(x), y*0.4f, float(z)), Vec3(s), 0.0f, radius*0.95f, Vec3(0.0f), 1.0f, true, 0.8f, NvFlexMakePhase(group++, 0), true, radius*0.1f, 0.0f, 0.0f, 1.25f*Vec4(0.875f, 0.782f, 0.051f, 1.0f));
				}
			}
		}

		AddPlinth();

		g_numSubsteps = 3;
		g_params.numIterations = 2;

		g_params.radius *= 1.0f;
		g_params.dynamicFriction = 0.25f;
		g_params.dissipation = 0.03f;
		g_params.particleCollisionMargin = g_params.radius*0.05f;
		g_params.sleepThreshold = g_params.radius*0.2f;
		g_params.shockPropagation = 2.5f;
		g_params.restitution = 0.55f;
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
