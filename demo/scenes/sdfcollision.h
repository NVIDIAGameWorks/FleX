
class SDFCollision : public Scene
{
public:

	SDFCollision(const char* name) : Scene(name)
	{
	}

	virtual void Initialize()
	{
		const int dim = 128;

		NvFlexDistanceFieldId sdf = CreateSDF(GetFilePathByPlatform("../../data/bunny.ply").c_str(), dim);

		AddSDF(sdf, Vec3(-1.f, 0.0f, 0.0f), QuatFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), DegToRad(-45.0f)), 0.5f);
		AddSDF(sdf, Vec3(0.0f, 0.0f, 0.0f), QuatFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), DegToRad(0.0f)), 1.0f);
		AddSDF(sdf, Vec3(1.0f, 0.0f, 0.0f), QuatFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), DegToRad(45.0f)), 2.0f);

		float stretchStiffness = 1.0f;
		float bendStiffness = 0.8f;
		float shearStiffness = 0.5f;

		int dimx = 64;
		int dimz = 64;
		float radius = 0.05f;
		int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter);

		CreateSpringGrid(Vec3(-0.6f, 2.9f, -0.6f), dimx, dimz, 1, radius*0.75f, phase, stretchStiffness, bendStiffness, shearStiffness, 0.0f, 1.0f);

		Vec3 lower, upper;
		GetParticleBounds(lower, upper);

		g_params.radius = radius*1.0f;
		g_params.dynamicFriction = 0.4f;
		g_params.staticFriction = 0.4f;
		g_params.particleFriction = 0.25f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 8;
		g_params.viscosity = 0.0f;
		g_params.drag = 0.02f;
		g_params.lift = 0.1f;
		g_params.collisionDistance = radius*0.5f;
		g_params.relaxationFactor = 1.3f;

		g_numSubsteps = 3;

		g_windStrength = 0.0f;

		// draw options
		g_drawPoints = false;
		g_drawSprings = false;
	}
};
