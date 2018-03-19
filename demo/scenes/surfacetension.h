
class SurfaceTension : public Scene
{
public:

	SurfaceTension(const char* name, float surfaceTension) : Scene(name), surfaceTension(surfaceTension) {}

	virtual void Initialize()
	{
		mCounter = 0;

		float radius = 0.1f;
		float restDistance = radius*0.55f;
		int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid);

		CreateParticleGrid(Vec3(0.0f, 0.2f, -1.0f), 16, 64, 16, restDistance, Vec3(0.0f), 1.0f, false, 0.0f, phase, 0.005f);

		g_params.radius = radius;

		g_params.numIterations = 3;
		g_params.vorticityConfinement = 0.0f;
		g_params.fluidRestDistance = restDistance;
		g_params.smoothing = 0.5f;
		g_params.relaxationFactor = 1.f;
		g_params.restitution = 0.0f;
		g_params.collisionDistance = 0.01f;

		g_params.dynamicFriction = 0.25f;
		g_params.viscosity = 0.5f;
		g_params.cohesion = 0.1f;
		g_params.adhesion = 0.0f;
		g_params.surfaceTension = surfaceTension;

		g_params.gravity[1] = 0.0f;

		g_numExtraParticles = 64 * 1024;

		g_emitters[0].mEnabled = true;
		g_emitters[0].mSpeed = (g_params.fluidRestDistance*2.f / g_dt);

		g_lightDistance *= 2.0f;

		// draw options		
		g_drawEllipsoids = true;
	}

	void Update()
	{
		if (g_frame == 300)
			g_params.gravity[1] = -9.8f;
	}

	int mCounter;
	float surfaceTension;
};
