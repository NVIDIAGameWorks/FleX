

class FluidBlock : public Scene
{
public:

	FluidBlock(const char* name) : Scene(name) {}

	virtual void Initialize()
	{
		float minSize = 0.5f;
		float maxSize = 0.7f;
		
		float radius = 0.1f;
		float restDistance = radius*0.55f;
		int group = 0;

		AddRandomConvex(6, Vec3(5.0f, -0.1f, 0.6f), 1.0f, 1.0f, Vec3(1.0f, 1.0f, 0.0f), 0.0f);

		float ly = 0.5f;
		
		AddRandomConvex(10, Vec3(2.5f, ly*0.5f, 1.f), minSize*0.5f, maxSize*0.5f, Vec3(0.0f, 1.0f, 0.0f), Randf(0.0f, 2.0f*kPi));

		AddRandomConvex(12, Vec3(3.8f, ly-0.5f, 1.f), minSize, maxSize, Vec3(1.0f, 0.0f, 0.0f), Randf(0.0f, 2.0f*kPi));
		AddRandomConvex(12, Vec3(3.8f, ly-0.5f, 2.6f), minSize, maxSize, Vec3(1.0f, 0.0f, 0.0f), 0.2f + Randf(0.0f, 2.0f*kPi));

		AddRandomConvex(12, Vec3(4.6f, ly, 0.2f), minSize, maxSize, Vec3(1.0f, 0.0f, 1.0f), Randf(0.0f, 2.0f*kPi));
		AddRandomConvex(12, Vec3(4.6f, ly, 2.0f), minSize, maxSize, Vec3(1.0f, 0.0f, 1.0f), 0.2f + Randf(0.0f, 2.0f*kPi));
	
		float size = 0.3f;
		for (int i=0; i < 32; ++i)
			CreateParticleShape(GetFilePathByPlatform("../../data/torus.obj").c_str(), Vec3(4.5f, 2.0f + radius*2.0f*i, 1.0f), size, 0.0f, radius*0.5f, Vec3(0.0f, 0.0f, 0.0f), 0.125f, true, 1.0f, NvFlexMakePhase(group++, 0), true, 0.0f);

		g_numSolidParticles = g_buffers->positions.size();	

		float sizex = 1.76f;
		float sizey = 2.20f;
		float sizez = 3.50f;

		int x = int(sizex/restDistance);
		int y = int(sizey/restDistance);
		int z = int(sizez/restDistance);

		CreateParticleGrid(Vec3(0.0f, restDistance*0.5f, 0.0f), x, y, z, restDistance, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid));		

		g_params.radius = radius;
		g_params.dynamicFriction = 0.0f;
		g_params.fluid = true;
		g_params.viscosity = 0.0f;
		g_params.numIterations = 3;
		g_params.vorticityConfinement = 40.f;
		g_params.anisotropyScale = 20.0f;
		g_params.fluidRestDistance = restDistance;
		g_params.numPlanes = 5;		
		//g_params.cohesion = 0.05f;

		g_maxDiffuseParticles = 128*1024;
		g_diffuseScale = 0.75f;

		g_waveFloorTilt = -0.025f; 
		
		g_lightDistance *= 0.5f;

		// draw options
		g_drawDensity = true;
		g_drawDiffuse = true;
		g_drawEllipsoids = true;
		g_drawPoints = false;

		g_warmup = true;
	}
};