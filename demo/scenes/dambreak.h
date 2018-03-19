

class DamBreak : public Scene
{
public:

	DamBreak(const char* name, float radius) : Scene(name), mRadius(radius) {}

	virtual void Initialize()
	{
		const float radius = mRadius;
		const float restDistance = mRadius*0.65f;

		int dx = int(ceilf(1.0f / restDistance));
		int dy = int(ceilf(2.0f / restDistance));
		int dz = int(ceilf(1.0f / restDistance));

		CreateParticleGrid(Vec3(0.0f, restDistance, 0.0f), dx, dy, dz, restDistance, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid), restDistance*0.01f);

		g_sceneLower = Vec3(0.0f, 0.0f, -0.5f);
		g_sceneUpper = Vec3(3.0f, 0.0f, -0.5f);

		g_numSubsteps = 2;

		g_params.radius = radius;
		g_params.fluidRestDistance = restDistance;
		g_params.dynamicFriction = 0.f;
		g_params.restitution = 0.001f;

		g_params.numIterations = 3;
		g_params.relaxationFactor = 1.0f;

		g_params.smoothing = 0.4f;
		
		g_params.viscosity = 0.001f;
		g_params.cohesion = 0.1f;
		g_params.vorticityConfinement = 80.0f;
		g_params.surfaceTension = 0.0f;

		g_params.numPlanes = 5;

		// limit velocity to CFL condition
		g_params.maxSpeed = 0.5f*radius*g_numSubsteps / g_dt;

		g_maxDiffuseParticles = 0;

		g_fluidColor = Vec4(0.113f, 0.425f, 0.55f, 1.0f);

		g_waveFrequency = 1.0f;
		g_waveAmplitude = 2.0f;
		g_waveFloorTilt = 0.0f;

		// draw options		
		g_drawPoints = true;
		g_drawEllipsoids = false;
		g_drawDiffuse = true;
	}

	float mRadius;
};
