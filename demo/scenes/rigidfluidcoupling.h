
class RigidFluidCoupling : public Scene
{
public:

	RigidFluidCoupling(const char* name) : Scene(name) {}

	virtual void Initialize()
	{
		float minSize = 0.5f;
		float maxSize = 1.0f;
		
		float radius = 0.1f;
		int group = 0;

		Randf();

		for (int i=0; i < 5; i++)
			AddRandomConvex(10, Vec3(i*2.0f, 0.0f, Randf(0.0f, 2.0f)), minSize, maxSize, Vec3(0.0f, 1.0f, 0.0f), Randf(0.0f, k2Pi*10.0f));

		for (int z=0; z < 10; ++z)
			for (int x=0; x < 50; ++x)
				CreateParticleShape(
					GetFilePathByPlatform("../../data/box.ply").c_str(), 
					Vec3(x*radius*2 - 1.0f, 1.0f + radius, 1.f + z*2.0f*radius) + 0.5f*Vec3(Randf(radius), 0.0f, Randf(radius)), 
					Vec3(2.0f, 2.0f + Randf(0.0f, 4.0f), 2.0f)*radius*0.5f, 
					0.0f,
					radius*0.5f, 
					Vec3(0.0f), 
					1.0f, 
					true,
					1.0f,
					NvFlexMakePhase(group++, 0),
					true,
					0.0f);

	
		// separte solid particle count
		g_numSolidParticles = g_buffers->positions.size();

		// number of fluid particles to allocate
		g_numExtraParticles = 64*1024;

		g_params.radius = radius;
		g_params.dynamicFriction = 0.5f;
		g_params.fluid = true;
		g_params.viscosity = 0.1f;		
		g_params.numIterations = 3;
		g_params.vorticityConfinement = 0.0f;
		g_params.anisotropyScale = 25.0f;
		g_params.fluidRestDistance = g_params.radius*0.55f;		
		

		g_emitters[0].mEnabled = true;	
		g_emitters[0].mSpeed = 2.0f*(g_params.fluidRestDistance)/g_dt;

		// draw options
		g_drawPoints = false;
		g_drawEllipsoids = true;
	}
};