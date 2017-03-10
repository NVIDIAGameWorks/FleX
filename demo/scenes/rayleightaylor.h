
class RayleighTaylor3D : public Scene
{
public:

	RayleighTaylor3D(const char* name) : Scene(name) {}

	int base;
	int width;
	int height;
	int depth;

	virtual void Initialize()
	{
		float radius = 0.05f;
		float restDistance = radius*0.5f;

		width = 128;
		height = 24;
		depth = 24;

		base = 4;

		float sep = restDistance*0.9f;
		int group = 0;

		CreateParticleGrid(Vec3(0.0f, 0.0f, 0.0f), width, base, depth, sep, Vec3(0.0f), 0.0f, false, 0.0f, NvFlexMakePhase(group++, 0), 0.0f);
		CreateParticleGrid(Vec3(0.0f, base*sep, 0.0f), width, height, depth, sep, Vec3(0.0f), 0.24f, false, 0.0f, NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid), restDistance*0.01f);
		CreateParticleGrid(Vec3(0.0f, sep*height + base*sep, 0.0f), width, height, depth, sep, Vec3(0.0f), 0.25f, false, 0.0f, NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid), restDistance*0.01f);

		g_params.gravity[1] = -9.f;
		g_params.radius = radius;
		g_params.dynamicFriction = 0.00f;
		g_params.fluid = true;
		g_params.viscosity = 2.0f;
		g_params.numIterations = 10;
		g_params.vorticityConfinement = 0.0f;
		g_params.anisotropyScale = 50.0f;
		g_params.smoothing = 1.f;
		g_params.fluidRestDistance = restDistance;
		g_params.numPlanes = 5;
		g_params.cohesion = 0.0002125f;
		g_params.surfaceTension = 0.0f;
		g_params.collisionDistance = 0.001f;//restDistance*0.5f;
		//g_params.solidPressure = 0.2f;

		g_params.relaxationFactor = 20.0f;
		g_numSubsteps = 5;

		g_fluidColor = Vec4(0.2f, 0.6f, 0.9f, 1.0f);

		g_lightDistance *= 0.85f;

		// draw options
		g_drawDensity = true;
		g_drawDiffuse = false;
		g_drawEllipsoids = false;
		g_drawPoints = true;

		g_blur = 2.0f;

		g_warmup = true;
	}

	virtual void Update()
	{
		if (g_params.numPlanes == 4)
			g_params.dynamicFriction = 0.2f;

		if (g_frame == 32)
		{
			int layer1start = width*depth*base;
			int layer1end = layer1start + width*height*depth;
			for (int i = layer1start; i < layer1end; ++i)
				g_buffers->positions[i].w = 1.0f;
		}
	}
};

class RayleighTaylor2D : public Scene
{
public:

	RayleighTaylor2D(const char* name) : Scene(name) {}

	int base;
	int width;
	int height;
	int depth;

	virtual void Initialize()
	{
		float radius = 0.05f;
		float restDistance = radius*0.5f;

		width = 128;
		height = 24;
		depth = 1;

		base = 4;

		float sep = restDistance*0.7f;
		int group = 0;

		CreateParticleGrid(Vec3(0.0f, 0.0f, 0.0f), width, base, depth, sep, Vec3(0.0f), 0.0f, false, 0.0f, NvFlexMakePhase(group++, 0), 0.0f);
		CreateParticleGrid(Vec3(0.0f, base*sep, 0.0f), width, height, depth, sep, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid), restDistance*0.01f);
		CreateParticleGrid(Vec3(0.0f, sep*height + base*sep, 0.0f), width, height, depth, sep, Vec3(0.0f), 0.25f, false, 0.0f, NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid), restDistance*0.01f);

		g_params.gravity[1] = -9.f;
		g_params.radius = radius;
		g_params.dynamicFriction = 0.00f;
		g_params.fluid = true;
		g_params.viscosity = 0.0f;
		g_params.numIterations = 10;
		g_params.vorticityConfinement = 0.0f;
		g_params.anisotropyScale = 50.0f;
		g_params.smoothing = 1.f;
		g_params.fluidRestDistance = restDistance;
		g_params.numPlanes = 5;
		g_params.cohesion = 0.0025f;
		g_params.surfaceTension = 0.0f;
		g_params.collisionDistance = 0.001f;
		g_params.restitution = 0.0f;

		g_params.relaxationFactor = 1.0f;
		g_numSubsteps = 10;

		g_fluidColor = Vec4(0.2f, 0.6f, 0.9f, 1.0f);

		g_lightDistance *= 0.85f;

		// draw options
		g_drawDensity = false;
		g_drawDiffuse = false;
		g_drawEllipsoids = false;
		g_drawPoints = true;

		g_pointScale = 0.9f;
		g_blur = 2.0f;

		g_warmup = true;
	}
};