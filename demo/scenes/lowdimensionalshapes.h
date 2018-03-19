
class LowDimensionalShapes: public Scene
{
public:

	LowDimensionalShapes(const char* name) : Scene(name) {}

	virtual void Initialize()
	{
		float radius = 0.1f;
		int group = 0;

		Mesh* mesh = ImportMesh(GetFilePathByPlatform("../../data/box.ply").c_str());

		CreateParticleShape(mesh, Vec3(0.0f, 1.0f, 0.0f), Vec3(1.2f, 0.001f, 1.2f), 0.0f, radius, Vec3(0.0f, 0.0f, 0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);

		for (int i=0; i < 64; ++i)
			CreateParticleShape(mesh, Vec3(i / 8 * radius, 0.0f, i % 8 * radius), Vec3(0.1f, 0.8f, 0.1f), 0.0f, radius, Vec3(0.0f, 0.0f, 0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);

		delete mesh;

		g_params.radius = radius;
		g_params.dynamicFriction = 1.0f;
		g_params.fluidRestDistance = radius;
		g_params.viscosity = 0.0f;
		g_params.numIterations = 4;
		g_params.vorticityConfinement = 0.f;
		g_params.numPlanes = 1;
		g_params.collisionDistance = radius*0.5f;
		g_params.shockPropagation = 5.0f;
				
		g_numSubsteps = 2;

		g_maxDiffuseParticles = 0;
		g_diffuseScale = 0.75f;

		g_lightDistance *= 1.5f;
		
		g_fluidColor = Vec4(0.2f, 0.6f, 0.9f, 1.0f);

		// draw options
		g_drawDensity = false;
		g_drawDiffuse = false;
		g_drawEllipsoids = false;
		g_drawPoints = true;
		g_drawMesh = false;

		g_warmup = false;

	}
		
};
